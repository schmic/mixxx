pragma ComponentBehavior: Bound

import "../Theme"
import Mixxx 1.0 as Mixxx
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    color: TouchTheme.background

    Flickable {
        id: settingsFlickable

        anchors.fill: parent
        boundsBehavior: Flickable.StopAtBounds
        clip: true
        contentHeight: settingsLayout.height + 32
        contentWidth: width
        flickableDirection: Flickable.VerticalFlick

        ScrollBar.vertical: ScrollBar {}

        ColumnLayout {
            id: settingsLayout

            x: 16
            y: 16
            width: settingsFlickable.width - 32
            height: Math.max(implicitHeight, settingsFlickable.height - 32)
            spacing: 16

            RowLayout {
                Layout.fillWidth: true
                spacing: 16

                Item {
                    Layout.fillWidth: true
                }
                SettingButton {
                    Layout.preferredWidth: 160
                    label: qsTr("Preferences")
                    Accessible.role: Accessible.Button
                    Accessible.name: label

                    onTriggered: Mixxx.PreferencesDialog.show()
                }
                SettingButton {
                    Layout.preferredWidth: 160
                    label: qsTr("Quit")
                    Accessible.role: Accessible.Button
                    Accessible.name: label

                    onTriggered: Qt.quit()
                }
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 16

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    Text {
                        Layout.fillWidth: true
                        color: TouchTheme.primaryText
                        font.family: TouchTheme.fontFamily
                        font.pixelSize: 20
                        text: qsTr("Start in fullscreen")
                    }
                    Text {
                        Layout.fillWidth: true
                        color: TouchTheme.secondaryText
                        font.family: TouchTheme.fontFamily
                        font.pixelSize: 16
                        text: qsTr("Applies the next time Mixxx starts.")
                        wrapMode: Text.WordWrap
                    }
                }
                SettingButton {
                    Layout.preferredWidth: 112
                    active: Mixxx.Config.configStartInFullscreenKey
                    label: active ? qsTr("On") : qsTr("Off")

                    onTriggered: Mixxx.Config.configStartInFullscreenKey = !Mixxx.Config.configStartInFullscreenKey
                }
            }
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: zoomSettingsLayout.implicitHeight + 32
                color: TouchTheme.libraryHeaderBackground
                border.color: TouchTheme.border
                border.width: 1

                ColumnLayout {
                    id: zoomSettingsLayout

                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 12

                    Text {
                        Layout.fillWidth: true
                        color: TouchTheme.primaryText
                        font.family: TouchTheme.fontFamily
                        font.pixelSize: 20
                        text: qsTr("Waveform zoom")
                    }
                    WaveformZoomSetting {
                        Layout.fillWidth: true
                        accent: TouchTheme.deck1Accent
                        group: "[Channel1]"
                        label: Mixxx.Config.waveformZoomSynchronization ? qsTr("Both decks") : qsTr("Deck 1")
                    }
                    WaveformZoomSetting {
                        Layout.fillWidth: true
                        accent: TouchTheme.deck2Accent
                        group: "[Channel2]"
                        label: qsTr("Deck 2")
                        visible: !Mixxx.Config.waveformZoomSynchronization
                    }
                }
            }
            RowLayout {
                Layout.fillHeight: true
                Layout.fillWidth: true
                spacing: 16

                VinylDeckSettings {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    accent: TouchTheme.deck1Accent
                    deckName: qsTr("Deck 1")
                    group: "[Channel1]"
                }
                VinylDeckSettings {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    accent: TouchTheme.deck2Accent
                    deckName: qsTr("Deck 2")
                    group: "[Channel2]"
                }
            }
        }
    }

    component WaveformZoomSetting: ColumnLayout {
        id: zoomSetting

        required property color accent
        required property string group
        required property string label

        spacing: 4

        Mixxx.ControlProxy {
            id: waveformZoomControl

            group: zoomSetting.group
            key: "waveform_zoom"
        }
        Text {
            Layout.fillWidth: true
            color: zoomSetting.accent
            font.family: TouchTheme.fontFamily
            font.pixelSize: 16
            text: zoomSetting.label
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            Text {
                color: TouchTheme.secondaryText
                font.family: TouchTheme.fontFamily
                font.pixelSize: 16
                text: qsTr("Zoom out")
            }
            Slider {
                id: zoomSlider

                Layout.fillWidth: true
                Layout.preferredHeight: TouchTheme.minimumTouchSize
                from: 1
                to: 10
                stepSize: 1
                snapMode: Slider.SnapAlways
                value: 11 - waveformZoomControl.value
                Accessible.name: qsTr("Waveform zoom — %1").arg(zoomSetting.label)

                onMoved: waveformZoomControl.value = 11 - value

                background: Rectangle {
                    x: zoomSlider.leftPadding
                    y: zoomSlider.topPadding + (zoomSlider.availableHeight - height) / 2
                    width: zoomSlider.availableWidth
                    height: 6
                    color: TouchTheme.controlBackground

                    Rectangle {
                        width: zoomSlider.visualPosition * parent.width
                        height: parent.height
                        color: zoomSetting.accent
                    }
                }
                handle: Rectangle {
                    x: zoomSlider.leftPadding + zoomSlider.visualPosition * (zoomSlider.availableWidth - width)
                    y: zoomSlider.topPadding + (zoomSlider.availableHeight - height) / 2
                    width: TouchTheme.minimumTouchSize
                    height: TouchTheme.minimumTouchSize
                    color: zoomSlider.pressed ? TouchTheme.controlPressedBackground : TouchTheme.controlBackground
                    border.color: zoomSetting.accent
                    border.width: zoomSlider.activeFocus ? 2 : 1
                }
            }
            Text {
                color: TouchTheme.secondaryText
                font.family: TouchTheme.fontFamily
                font.pixelSize: 16
                text: qsTr("Zoom in")
            }
        }
    }

    component VinylDeckSettings: Rectangle {
        id: deckSettings

        required property color accent
        required property string deckName
        required property string group

        implicitHeight: deckSettingsLayout.implicitHeight + 32

        color: TouchTheme.libraryHeaderBackground
        border.color: deckSettings.accent
        border.width: 1

        Mixxx.ControlProxy {
            id: vinylEnabledControl

            group: deckSettings.group
            key: "vinylcontrol_enabled"
        }
        Mixxx.ControlProxy {
            id: vinylModeControl

            group: deckSettings.group
            key: "vinylcontrol_mode"
        }
        Mixxx.ControlProxy {
            id: vinylCueingControl

            group: deckSettings.group
            key: "vinylcontrol_cueing"
        }

        ColumnLayout {
            id: deckSettingsLayout

            anchors.fill: parent
            anchors.margins: 16
            spacing: 12

            Text {
                Layout.fillWidth: true
                color: TouchTheme.primaryText
                font.family: TouchTheme.fontFamily
                font.pixelSize: 22
                font.weight: Font.DemiBold
                text: deckSettings.deckName
            }
            Text {
                Layout.fillWidth: true
                color: TouchTheme.secondaryText
                font.family: TouchTheme.fontFamily
                font.pixelSize: 16
                text: qsTr("Vinyl Control")
            }
            SettingButton {
                Layout.fillWidth: true
                active: vinylEnabledControl.value > 0
                label: qsTr("Vinyl Control")

                onTriggered: vinylEnabledControl.parameter = vinylEnabledControl.value > 0 ? 0 : 1
            }
            Text {
                Layout.fillWidth: true
                color: TouchTheme.secondaryText
                font.family: TouchTheme.fontFamily
                font.pixelSize: 16
                text: qsTr("Tracking Mode")
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 4

                SettingButton {
                    Layout.fillWidth: true
                    active: vinylModeControl.value === 0
                    label: qsTr("ABS")

                    onTriggered: vinylModeControl.parameter = 0
                }
                SettingButton {
                    Layout.fillWidth: true
                    active: vinylModeControl.value === 1
                    label: qsTr("REL")

                    onTriggered: vinylModeControl.parameter = 1
                }
                SettingButton {
                    Layout.fillWidth: true
                    active: vinylModeControl.value === 2
                    label: qsTr("CONST")

                    onTriggered: vinylModeControl.parameter = 2
                }
            }
            Text {
                Layout.fillWidth: true
                color: TouchTheme.secondaryText
                font.family: TouchTheme.fontFamily
                font.pixelSize: 16
                text: qsTr("Relative Cueing")
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 4

                SettingButton {
                    Layout.fillWidth: true
                    active: vinylCueingControl.value === 0
                    label: qsTr("OFF")

                    onTriggered: vinylCueingControl.parameter = 0
                }
                SettingButton {
                    Layout.fillWidth: true
                    active: vinylCueingControl.value === 1
                    label: qsTr("ONE")

                    onTriggered: vinylCueingControl.parameter = 1
                }
                SettingButton {
                    Layout.fillWidth: true
                    active: vinylCueingControl.value === 2
                    label: qsTr("HOT")

                    onTriggered: vinylCueingControl.parameter = 2
                }
            }
            Item {
                Layout.fillHeight: true
            }
        }
    }

    component SettingButton: Rectangle {
        id: button

        property bool active: false
        required property string label

        signal triggered

        color: tapHandler.pressed || button.active ? TouchTheme.controlPressedBackground : TouchTheme.controlBackground
        implicitHeight: TouchTheme.minimumTouchSize

        Text {
            anchors.centerIn: parent
            color: button.active ? TouchTheme.primaryText : TouchTheme.secondaryText
            font.family: TouchTheme.fontFamily
            font.pixelSize: 16
            font.weight: Font.DemiBold
            text: button.label
        }
        TapHandler {
            id: tapHandler

            onTapped: button.triggered()
        }
    }
}
