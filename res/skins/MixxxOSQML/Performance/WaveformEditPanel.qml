pragma ComponentBehavior: Bound

import "../Theme"
import Mixxx 1.0 as Mixxx
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    required property color accentColor
    required property string group
    property real topInset: 0
    property real bottomInset: 0
    readonly property var player: Mixxx.PlayerManager.getPlayer(root.group)
    readonly property bool gridEditable: (root.player?.isLoaded ?? false) && bpmLockControl.value === 0

    color: TouchTheme.controlBackground

    Rectangle {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.top: parent.top
        width: TouchTheme.hotcueColorStripeHeight
        color: TouchTheme.border
        z: 1
    }
    Mixxx.ControlProxy {
        id: bpmLockControl

        group: root.group
        key: "bpmlock"
    }
    Flickable {
        id: panelFlickable

        anchors.fill: parent
        anchors.topMargin: root.topInset
        anchors.bottomMargin: root.bottomInset
        boundsBehavior: Flickable.StopAtBounds
        clip: true
        contentHeight: Math.max(height, buttons.implicitHeight)
        contentWidth: width
        flickableDirection: Flickable.VerticalFlick

        GridLayout {
            id: buttons

            y: Math.max(0, (panelFlickable.height - implicitHeight) / 2)
            width: panelFlickable.width
            columns: 2
            columnSpacing: TouchTheme.waveformEditButtonSpacing
            rowSpacing: TouchTheme.waveformEditButtonSpacing

            EditButton {
                controlKey: "beats_translate_earlier"
                enabled: root.gridEditable
                iconSource: "../Icons/beatgrid-earlier.svg"
                Accessible.name: qsTr("Move beatgrid earlier by a small step")
            }
            EditButton {
                controlKey: "beats_translate_later"
                enabled: root.gridEditable
                iconSource: "../Icons/beatgrid-later.svg"
                Accessible.name: qsTr("Move beatgrid later by a small step")
            }
            EditButton {
                controlKey: "quantize"
                enabled: root.player?.isLoaded ?? false
                togglesControl: true
                iconSource: "../Icons/quantize.svg"
                Accessible.name: qsTr("Quantize")
            }
            EditButton {
                controlKey: "beats_translate_half"
                enabled: root.gridEditable
                iconSource: "../Icons/beatgrid-half.svg"
                Accessible.name: qsTr("Move beatgrid later by half a beat")
            }
            EditButton {
                controlKey: "bpmlock"
                enabled: root.player?.isLoaded ?? false
                togglesControl: true
                iconSource: bpmLockControl.value > 0 ? "../Icons/locked.svg" : "../Icons/unlocked.svg"
                Accessible.name: qsTr("BPM lock")
            }
            EditButton {
                controlKey: "intro_start_set"
                enabled: root.player?.isLoaded ?? false
                iconSource: "../Icons/intro-set.svg"
                Accessible.name: qsTr("Set intro start at the current position")
            }
        }
    }

    component EditButton: Button {
        id: button

        required property string controlKey
        required property url iconSource
        property bool togglesControl: false
        readonly property bool controlActive: togglesControl && control.value > 0

        Accessible.checkable: togglesControl
        Accessible.checked: controlActive

        Layout.minimumWidth: TouchTheme.waveformEditButtonSize
        Layout.maximumWidth: TouchTheme.waveformEditButtonSize
        Layout.preferredWidth: TouchTheme.waveformEditButtonSize
        Layout.minimumHeight: TouchTheme.waveformEditButtonSize
        Layout.maximumHeight: TouchTheme.waveformEditButtonSize
        Layout.preferredHeight: TouchTheme.waveformEditButtonSize
        display: AbstractButton.IconOnly
        icon.source: button.iconSource
        icon.width: TouchTheme.waveformEditIconSize
        icon.height: TouchTheme.waveformEditIconSize
        icon.color: !button.enabled ? TouchTheme.mutedText :
            button.controlActive ? root.accentColor : TouchTheme.primaryText

        onClicked: {
            if (togglesControl) {
                control.toggle();
            } else {
                control.trigger();
            }
        }

        Mixxx.ControlProxy {
            id: control

            group: root.group
            key: button.controlKey
        }
        background: Rectangle {
            color: button.down ? TouchTheme.controlPressedBackground : TouchTheme.controlBackground

            Rectangle {
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 2
                anchors.horizontalCenter: parent.horizontalCenter
                width: 3
                height: 3
                color: TouchTheme.primaryText
                visible: button.visualFocus
            }
        }
    }
}
