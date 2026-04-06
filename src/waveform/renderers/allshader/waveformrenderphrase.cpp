#include "waveform/renderers/allshader/waveformrenderphrase.h"

#include <QDomNode>
#include <algorithm>

#include "rendergraph/material/unicolormaterial.h"
#include "rendergraph/vertexupdaters/vertexupdater.h"
#include "skin/legacy/skincontext.h"
#include "track/track.h"
#include "waveform/renderers/waveformwidgetrenderer.h"
#include "widget/wskincolor.h"

using namespace rendergraph;

namespace {

constexpr int kPhraseMarkerPeriodBeats = 4;
const QColor kDefaultPhraseMarkerColor(224, 64, 64);

float phraseMarkerHalfWidth(double scaleFactor) {
    return static_cast<float>(std::max(3.0, scaleFactor * 3.5));
}

float phraseMarkerHeight(double scaleFactor) {
    return static_cast<float>(std::max(4.0, scaleFactor * 5.0));
}

bool isPhraseMarkerBeat(int beatIndex) {
    return beatIndex % kPhraseMarkerPeriodBeats == 0;
}

} // namespace

namespace allshader {

WaveformRenderPhrase::WaveformRenderPhrase(WaveformWidgetRenderer* waveformWidget,
        ::WaveformRendererAbstract::PositionSource type)
        : ::WaveformRendererAbstract(waveformWidget),
          m_isSlipRenderer(type == ::WaveformRendererAbstract::Slip) {
    initForRectangles<UniColorMaterial>(0);
    setUsePreprocess(true);
}

void WaveformRenderPhrase::setup(const QDomNode& node, const SkinContext& skinContext) {
    QColor color(skinContext.selectString(node, QStringLiteral("PhraseMarkerColor")));
    m_color = WSkinColor::getCorrectColor(color.isValid() ? color : kDefaultPhraseMarkerColor)
                      .toRgb();
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
    const TrackPointer trackInfo = m_waveformRenderer->getTrackInfo();

    if (!trackInfo || (m_isSlipRenderer && !m_waveformRenderer->isSlipActive())) {
        return false;
    }

    const auto positionType = m_isSlipRenderer ? ::WaveformRendererAbstract::Slip
                                               : ::WaveformRendererAbstract::Play;

    const mixxx::BeatsPointer trackBeats = trackInfo->getBeats();
    if (!trackBeats) {
        return false;
    }

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

    auto it = trackBeats->iteratorFrom(startPosition);
    if (it == trackBeats->cend() || *it > endPosition) {
        return false;
    }

    const auto referenceBeat = trackBeats->cfirstmarker();
    int beatIndex = it - referenceBeat;
    int numPhraseMarkers = 0;
    for (auto countIt = it; countIt != trackBeats->cend() && *countIt <= endPosition;
            ++countIt, ++beatIndex) {
        if (isPhraseMarkerBeat(beatIndex)) {
            ++numPhraseMarkers;
        }
    }

    if (numPhraseMarkers == 0) {
        return false;
    }

    constexpr int numVerticesPerTriangle = 3;
    geometry().allocate(numPhraseMarkers * numVerticesPerTriangle);

    const float devicePixelRatio = m_waveformRenderer->getDevicePixelRatio();
    const float triangleHalfWidth = phraseMarkerHalfWidth(scaleFactor());
    const float triangleHeight = phraseMarkerHeight(scaleFactor());

    VertexUpdater vertexUpdater{geometry().vertexDataAs<Geometry::Point2D>()};

    beatIndex = it - referenceBeat;
    for (; it != trackBeats->cend() && *it <= endPosition; ++it, ++beatIndex) {
        if (!isPhraseMarkerBeat(beatIndex)) {
            continue;
        }

        double beatPosition = it->toEngineSamplePos();
        double xBeatPoint = m_waveformRenderer->transformSamplePositionInRendererWorld(
                beatPosition, positionType);
        xBeatPoint = qRound(xBeatPoint * devicePixelRatio) / devicePixelRatio;

        const float x = static_cast<float>(xBeatPoint);
        vertexUpdater.addTriangle({x - triangleHalfWidth, 0.f},
                {x + triangleHalfWidth, 0.f},
                {x, triangleHeight});
    }

    markDirtyGeometry();
    DEBUG_ASSERT(numPhraseMarkers * numVerticesPerTriangle == vertexUpdater.index());

    material().setUniform(1, color);
    markDirtyMaterial();

    return true;
}

} // namespace allshader
