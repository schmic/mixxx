#include "waveform/renderers/allshader/waveformrenderphrase.h"

#include <QDomNode>
#include <algorithm>
#include <iterator>

#include "moc_waveformrenderphrase.cpp"
#include "rendergraph/material/unicolormaterial.h"
#include "rendergraph/vertexupdaters/vertexupdater.h"
#include "skin/legacy/skincontext.h"
#include "track/track.h"
#include "waveform/renderers/waveformwidgetrenderer.h"
#include "waveform/waveformwidgetfactory.h"
#include "widget/wskincolor.h"

using namespace rendergraph;

namespace {

const QColor kDefaultPhraseMarkerColor(224, 64, 64);

float phraseMarkerHalfWidth(double scaleFactor) {
    return static_cast<float>(std::max(3.0, scaleFactor * 3.5));
}

float phraseMarkerHeight(double scaleFactor) {
    return static_cast<float>(std::max(4.0, scaleFactor * 5.0));
}

} // namespace

namespace allshader {

WaveformRenderPhrase::WaveformRenderPhrase(WaveformWidgetRenderer* waveformWidget,
        ::WaveformRendererAbstract::PositionSource type)
        : ::WaveformRendererAbstract(waveformWidget),
          m_isSlipRenderer(type == ::WaveformRendererAbstract::Slip),
          m_introStartPosCO(m_waveformRenderer->getGroup(),
                  QStringLiteral("intro_start_position")) {
    initForRectangles<UniColorMaterial>(0);
    setUsePreprocess(true);
}

void WaveformRenderPhrase::setup(const QDomNode& node, const SkinContext& skinContext) {
    QColor color(skinContext.selectString(node, QStringLiteral("PhraseMarkerColor")));
    m_color = WSkinColor::getCorrectColor(color.isValid() ? color : kDefaultPhraseMarkerColor)
                      .toRgb();
}

void WaveformRenderPhrase::onSetTrack() {
    if (m_pLoadedTrack) {
        disconnect(m_pLoadedTrack.get(),
                &Track::beatsUpdated,
                this,
                &WaveformRenderPhrase::updateFirstDownbeat);
        disconnect(m_pLoadedTrack.get(),
                &Track::cuesUpdated,
                this,
                &WaveformRenderPhrase::updateFirstDownbeat);
    }

    m_pLoadedTrack = m_waveformRenderer->getTrackInfo();
    updateFirstDownbeat();

    if (m_pLoadedTrack) {
        connect(m_pLoadedTrack.get(),
                &Track::beatsUpdated,
                this,
                &WaveformRenderPhrase::updateFirstDownbeat);
        connect(m_pLoadedTrack.get(),
                &Track::cuesUpdated,
                this,
                &WaveformRenderPhrase::updateFirstDownbeat);
    }
}

void WaveformRenderPhrase::updateFirstDownbeat() {
    m_pTrackBeats = m_pLoadedTrack ? m_pLoadedTrack->getBeats() : nullptr;
    m_firstDownbeat.reset();
    if (!m_pTrackBeats) {
        return;
    }

    const auto introCuePos = mixxx::audio::FramePos::fromEngineSamplePosMaybeInvalid(
            m_introStartPosCO.get());
    if (!introCuePos.isValid()) {
        return;
    }
    const auto beatIt = m_pTrackBeats->iteratorFrom(
            m_pTrackBeats->findClosestBeat(introCuePos));
    if (beatIt != m_pTrackBeats->cend()) {
        m_firstDownbeat = beatIt;
    }
}

void WaveformRenderPhrase::draw(QPainter* painter, QPaintEvent* event) {
    Q_UNUSED(painter);
    Q_UNUSED(event);
    DEBUG_ASSERT(false);
}

void WaveformRenderPhrase::preprocess() {
    if (!preprocessInner()) {
        geometry().allocate(0);
        markDirtyGeometry();
    }
}

bool WaveformRenderPhrase::preprocessInner() {
    if (!m_pTrackBeats || !m_firstDownbeat ||
            (m_isSlipRenderer && !m_waveformRenderer->isSlipActive())) {
        return false;
    }

    auto* pFactory = WaveformWidgetFactory::instance();
    if (!pFactory->getDownbeatsEnabled()) {
        return false;
    }
    const int downbeatDistance = pFactory->getDownbeatDistance();

    const int alpha = m_waveformRenderer->getBeatGridAlpha();
    if (alpha == 0) {
        return false;
    }

    QColor color = m_color;
    color.setAlphaF(alpha / 100.0f);

    const double trackSamples = m_waveformRenderer->getTrackSamples();
    if (trackSamples <= 0.0) {
        return false;
    }

    const auto positionType = m_isSlipRenderer ? ::WaveformRendererAbstract::Slip
                                               : ::WaveformRendererAbstract::Play;

    const double firstDisplayedPosition =
            m_waveformRenderer->getFirstDisplayedPosition(positionType);
    const double lastDisplayedPosition =
            m_waveformRenderer->getLastDisplayedPosition(positionType);

    const auto startPosition = mixxx::audio::FramePos::fromEngineSamplePos(
            firstDisplayedPosition * trackSamples);
    const auto endPosition = mixxx::audio::FramePos::fromEngineSamplePos(
            lastDisplayedPosition * trackSamples);

    if (!startPosition.isValid() || !endPosition.isValid()) {
        return false;
    }

    auto it = m_pTrackBeats->iteratorFrom(startPosition);
    if (it == m_pTrackBeats->cend() || *it > endPosition) {
        return false;
    }

    const auto isMarkedBar = [&](mixxx::Beats::ConstIterator beat) {
        return std::distance(*m_firstDownbeat, beat) % downbeatDistance == 0;
    };

    int numMarkers = 0;
    for (auto countIt = it; countIt != m_pTrackBeats->cend() && *countIt <= endPosition;
            ++countIt) {
        if (isMarkedBar(countIt)) {
            ++numMarkers;
        }
    }

    if (numMarkers == 0) {
        return false;
    }

    constexpr int numVerticesPerTriangle = 3;
    geometry().allocate(numMarkers * numVerticesPerTriangle);

    const float devicePixelRatio = m_waveformRenderer->getDevicePixelRatio();
    const float triangleHalfWidth = phraseMarkerHalfWidth(scaleFactor());
    const float triangleHeight = phraseMarkerHeight(scaleFactor());

    VertexUpdater vertexUpdater{geometry().vertexDataAs<Geometry::Point2D>()};

    for (; it != m_pTrackBeats->cend() && *it <= endPosition; ++it) {
        if (!isMarkedBar(it)) {
            continue;
        }

        const double beatPosition = it->toEngineSamplePos();
        double xBeatPoint = m_waveformRenderer->transformSamplePositionInRendererWorld(
                beatPosition, positionType);
        xBeatPoint = qRound(xBeatPoint * devicePixelRatio) / devicePixelRatio;

        const float x = static_cast<float>(xBeatPoint);
        vertexUpdater.addTriangle({x - triangleHalfWidth, 0.f},
                {x + triangleHalfWidth, 0.f},
                {x, triangleHeight});
    }

    markDirtyGeometry();
    DEBUG_ASSERT(numMarkers * numVerticesPerTriangle == vertexUpdater.index());

    material().setUniform(1, color);
    markDirtyMaterial();

    return true;
}

} // namespace allshader
