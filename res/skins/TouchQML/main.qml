import "Deck" as Deck
import "Library" as Library
import "Theme"
import Mixxx 1.0 as Mixxx
import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: root

    readonly property real deckSplitX: width / 2

    color: TouchTheme.background
    height: 1080
    minimumHeight: 600
    minimumWidth: 1024
    title: qsTr("Touch QML")
    visible: true
    width: 1920

    Shortcut {
        context: Qt.ApplicationShortcut
        sequence: "Ctrl+P"

        onActivated: Mixxx.PreferencesDialog.show()
    }
    Shortcut {
        context: Qt.ApplicationShortcut
        sequence: "Ctrl+Q"

        onActivated: Qt.quit()
    }
    Mixxx.SkinControlCreator {
        buttonMode: Mixxx.SkinControlCreator.Toggle
        defaultValue: 1
        group: "[Skin]"
        key: "show_intro_outro_cues"
        persist: true
    }
    Mixxx.ControlProxy {
        id: libraryViewControl

        group: "[Skin]"
        key: "show_maximized_library"
    }
    Column {
        anchors.fill: parent
        spacing: 0

        NavigationBar {
            splitX: root.deckSplitX
            width: parent.width
        }
        Deck.DeckStatusRow {
            splitX: root.deckSplitX
            width: parent.width
        }
        Deck.DeckOverviewRow {
            splitX: root.deckSplitX
            width: parent.width
        }
        Item {
            height: Math.max(0, root.height - TouchTheme.topStackHeight)
            width: parent.width

            Rectangle {
                anchors.fill: parent
                color: TouchTheme.background
            }
            Library.LibraryView {
                anchors.fill: parent
                visible: libraryViewControl.value > 0
            }
        }
    }
}
