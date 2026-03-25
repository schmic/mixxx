#include "waveform/renderers/waveformrenderbeat.h"

#include <QPainter>
#include <QPainterPath>
#include <algorithm>

#include "track/track.h"
#include "util/painterscope.h"
#include "waveform/renderers/waveformwidgetrenderer.h"
#include "widget/wskincolor.h"

class QPaintEvent;

namespace {

constexpr int kPhraseMarkerPeriodBeats = 4;
const QColor kDefaultPhraseMarkerColor(224, 64, 64);

qreal phraseMarkerHalfWidth(double scaleFactor) {
    return std::max<qreal>(3.0, scaleFactor * 3.5);
}

qreal phraseMarkerHeight(double scaleFactor) {
    return std::max<qreal>(4.0, scaleFactor * 5.0);
}

bool isPhraseMarkerBeat(int beatIndex) {
    return beatIndex % kPhraseMarkerPeriodBeats == 0;
}

void appendPhraseMarker(QPainterPath* path,
        Qt::Orientation orientation,
        qreal beatPoint,
        qreal halfWidth,
        qreal height) {
    if (orientation == Qt::Horizontal) {
        path->moveTo(beatPoint - halfWidth, 0.0);
        path->lineTo(beatPoint + halfWidth, 0.0);
        path->lineTo(beatPoint, height);
    } else {
        path->moveTo(0.0, beatPoint - halfWidth);
        path->lineTo(0.0, beatPoint + halfWidth);
        path->lineTo(height, beatPoint);
    }
    path->closeSubpath();
}

} // namespace

WaveformRenderBeat::WaveformRenderBeat(WaveformWidgetRenderer* waveformWidgetRenderer)
        : WaveformRendererAbstract(waveformWidgetRenderer) {
    m_beats.resize(128);
}

WaveformRenderBeat::~WaveformRenderBeat() {
}

void WaveformRenderBeat::setup(const QDomNode& node, const SkinContext& context) {
    m_beatColor = QColor(context.selectString(node, "BeatColor"));
    m_beatColor = WSkinColor::getCorrectColor(m_beatColor).toRgb();
    QColor phraseMarkerColor(context.selectString(node, "PhraseMarkerColor"));
    m_phraseMarkerColor = WSkinColor::getCorrectColor(
            phraseMarkerColor.isValid() ? phraseMarkerColor
                                        : kDefaultPhraseMarkerColor)
                                  .toRgb();
}

void WaveformRenderBeat::draw(QPainter* painter, QPaintEvent* /*event*/) {
    TrackPointer pTrackInfo = m_waveformRenderer->getTrackInfo();

    if (!pTrackInfo) {
        return;
    }

    mixxx::BeatsPointer trackBeats = pTrackInfo->getBeats();
    if (!trackBeats) {
        return;
    }

    int alpha = m_waveformRenderer->getBeatGridAlpha();
    if (alpha == 0) {
        return;
    }
#ifdef MIXXX_USE_QOPENGL
    // Using alpha transparency with drawLines causes a graphical issue when
    // drawing with QPainter on the QOpenGLWindow: instead of individual lines
    // a large rectangle encompassing all beatlines is drawn.
    m_beatColor.setAlphaF(1.f);
#else
    m_beatColor.setAlphaF(alpha / 100.0);
#endif
    QColor phraseMarkerColor = m_phraseMarkerColor;
    phraseMarkerColor.setAlphaF(alpha / 100.0f);

    const double trackSamples = m_waveformRenderer->getTrackSamples();
    if (trackSamples <= 0) {
        return;
    }

    const float devicePixelRatio = m_waveformRenderer->getDevicePixelRatio();

    const double firstDisplayedPosition =
            m_waveformRenderer->getFirstDisplayedPosition();
    const double lastDisplayedPosition =
            m_waveformRenderer->getLastDisplayedPosition();

    // qDebug() << "trackSamples" << trackSamples
    //          << "firstDisplayedPosition" << firstDisplayedPosition
    //          << "lastDisplayedPosition" << lastDisplayedPosition;

    const auto startPosition = mixxx::audio::FramePos::fromEngineSamplePos(
            firstDisplayedPosition * trackSamples);
    const auto endPosition = mixxx::audio::FramePos::fromEngineSamplePos(
            lastDisplayedPosition * trackSamples);
    auto it = trackBeats->iteratorFrom(startPosition);

    // if no beat do not waste time saving/restoring painter
    if (it == trackBeats->cend() || *it > endPosition) {
        return;
    }

    PainterScope PainterScope(painter);

    painter->setRenderHint(QPainter::Antialiasing);

    QPen beatPen(m_beatColor);
    beatPen.setWidthF(std::max(1.0, scaleFactor()));
    painter->setPen(beatPen);

    const Qt::Orientation orientation = m_waveformRenderer->getOrientation();
    const float rendererWidth = m_waveformRenderer->getWidth();
    const float rendererHeight = m_waveformRenderer->getHeight();
    const qreal triangleHalfWidth = phraseMarkerHalfWidth(scaleFactor());
    const qreal triangleHeight = phraseMarkerHeight(scaleFactor());
    const auto referenceBeat = trackBeats->cfirstmarker();
    QPainterPath phraseMarkers;

    int beatCount = 0;
    int beatIndex = it - referenceBeat;

    for (; it != trackBeats->cend() && *it <= endPosition; ++it, ++beatIndex) {
        double beatPosition = it->toEngineSamplePos();
        double xBeatPoint =
                m_waveformRenderer->transformSamplePositionInRendererWorld(beatPosition);

        xBeatPoint = qRound(xBeatPoint * devicePixelRatio) / devicePixelRatio;

        // If we don't have enough space, double the size.
        if (beatCount >= m_beats.size()) {
            m_beats.resize(m_beats.size() * 2);
        }

        if (orientation == Qt::Horizontal) {
            m_beats[beatCount++].setLine(xBeatPoint, 0.0f, xBeatPoint, rendererHeight);
        } else {
            m_beats[beatCount++].setLine(0.0f, xBeatPoint, rendererWidth, xBeatPoint);
        }

        if (isPhraseMarkerBeat(beatIndex)) {
            appendPhraseMarker(
                    &phraseMarkers, orientation, xBeatPoint, triangleHalfWidth, triangleHeight);
        }
    }

    // Make sure to use constData to prevent detaches!
    painter->drawLines(m_beats.constData(), beatCount);
    painter->fillPath(phraseMarkers, phraseMarkerColor);
}
