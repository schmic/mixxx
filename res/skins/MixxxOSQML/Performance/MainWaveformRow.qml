import "../Theme"
import Mixxx 1.0 as Mixxx
import QtQuick
import QtQuick.Controls

Item {
    id: root

    required property real deckWaveformHeight
    required property bool skinControlsReady
    readonly property var editPanelsControl: editPanelsControlLoader.item
    readonly property bool editPanelsVisible: (editPanelsControl?.value ?? 0) > 0
    readonly property real editPanelsWidth: editPanelsVisible ? TouchTheme.waveformEditPanelWidth : 0

    Loader {
        id: editPanelsControlLoader

        // Transient skin controls may not exist until component completion.
        active: root.skinControlsReady
        sourceComponent: Mixxx.ControlProxy {
            group: "[Skin]"
            key: "show_beatgrid_controls"
        }
    }

    DeckHotcueGrid {
        id: deck1Hotcues

        anchors.top: parent.top
        group: "[Channel1]"
        height: TouchTheme.hotcueRowHeight
        width: root.width
    }
    DeckWaveform {
        id: deck1Waveform

        anchors.top: deck1Hotcues.bottom
        anchors.topMargin: TouchTheme.hotcueWaveformSpacing
        accentColor: TouchTheme.deck1Accent
        group: "[Channel1]"
        height: root.deckWaveformHeight
        width: root.width - root.editPanelsWidth
    }
    WaveformEditPanel {
        anchors.right: parent.right
        anchors.top: deck1Waveform.top
        accentColor: TouchTheme.deck1Accent
        bottomInset: TouchTheme.waveformEditButtonSize / 2
        group: "[Channel1]"
        height: root.deckWaveformHeight
        visible: root.editPanelsVisible
        width: TouchTheme.waveformEditPanelWidth
    }
    DeckWaveform {
        id: deck2Waveform

        anchors.top: deck1Waveform.bottom
        accentColor: TouchTheme.deck2Accent
        group: "[Channel2]"
        height: root.deckWaveformHeight
        width: root.width - root.editPanelsWidth
    }
    WaveformEditPanel {
        anchors.right: parent.right
        anchors.top: deck2Waveform.top
        accentColor: TouchTheme.deck2Accent
        topInset: TouchTheme.waveformEditButtonSize / 2
        group: "[Channel2]"
        height: root.deckWaveformHeight
        visible: root.editPanelsVisible
        width: TouchTheme.waveformEditPanelWidth
    }
    DeckHotcueGrid {
        anchors.top: deck2Waveform.bottom
        anchors.topMargin: TouchTheme.hotcueWaveformSpacing
        group: "[Channel2]"
        height: TouchTheme.hotcueRowHeight
        width: root.width
    }
    Button {
        id: editPanelsToggle

        anchors.right: parent.right
        y: deck2Waveform.y - height / 2
        width: TouchTheme.waveformEditButtonSize
        height: TouchTheme.waveformEditButtonSize
        enabled: root.editPanelsControl?.initialized ?? false
        z: 2

        Accessible.name: root.editPanelsVisible ? qsTr("Hide waveform editing panels") : qsTr("Show waveform editing panels")
        Accessible.checkable: true
        Accessible.checked: root.editPanelsVisible

        onClicked: root.editPanelsControl.toggle()

        background: Rectangle {
            color: editPanelsToggle.down ? TouchTheme.controlPressedBackground : TouchTheme.controlBackground
        }
        contentItem: Item {
            Image {
                anchors.centerIn: parent
                width: TouchTheme.waveformEditIconSize
                height: TouchTheme.waveformEditIconSize
                sourceSize.width: TouchTheme.waveformEditIconSize
                sourceSize.height: TouchTheme.waveformEditIconSize
                fillMode: Image.PreserveAspectFit
                opacity: root.editPanelsVisible ? 1.0 : 0.6
                source: "../Icons/waveform-edit.svg"
            }
        }
    }
}
