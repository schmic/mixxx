import "../Theme"
import Mixxx 1.0 as Mixxx
import Mixxx.Controls 1.0 as MixxxControls
import QtQuick

Rectangle {
    id: root

    required property color accentColor
    required property string group
    readonly property var player: Mixxx.PlayerManager.getPlayer(root.group)

    border.color: TouchTheme.border
    border.width: 1
    clip: true
    color: TouchTheme.overviewBackground
    height: TouchTheme.deckOverviewHeight

    MixxxControls.WaveformOverview {
        anchors.fill: parent
        anchors.leftMargin: 4
        anchors.margins: 2
        colorHigh: TouchTheme.waveformHigh
        colorLow: TouchTheme.waveformLow
        colorMid: TouchTheme.waveformMid
        group: root.group
        opacity: root.player?.isLoaded ? 1.0 : 0.25
        renderer: Mixxx.WaveformOverview.Renderer.RGB
    }
    Rectangle {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.top: parent.top
        color: root.accentColor
        width: 3
    }
}
