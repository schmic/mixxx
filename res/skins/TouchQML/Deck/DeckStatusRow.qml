import "../Theme"
import QtQuick

Item {
    id: root

    required property real splitX

    height: TouchTheme.deckStatusHeight

    DeckStatus {
        accentColor: TouchTheme.deck1Accent
        deckNumber: 1
        group: "[Channel1]"
        height: parent.height
        syncPartnerGroup: "[Channel2]"
        width: root.splitX
        x: 0
    }
    DeckStatus {
        accentColor: TouchTheme.deck2Accent
        deckNumber: 2
        group: "[Channel2]"
        height: parent.height
        syncPartnerGroup: "[Channel1]"
        width: root.width - root.splitX
        x: root.splitX
    }
    Rectangle {
        anchors.bottom: parent.bottom
        anchors.top: parent.top
        color: TouchTheme.centerDivider
        width: TouchTheme.centerDividerWidth
        x: root.splitX - width / 2
        z: 2
    }
}
