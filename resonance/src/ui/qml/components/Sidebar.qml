import Resonance
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts

Rectangle {
    id: sidebar

    signal openFolderRequested()

    implicitWidth: Theme.sidebarWidth
    color: Theme.sidebar

    Rectangle {
        width: 1
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        color: Theme.border
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 11

            Rectangle {
                Layout.preferredWidth: 36
                Layout.preferredHeight: 36
                radius: 11
                gradient: Gradient {
                    GradientStop { position: 0.0; color: Theme.accentHover }
                    GradientStop { position: 1.0; color: Qt.darker(Theme.accent, 1.18) }
                }

                Text {
                    anchors.centerIn: parent
                    text: "R"
                    color: Theme.onAccent
                    font.family: Theme.fontFamily
                    font.pixelSize: 19
                    font.weight: Font.Bold
                }
            }

            ColumnLayout {
                spacing: 0
                Text {
                    text: "Resonance"
                    color: Theme.text
                    font.family: Theme.fontFamily
                    font.pixelSize: 17
                    font.weight: Font.DemiBold
                }
                SectionLabel { text: "OFFLINE PLAYER" }
            }
        }

        Item { Layout.preferredHeight: 8 }

        ResonanceButton {
            Layout.fillWidth: true
            text: qsTr("Open folder")
            iconName: "folder-open"
            primary: true
            onClicked: sidebar.openFolderRequested()
            Accessible.name: qsTr("Open a music folder")
        }

        Item { Layout.preferredHeight: 6 }

        FolderTreePane {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 110
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 2

            SectionLabel {
                Layout.fillWidth: true
                text: "PLAYLISTS"
            }

            IconButton {
                iconName: "plus"
                iconSize: 13
                buttonSize: 26
                accessibleLabel: qsTr("New playlist")
                onClicked: playlistNamePopup.openCreate()
            }

            IconButton {
                iconName: "upload"
                iconSize: 13
                buttonSize: 26
                accessibleLabel: qsTr("Import M3U or PLS playlist")
                onClicked: importDialog.open()
            }
        }

        ListView {
            id: playlistList
            Layout.fillWidth: true
            Layout.preferredHeight: 118
            Layout.minimumHeight: 60
            clip: true
            spacing: 1
            model: Playlists.model
            boundsBehavior: Flickable.StopAtBounds

            ScrollBar.vertical: ScrollBar {
                id: playlistScrollBar
                policy: ScrollBar.AsNeeded
                contentItem: Rectangle {
                    implicitWidth: 5
                    radius: 2.5
                    color: playlistScrollBar.pressed ? Theme.accent : Theme.border
                    opacity: playlistScrollBar.active ? 1.0 : 0.35
                }
                background: Item {}
            }

            delegate: Rectangle {
                id: playlistRow

                required property int index
                required property string playlistId
                required property string name
                required property int trackCount
                required property bool current

                width: playlistList.width
                height: 29
                radius: 6
                color: current ? Theme.selected
                               : playlistMouse.containsMouse ? Theme.hover : "transparent"

                Behavior on color {
                    ColorAnimation { duration: Theme.animFast }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 7
                    anchors.rightMargin: 7
                    spacing: 7

                    Icon {
                        name: "list"
                        size: 13
                        color: playlistRow.current ? Theme.accent : Theme.textFaint
                    }

                    Text {
                        Layout.fillWidth: true
                        text: playlistRow.name
                        color: playlistRow.current ? Theme.accent : Theme.text
                        font.family: Theme.fontFamily
                        font.pixelSize: 12
                        font.weight: playlistRow.current ? Font.DemiBold : Font.Normal
                        elide: Text.ElideMiddle
                    }

                    Text {
                        text: playlistRow.trackCount
                        color: Theme.textFaint
                        font.family: Theme.monoFamily
                        font.pixelSize: 9
                    }
                }

                MouseArea {
                    id: playlistMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: Library.playPlaylist(playlistRow.playlistId)
                    Accessible.name: qsTr("Play playlist %1").arg(playlistRow.name)
                }

                Accessible.role: Accessible.ListItem
                Accessible.name: qsTr("Playlist %1, %2 tracks")
                                 .arg(playlistRow.name)
                                 .arg(playlistRow.trackCount)
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 2
            visible: Playlists.hasCurrent

            IconButton {
                iconName: "folder-plus"
                iconSize: 14
                buttonSize: 28
                accessibleLabel: qsTr("Add a folder to this playlist")
                onClicked: addFolderDialog.open()
            }

            IconButton {
                iconName: "pencil"
                iconSize: 14
                buttonSize: 28
                accessibleLabel: qsTr("Rename this playlist")
                onClicked: playlistNamePopup.openRename(Playlists.currentId,
                                                        Playlists.currentName)
            }

            IconButton {
                iconName: "download"
                iconSize: 14
                buttonSize: 28
                accessibleLabel: qsTr("Export this playlist")
                onClicked: exportDialog.open()
            }

            Item { Layout.fillWidth: true }

            IconButton {
                iconName: "trash"
                iconSize: 14
                buttonSize: 28
                accessibleLabel: qsTr("Delete this playlist")
                onClicked: deletePopup.open()
            }
        }

        Text {
            Layout.fillWidth: true
            visible: Playlists.error.length > 0
            text: Playlists.error
            color: Theme.danger
            font.family: Theme.fontFamily
            font.pixelSize: 10
            wrapMode: Text.WordWrap
            maximumLineCount: 3
            elide: Text.ElideRight

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: Playlists.clearError()
                Accessible.name: qsTr("Dismiss playlist error")
            }
        }

        Text {
            Layout.fillWidth: true
            visible: Playlists.model.count === 0
            text: qsTr("No playlists yet")
            color: Theme.textFaint
            font.family: Theme.fontFamily
            font.pixelSize: 11
            horizontalAlignment: Text.AlignHCenter
        }

        SectionLabel { text: "ACCENT" }

        Row {
            Layout.fillWidth: true
            spacing: 9

            Repeater {
                model: ["#7C9CFF", "#4FD1C5", "#F6A65D",
                        "#E879F9", "#FF7A8A", "#A3E635"]

                delegate: Rectangle {
                    width: 21
                    height: 21
                    radius: 11
                    color: modelData
                    border.width: AppSettings.accent.toLowerCase() === modelData.toLowerCase() ? 2 : 0
                    border.color: Theme.text
                    scale: swatchMouse.containsMouse ? 1.12 : 1.0

                    Behavior on scale {
                        NumberAnimation { duration: Theme.animFast }
                    }

                    MouseArea {
                        id: swatchMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: AppSettings.accent = modelData
                        Accessible.name: qsTr("Use accent color %1").arg(modelData)
                    }
                }
            }
        }

        Item { Layout.preferredHeight: 2 }

        ResonanceButton {
            Layout.fillWidth: true
            ghost: true
            compact: true
            text: Theme.dark ? qsTr("Light theme") : qsTr("Dark theme")
            iconName: Theme.dark ? "sun" : "moon"
            onClicked: AppSettings.dark = !AppSettings.dark
            Accessible.name: text
        }

        Text {
            Layout.fillWidth: true
            text: "v0.3 · M3"
            color: Theme.textFaint
            font.family: Theme.monoFamily
            font.pixelSize: 10
            horizontalAlignment: Text.AlignHCenter
        }
    }

    PlaylistNamePopup { id: playlistNamePopup }

    FolderDialog {
        id: addFolderDialog
        title: qsTr("Add folder to playlist")
        onAccepted: Playlists.addUrls(Playlists.currentId, [selectedFolder])
    }

    FileDialog {
        id: importDialog
        title: qsTr("Import playlist")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Playlists (*.m3u *.m3u8 *.pls)")]
        onAccepted: Playlists.importPlaylist(selectedFile)
    }

    FileDialog {
        id: exportDialog
        title: qsTr("Export playlist")
        fileMode: FileDialog.SaveFile
        defaultSuffix: "m3u8"
        nameFilters: [
            qsTr("M3U8 playlist (*.m3u8)"),
            qsTr("M3U playlist (*.m3u)"),
            qsTr("PLS playlist (*.pls)"),
        ]
        onAccepted: Playlists.exportPlaylist(Playlists.currentId, selectedFile)
    }

    Popup {
        id: deletePopup

        parent: Overlay.overlay
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        width: 320
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        padding: 18

        background: Rectangle {
            color: Theme.surfaceElevated
            radius: Theme.radiusLarge
            border.width: 1
            border.color: Theme.borderStrong
        }

        contentItem: ColumnLayout {
            spacing: 12

            SectionLabel { text: qsTr("DELETE PLAYLIST") }

            Text {
                Layout.fillWidth: true
                text: qsTr("Delete “%1”? The audio files themselves are not touched.")
                      .arg(Playlists.currentName)
                color: Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: 12
                wrapMode: Text.WordWrap
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Item { Layout.fillWidth: true }
                ResonanceButton {
                    text: qsTr("Cancel")
                    ghost: true
                    compact: true
                    onClicked: deletePopup.close()
                }
                ResonanceButton {
                    text: qsTr("Delete")
                    primary: true
                    compact: true
                    onClicked: {
                        Playlists.deletePlaylist(Playlists.currentId)
                        deletePopup.close()
                    }
                }
            }
        }
    }

}
