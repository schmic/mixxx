#pragma once

#include <QColor>
#include <optional>

#include "control/pollingcontrolproxy.h"
#include "rendergraph/geometrynode.h"
#include "track/beats.h"
#include "track/track_decl.h"
#include "util/class.h"
#include "waveform/renderers/waveformrendererabstract.h"

class QDomNode;
class SkinContext;

namespace allshader {
class WaveformRenderPhrase;
} // namespace allshader

class allshader::WaveformRenderPhrase final
        : public QObject,
          public ::WaveformRendererAbstract,
          public rendergraph::GeometryNode {
    Q_OBJECT
  public:
    explicit WaveformRenderPhrase(WaveformWidgetRenderer* waveformWidget,
            ::WaveformRendererAbstract::PositionSource type =
                    ::WaveformRendererAbstract::Play);

    void draw(QPainter* painter, QPaintEvent* event) override final;
    void setup(const QDomNode& node, const SkinContext& skinContext) override;
    void onSetTrack() override;
    void preprocess() override;

  private slots:
    void updateFirstDownbeat();

  private:
    bool preprocessInner();

    QColor m_color;
    bool m_isSlipRenderer;
    PollingControlProxy m_introStartPosCO;
    TrackPointer m_pLoadedTrack;
    mixxx::BeatsPointer m_pTrackBeats;
    std::optional<mixxx::Beats::ConstIterator> m_firstDownbeat;

    DISALLOW_COPY_AND_ASSIGN(WaveformRenderPhrase);
};
