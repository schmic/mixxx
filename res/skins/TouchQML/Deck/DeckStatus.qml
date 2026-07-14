import "../Theme"
import Mixxx 1.0 as Mixxx
import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root

    required property color accentColor
    readonly property var currentTrack: player?.currentTrack
    required property int deckNumber
    readonly property string displayKey: root.loaded ? Mixxx.KeyUtils.keyToString(keyControl.value, keyNotationControl.value) : "--"
    readonly property string displayTitle: {
        if (!root.loaded) {
            return qsTr("No track loaded");
        }
        const title = root.currentTrack?.title || qsTr("Unknown title");
        const artist = root.currentTrack?.artist || "";
        return artist.length > 0 ? title + "  —  " + artist : title;
    }
    required property string group
    readonly property bool loaded: player?.isLoaded ?? false
    readonly property var player: Mixxx.PlayerManager.getPlayer(root.group)
    readonly property int syncModeExplicitLeader: 3
    required property string syncPartnerGroup

    function beatSizeText(value) {
        if (!root.loaded || value <= 0) {
            return "--";
        }
        if (value < 1) {
            return "1/" + Math.round(1 / value);
        }
        return Math.abs(value - Math.round(value)) < 0.001 ? Math.round(value).toString() : value.toFixed(1);
    }
    function remainingTimeText() {
        if (!root.loaded || durationControl.value <= 0) {
            return "--:--.-";
        }
        const remaining = Math.max(0, durationControl.value * (1 - playPositionControl.value));
        const minutes = Math.floor(remaining / 60);
        const seconds = Math.floor(remaining % 60);
        const tenths = Math.floor((remaining - Math.floor(remaining)) * 10);
        return "-" + minutes.toString().padStart(2, "0") + ":" + seconds.toString().padStart(2, "0") + "." + tenths;
    }

    color: TouchTheme.deckStatusBackground
    height: TouchTheme.deckStatusHeight

    Mixxx.ControlProxy {
        id: bpmControl

        group: root.group
        key: "bpm"
    }
    Mixxx.ControlProxy {
        id: rateRatioControl

        group: root.group
        key: "rate_ratio"
    }
    Mixxx.ControlProxy {
        id: rateRangeControl

        group: root.group
        key: "rateRange"
    }
    Mixxx.ControlProxy {
        id: keyControl

        group: root.group
        key: "key"
    }
    Mixxx.ControlProxy {
        id: keyNotationControl

        group: "[Library]"
        key: "key_notation"
    }
    Mixxx.ControlProxy {
        id: durationControl

        group: root.group
        key: "duration"
    }
    Mixxx.ControlProxy {
        id: playPositionControl

        group: root.group
        key: "playposition"
    }
    Mixxx.ControlProxy {
        id: loopEnabledControl

        group: root.group
        key: "loop_enabled"
    }
    Mixxx.ControlProxy {
        id: reloopControl

        group: root.group
        key: "reloop_toggle"
    }
    Mixxx.ControlProxy {
        id: beatloopSizeControl

        group: root.group
        key: "beatloop_size"
    }
    Mixxx.ControlProxy {
        id: beatjumpSizeControl

        group: root.group
        key: "beatjump_size"
    }
    Mixxx.ControlProxy {
        id: beatjumpForwardControl

        group: root.group
        key: "beatjump_forward"
    }
    Mixxx.ControlProxy {
        id: syncEnabledControl

        group: root.group
        key: "sync_enabled"
    }
    Mixxx.ControlProxy {
        id: beatSyncControl

        group: root.group
        key: "beatsync"
    }
    Mixxx.ControlProxy {
        id: syncLeaderControl

        group: root.group
        key: "sync_leader"
    }
    Mixxx.ControlProxy {
        id: syncModeControl

        group: root.group
        key: "sync_mode"
    }
    Mixxx.ControlProxy {
        id: partnerSyncLeaderControl

        group: root.syncPartnerGroup
        key: "sync_leader"
    }
    Item {
        id: upperRow

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: TouchTheme.deckStatusRowHeight

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 8
            spacing: 0

            Text {
                Layout.preferredWidth: 34
                color: root.accentColor
                font.family: TouchTheme.fontFamily
                font.pixelSize: 24
                font.weight: Font.DemiBold
                horizontalAlignment: Text.AlignHCenter
                text: root.deckNumber
                verticalAlignment: Text.AlignVCenter
            }
            Text {
                Layout.fillWidth: true
                color: root.loaded ? TouchTheme.primaryText : TouchTheme.mutedText
                elide: Text.ElideRight
                font.family: TouchTheme.fontFamily
                font.pixelSize: 18
                font.weight: Font.DemiBold
                text: root.displayTitle
                verticalAlignment: Text.AlignVCenter
            }
            Item {
                Layout.preferredHeight: TouchTheme.minimumTouchSize
                Layout.preferredWidth: 94
                opacity: !root.loaded ? 0.45 : loopTapHandler.pressed ? 0.62 : 1.0

                Rectangle {
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.top: parent.top
                    color: TouchTheme.border
                    width: 1
                }
                Row {
                    anchors.centerIn: parent
                    spacing: 8

                    Image {
                        antialiasing: true
                        fillMode: Image.PreserveAspectFit
                        height: TouchTheme.deckStatusIconSize
                        mipmap: true
                        source: loopEnabledControl.value > 0 ? Qt.resolvedUrl("../Icons/loop-active.svg") : Qt.resolvedUrl("../Icons/loop.svg")
                        sourceSize.height: TouchTheme.deckStatusIconSize
                        sourceSize.width: TouchTheme.deckStatusIconSize
                        width: TouchTheme.deckStatusIconSize
                    }
                    Text {
                        color: loopEnabledControl.value > 0 ? TouchTheme.activeLoop : TouchTheme.primaryText
                        font.family: TouchTheme.fontFamily
                        font.pixelSize: 18
                        font.weight: Font.DemiBold
                        height: TouchTheme.deckStatusIconSize
                        text: root.beatSizeText(beatloopSizeControl.value)
                        verticalAlignment: Text.AlignVCenter
                    }
                }
                TapHandler {
                    id: loopTapHandler

                    enabled: root.loaded

                    onTapped: reloopControl.trigger()
                }
            }
            Item {
                Layout.preferredHeight: TouchTheme.minimumTouchSize
                Layout.preferredWidth: 78
                opacity: !root.loaded ? 0.45 : beatjumpTapHandler.pressed ? 0.62 : 1.0

                Rectangle {
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.top: parent.top
                    color: TouchTheme.border
                    width: 1
                }
                Row {
                    anchors.centerIn: parent
                    spacing: 8

                    Image {
                        antialiasing: true
                        fillMode: Image.PreserveAspectFit
                        height: TouchTheme.deckStatusIconSize
                        mipmap: true
                        source: Qt.resolvedUrl("../Icons/beatjump.svg")
                        sourceSize.height: TouchTheme.deckStatusIconSize
                        sourceSize.width: TouchTheme.deckStatusIconSize
                        width: TouchTheme.deckStatusIconSize
                    }
                    Text {
                        color: TouchTheme.primaryText
                        font.family: TouchTheme.fontFamily
                        font.pixelSize: 18
                        font.weight: Font.DemiBold
                        height: TouchTheme.deckStatusIconSize
                        text: root.beatSizeText(beatjumpSizeControl.value)
                        verticalAlignment: Text.AlignVCenter
                    }
                }
                TapHandler {
                    id: beatjumpTapHandler

                    enabled: root.loaded

                    onTapped: beatjumpForwardControl.trigger()
                }
            }
        }
    }
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        color: TouchTheme.border
        height: 1
        y: TouchTheme.deckStatusRowHeight - 1
    }
    Item {
        id: lowerRow

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: TouchTheme.deckStatusRowHeight

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 8
            anchors.rightMargin: 8
            spacing: 5

            Item {
                id: syncAction

                property bool holdActionTriggered: false

                Layout.preferredHeight: TouchTheme.minimumTouchSize
                Layout.preferredWidth: 54

                Timer {
                    id: syncHoldTimer

                    interval: 2000
                    repeat: false

                    onTriggered: {
                        syncAction.holdActionTriggered = true;
                        if (partnerSyncLeaderControl.value > 0) {
                            syncEnabledControl.value = 1;
                        } else {
                            syncModeControl.value = root.syncModeExplicitLeader;
                        }
                    }
                }
                Rectangle {
                    anchors.centerIn: parent
                    border.color: syncLeaderControl.value > 0 ? TouchTheme.activeLeader : TouchTheme.border
                    border.width: 1
                    color: syncTapHandler.pressed ? TouchTheme.controlPressedBackground : TouchTheme.controlBackground
                    height: 40
                    width: parent.width

                    Column {
                        anchors.centerIn: parent
                        spacing: 0

                        Text {
                            color: syncEnabledControl.value > 0 ? TouchTheme.activeSync : TouchTheme.secondaryText
                            font.family: TouchTheme.fontFamily
                            font.pixelSize: 12
                            font.weight: Font.Bold
                            horizontalAlignment: Text.AlignHCenter
                            text: qsTr("SYNC")
                            width: 50
                        }
                        Text {
                            color: syncLeaderControl.value > 0 ? TouchTheme.activeLeader : TouchTheme.mutedText
                            font.family: TouchTheme.fontFamily
                            font.pixelSize: 10
                            font.weight: Font.Bold
                            horizontalAlignment: Text.AlignHCenter
                            text: qsTr("LEAD")
                            width: 50
                        }
                    }
                }
                TapHandler {
                    id: syncTapHandler

                    enabled: root.loaded

                    onPressedChanged: {
                        if (pressed) {
                            syncAction.holdActionTriggered = false;
                            syncHoldTimer.restart();
                        } else {
                            syncHoldTimer.stop();
                        }
                    }
                    onTapped: {
                        syncHoldTimer.stop();
                        if (syncAction.holdActionTriggered) {
                            syncAction.holdActionTriggered = false;
                            return;
                        }
                        beatSyncControl.trigger();
                    }
                }
            }
            MetaValue {
                Layout.preferredWidth: 72
                caption: qsTr("PITCH")
                text: root.loaded ? ((rateRatioControl.value - 1) * 100).toFixed(2) + "%" : "--"
            }
            MetaValue {
                Layout.preferredWidth: 52
                caption: qsTr("RANGE")
                text: root.loaded ? "±" + (rateRangeControl.value * 100).toFixed(0) : "--"
            }
            MetaValue {
                Layout.preferredWidth: 52
                accent: true
                caption: qsTr("KEY")
                text: root.displayKey.length > 0 ? root.displayKey : "--"
            }
            MetaValue {
                Layout.preferredWidth: 66
                caption: qsTr("BPM")
                text: root.loaded && bpmControl.value > 0 ? bpmControl.value.toFixed(1) : "--.-"
            }
            MetaValue {
                Layout.preferredWidth: 88
                caption: qsTr("REMAIN")
                text: root.remainingTimeText()
            }
            Item {
                Layout.fillWidth: true
            }
        }
    }

    component MetaValue: Item {
        id: metaValue

        property bool accent: false
        required property string caption
        required property string text

        Layout.preferredHeight: TouchTheme.minimumTouchSize

        Column {
            anchors.centerIn: parent
            spacing: 0

            Text {
                color: TouchTheme.mutedText
                font.family: TouchTheme.fontFamily
                font.pixelSize: 10
                font.weight: Font.DemiBold
                horizontalAlignment: Text.AlignHCenter
                text: metaValue.caption
                width: metaValue.width
            }
            Text {
                color: metaValue.accent ? TouchTheme.keyText : TouchTheme.primaryText
                elide: Text.ElideRight
                font.family: TouchTheme.fontFamily
                font.pixelSize: 18
                font.weight: Font.DemiBold
                horizontalAlignment: Text.AlignHCenter
                text: metaValue.text
                width: metaValue.width
            }
        }
    }
}
