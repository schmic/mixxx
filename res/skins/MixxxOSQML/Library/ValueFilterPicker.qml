pragma ComponentBehavior: Bound

import "../Theme"
import QtQuick
import QtQuick.Controls

Popup {
    id: root

    required property var values
    required property string selectedValue
    required property string title
    required property string allLabel

    signal valueSelected(string value)

    parent: Overlay.overlay
    x: Math.round((parent.width - width) / 2)
    y: Math.round((parent.height - height) / 2)
    width: Math.min(400, parent.width - 32)
    height: Math.min(520, 56 + (root.values.length + 1) * TouchTheme.minimumTouchSize, parent.height - 32)
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnReleaseOutside
    focus: true
    modal: true
    padding: 0

    Overlay.modal: Rectangle {
        color: "#80000000"

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.AllButtons
            preventStealing: true

            onClicked: root.close()
            onWheel: wheel => wheel.accepted = true
        }
    }
    background: Rectangle {
        border.color: TouchTheme.deck1Accent
        border.width: 1
        color: TouchTheme.libraryBackground
    }
    contentItem: Item {
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.AllButtons

            onWheel: wheel => wheel.accepted = true
        }

        Rectangle {
            id: pickerHeader

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            color: TouchTheme.libraryHeaderBackground
            height: 56

            Text {
                anchors.left: parent.left
                anchors.leftMargin: 16
                anchors.right: closePicker.left
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                color: TouchTheme.primaryText
                elide: Text.ElideRight
                font.family: TouchTheme.fontFamily
                font.pixelSize: 16
                font.weight: Font.DemiBold
                text: root.title
            }
            Rectangle {
                id: closePicker

                anchors.right: parent.right
                anchors.top: parent.top
                color: closePickerTap.pressed ? TouchTheme.controlPressedBackground : "transparent"
                height: parent.height
                width: 56

                Text {
                    anchors.centerIn: parent
                    color: TouchTheme.secondaryText
                    font.family: TouchTheme.fontFamily
                    font.pixelSize: 22
                    text: "x"
                }
                TapHandler {
                    id: closePickerTap

                    gesturePolicy: TapHandler.WithinBounds

                    onTapped: root.close()
                }
            }
            Rectangle {
                anchors.bottom: parent.bottom
                color: TouchTheme.border
                height: 1
                width: parent.width
            }
        }
        ListView {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: pickerHeader.bottom
            clip: true
            model: [""].concat(root.values)

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }

            delegate: Rectangle {
                id: valueDelegate

                required property int index
                required property string modelData
                readonly property bool selected: root.selectedValue.toLocaleLowerCase() === modelData.toLocaleLowerCase()

                color: selected ? TouchTheme.libraryRowSelectedBackground : valueDelegateTap.pressed ? TouchTheme.controlPressedBackground : index % 2 === 0 ? TouchTheme.libraryRowBackground : TouchTheme.libraryRowAlternateBackground
                height: TouchTheme.minimumTouchSize
                width: ListView.view.width

                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: 16
                    anchors.right: parent.right
                    anchors.rightMargin: 16
                    anchors.verticalCenter: parent.verticalCenter
                    color: valueDelegate.selected ? TouchTheme.deck1Accent : TouchTheme.primaryText
                    elide: Text.ElideRight
                    font.family: TouchTheme.fontFamily
                    font.pixelSize: 15
                    font.weight: valueDelegate.selected ? Font.DemiBold : Font.Normal
                    text: valueDelegate.modelData.length > 0 ? valueDelegate.modelData : root.allLabel
                }
                Rectangle {
                    anchors.bottom: parent.bottom
                    color: TouchTheme.border
                    height: 1
                    width: parent.width
                }
                TapHandler {
                    id: valueDelegateTap

                    gesturePolicy: TapHandler.WithinBounds

                    onTapped: {
                        root.valueSelected(valueDelegate.modelData);
                        root.close();
                    }
                }
            }
        }
    }
}
