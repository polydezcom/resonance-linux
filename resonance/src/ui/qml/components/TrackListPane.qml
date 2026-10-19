import Resonance
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic

Rectangle {
    id: pane

    signal openFolderRequested()

    color: Theme.surface
    radius: Theme.radiusLarge
    border.width: 1
    border.color: Theme.border

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                SectionLabel { text: "TRACKS" }
                Text {
                    Layout.fillWidth: true
                    text: Library.hasTracks ? Library.folderName : qsTr("Your music")
                    color: Theme.text
                    font.family: Theme.fontFamily
                    font.pixelSize: 21
                    font.weight: Font.DemiBold
                    elide: Text.ElideRight
                }
            }

            ColumnLayout {
                spacing: 1
                Layout.alignment: Qt.AlignVCenter
                Text {
                    Layout.alignment: Qt.AlignRight
                    text: Library.hasTracks ? qsTr("%1 tracks").arg(Library.trackCount)
                                            : qsTr("Empty")
                    color: Theme.textMuted
                    font.family: Theme.fontFamily
                    font.pixelSize: 12
                }
                Text {
                    Layout.alignment: Qt.AlignRight
                    text: qsTr("Natural order")
                    color: Theme.textFaint
                    font.family: Theme.fontFamily
                    font.pixelSize: 10
                }
            }

            IconButton {
                iconName: "refresh"
                iconSize: 17
                buttonSize: 32
                enabled: Library.folder.length > 0 && !Library.scanning
                accessibleLabel: qsTr("Rescan folder")
                onClicked: Library.refresh()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Theme.border
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 38
            radius: Theme.radiusSmall
            color: Qt.rgba(Theme.danger.r, Theme.danger.g, Theme.danger.b, 0.10)
            border.width: 1
            border.color: Qt.rgba(Theme.danger.r, Theme.danger.g, Theme.danger.b, 0.28)
            visible: Library.error.length > 0

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 11
                anchors.rightMargin: 6
                spacing: 8

                Icon { name: "alert"; size: 15; color: Theme.danger }
                Text {
                    Layout.fillWidth: true
                    text: Library.error
                    color: Theme.danger
                    font.family: Theme.fontFamily
                    font.pixelSize: 12
                    elide: Text.ElideRight
                }
                IconButton {
                    iconName: "x"
                    iconSize: 14
                    buttonSize: 26
                    iconColor: Theme.danger
                    accessibleLabel: qsTr("Dismiss error")
                    onClicked: Library.clearError()
                }
            }
        }

        ListView {
            id: trackList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: Library.tracks
            currentIndex: -1
            spacing: 2
            reuseItems: true
            focus: true
            boundsBehavior: Flickable.StopAtBounds

            ScrollBar.vertical: ScrollBar {
                id: verticalScrollBar
                policy: ScrollBar.AsNeeded
                contentItem: Rectangle {
                    implicitWidth: 6
                    radius: 3
                    color: verticalScrollBar.pressed ? Theme.accent : Theme.border
                    opacity: verticalScrollBar.active ? 1.0 : 0.35

                    Behavior on opacity {
                        NumberAnimation { duration: Theme.animFast }
                    }
                }
                background: Item {}
            }

            Keys.onReturnPressed: {
                if (trackList.currentIndex >= 0)
                    Library.playIndex(trackList.currentIndex)
            }
            Keys.onEnterPressed: {
                if (trackList.currentIndex >= 0)
                    Library.playIndex(trackList.currentIndex)
            }

            delegate: Rectangle {
                id: row

                required property int index
                required property string title
                required property string path
                required property string folder
                required property int durationMs

                readonly property bool isCurrent: Player.path === path
                readonly property bool isSelected: trackList.currentIndex === index

                width: trackList.width
                height: 54
                radius: Theme.radiusSmall
                color: isSelected ? Theme.selected
                                  : rowMouse.containsMouse ? Theme.hover : "transparent"

                Behavior on color {
                    ColorAnimation { duration: Theme.animFast }
                }

                Rectangle {
                    width: 3
                    height: 22
                    radius: 2
                    color: Theme.accent
                    visible: row.isCurrent
                    anchors.left: parent.left
                    anchors.leftMargin: 3
                    anchors.verticalCenter: parent.verticalCenter
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14
                    anchors.rightMargin: 14
                    spacing: 12

                    Text {
                        Layout.preferredWidth: 28
                        text: (row.index + 1) < 10 ? "0" + (row.index + 1)
                                                   : String(row.index + 1)
                        color: row.isCurrent ? Theme.accent : Theme.textFaint
                        font.family: Theme.monoFamily
                        font.pixelSize: 11
                        horizontalAlignment: Text.AlignRight
                    }

                    Rectangle {
                        Layout.preferredWidth: 34
                        Layout.preferredHeight: 34
                        radius: 9
                        color: row.isCurrent ? Theme.accentSoft : Theme.surfaceAlt

                        Icon {
                            anchors.centerIn: parent
                            name: row.isCurrent && Player.playing ? "volume-2" : "music"
                            size: 16
                            color: row.isCurrent ? Theme.accent : Theme.textMuted
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1

                        Text {
                            Layout.fillWidth: true
                            text: row.title
                            color: row.isCurrent ? Theme.accent : Theme.text
                            font.family: Theme.fontFamily
                            font.pixelSize: 13
                            font.weight: row.isCurrent ? Font.DemiBold : Font.Medium
                            elide: Text.ElideRight
                        }

                        Text {
                            Layout.fillWidth: true
                            text: row.folder.length > 0 ? row.folder : qsTr("This folder")
                            color: Theme.textFaint
                            font.family: Theme.fontFamily
                            font.pixelSize: 11
                            elide: Text.ElideRight
                        }
                    }

                    Text {
                        text: row.durationMs > 0 ? Player.formatTime(row.durationMs) : "—"
                        color: Theme.textMuted
                        font.family: Theme.monoFamily
                        font.pixelSize: 11
                    }
                }

                MouseArea {
                    id: rowMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        trackList.currentIndex = row.index
                        trackList.forceActiveFocus()
                    }
                    onDoubleClicked: Library.playIndex(row.index)
                    Accessible.name: row.title
                    Accessible.role: Accessible.ListItem
                    Accessible.onPressAction: Library.playIndex(row.index)
                }

                Accessible.role: Accessible.ListItem
                Accessible.name: row.title
            }

            add: Transition {
                NumberAnimation {
                    properties: "opacity"
                    from: 0
                    to: 1
                    duration: Theme.anim
                }
            }
        }

        Connections {
            target: Player
            function onTrackChanged() {
                const index = Library.tracks.indexOfPath(Player.path)
                if (index >= 0) {
                    trackList.currentIndex = index
                    trackList.positionViewAtIndex(index, ListView.Contain)
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 14
            visible: Library.hasTracks

            Text {
                text: qsTr("Enter to play")
                color: Theme.textFaint
                font.family: Theme.fontFamily
                font.pixelSize: 10
            }
            Text {
                text: qsTr("Space to pause")
                color: Theme.textFaint
                font.family: Theme.fontFamily
                font.pixelSize: 10
            }
            Item { Layout.fillWidth: true }
            Text {
                text: qsTr("Drag files or folders anywhere")
                color: Theme.textFaint
                font.family: Theme.fontFamily
                font.pixelSize: 10
            }
        }
    }

    Item {
        id: emptyState
        anchors.fill: parent
        anchors.topMargin: 96
        anchors.bottomMargin: 44
        visible: Library.trackCount === 0 && !Library.scanning
        z: 2

        ColumnLayout {
            anchors.centerIn: parent
            width: Math.min(360, parent.width - 40)
            spacing: 10

            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 58
                Layout.preferredHeight: 58
                radius: 17
                color: Theme.accentSoft

                Icon {
                    anchors.centerIn: parent
                    name: "folder-open"
                    size: 26
                    color: Theme.accent
                }
            }

            Text {
                Layout.fillWidth: true
                text: qsTr("Nothing loaded yet")
                color: Theme.text
                font.family: Theme.fontFamily
                font.pixelSize: 17
                font.weight: Font.DemiBold
                horizontalAlignment: Text.AlignHCenter
            }

            Text {
                Layout.fillWidth: true
                text: qsTr("Open a folder or drop music anywhere in this window.")
                color: Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: 12
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }

            ResonanceButton {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 4
                text: qsTr("Choose folder")
                iconName: "folder-open"
                primary: true
                onClicked: pane.openFolderRequested()
            }
        }
    }

    Item {
        id: scanningState
        anchors.fill: parent
        anchors.topMargin: 96
        anchors.bottomMargin: 44
        visible: Library.trackCount === 0 && Library.scanning
        z: 2

        ColumnLayout {
            anchors.centerIn: parent
            spacing: 12

            Icon {
                Layout.alignment: Qt.AlignHCenter
                name: "refresh"
                size: 30
                color: Theme.accent

                RotationAnimator on rotation {
                    from: 0
                    to: 360
                    duration: 1100
                    loops: Animation.Infinite
                    running: Library.scanning
                }
            }

            Text {
                text: qsTr("Scanning your folder…")
                color: Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: 13
            }
        }
    }
}
