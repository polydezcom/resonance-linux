import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts
import Resonance

ApplicationWindow {
    id: root

    width: 1180
    height: 760
    minimumWidth: 960
    minimumHeight: 600
    visible: true
    title: qsTr("Resonance")
    color: Theme.background

    property bool toolsOpen: false

    function toggleTools(open) {
        toolsOpen = open
        AppSettings.toolsOpen = open
    }

    Binding {
        target: Theme
        property: "dark"
        value: AppSettings.dark
    }

    Binding {
        target: Theme
        property: "accent"
        value: AppSettings.accent
    }

    background: Rectangle {
        color: Theme.background
    }

    FolderDialog {
        id: folderDialog
        title: qsTr("Choose a music folder")
        onAccepted: Library.openFolder(selectedFolder)
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        HeaderBar {
            Layout.fillWidth: true
            title: Library.folder.length > 0 ? Library.folderName : qsTr("Your music")
            detail: Library.folder.length > 0
                    ? Library.folder
                    : qsTr("Folder-first, fully offline, no accounts.")
            scanning: Library.scanning
            toolsOpen: root.toolsOpen
            onOpenFolderRequested: folderDialog.open()
            onToggleToolsRequested: root.toggleTools(!root.toolsOpen)
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Sidebar {
                Layout.fillHeight: true
                onOpenFolderRequested: folderDialog.open()
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 12

                    TrackListPane {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        onOpenFolderRequested: folderDialog.open()
                    }

                    Item {
                        id: toolsHost
                        property real targetWidth: root.toolsOpen ? Theme.rightPanelWidth : 0
                        Layout.preferredWidth: targetWidth
                        Layout.fillHeight: true
                        clip: true
                        visible: targetWidth > 0.5

                        Behavior on targetWidth {
                            NumberAnimation {
                                duration: Theme.anim
                                easing.type: Easing.OutCubic
                            }
                        }

                        NowPlayingPanel {
                            width: Theme.rightPanelWidth
                            height: toolsHost.height
                            onCloseRequested: root.toggleTools(false)
                        }
                    }
                }
            }
        }

        TransportBar {
            Layout.fillWidth: true
        }
    }

    Shortcut {
        sequence: StandardKey.Open
        onActivated: folderDialog.open()
    }

    Shortcut {
        sequence: "Space"
        enabled: !folderDialog.visible
        onActivated: {
            if (Player.hasTrack)
                Player.togglePlayPause()
        }
    }

    Shortcut {
        sequence: "Ctrl+Left"
        onActivated: Player.seek(Player.positionMs - 5000)
    }

    Shortcut {
        sequence: "Ctrl+Right"
        onActivated: Player.seek(Player.positionMs + 5000)
    }

    Shortcut {
        sequence: "Ctrl+Up"
        onActivated: Player.volume = Math.min(1, Player.volume + 0.05)
    }

    Shortcut {
        sequence: "Ctrl+Down"
        onActivated: Player.volume = Math.max(0, Player.volume - 0.05)
    }

    Shortcut {
        sequence: "F5"
        onActivated: Library.refresh()
    }

    Shortcut {
        sequence: "Alt+Left"
        onActivated: Player.previous()
    }

    Shortcut {
        sequence: "Alt+Right"
        onActivated: Player.next()
    }

    Shortcut {
        sequence: "Ctrl+Alt+Left"
        onActivated: Library.previousFolder()
    }

    Shortcut {
        sequence: "Ctrl+Alt+Right"
        onActivated: Library.nextFolder()
    }

    Shortcut {
        sequence: "S"
        onActivated: Player.toggleShuffle()
    }

    Shortcut {
        sequence: "R"
        onActivated: Player.cycleRepeat()
    }

    Shortcut {
        sequence: "Ctrl+T"
        onActivated: root.toggleTools(!root.toolsOpen)
    }

    Shortcut {
        sequence: "Ctrl+Q"
        onActivated: Qt.quit()
    }

    DropArea {
        id: dropArea
        anchors.fill: parent
        z: 101

        onEntered: function(drag) {
            dropOverlay.visible = true
            drag.acceptProposedAction()
        }

        onExited: dropOverlay.visible = false

        onDropped: function(drop) {
            dropOverlay.visible = false
            Library.openUrls(drop.urls)
            drop.acceptProposedAction()
        }
    }

    Rectangle {
        id: dropOverlay
        anchors.fill: parent
        visible: false
        z: 100
        color: Qt.rgba(Theme.background.r, Theme.background.g,
                       Theme.background.b, 0.78)

        Rectangle {
            anchors.centerIn: parent
            width: Math.min(420, parent.width - 64)
            height: 190
            radius: 22
            color: Theme.surfaceElevated
            border.width: 2
            border.color: Theme.accent

            ColumnLayout {
                anchors.centerIn: parent
                width: parent.width - 48
                spacing: 10

                Icon {
                    Layout.alignment: Qt.AlignHCenter
                    name: "folder-open"
                    size: 42
                    color: Theme.accent
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Drop to play")
                    color: Theme.text
                    font.family: Theme.fontFamily
                    font.pixelSize: 20
                    font.weight: Font.DemiBold
                    horizontalAlignment: Text.AlignHCenter
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Folders are scanned recursively. Tracks play in natural order.")
                    color: Theme.textMuted
                    font.family: Theme.fontFamily
                    font.pixelSize: 12
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                }
            }
        }
    }

    Component.onCompleted: {
        root.toolsOpen = AppSettings.toolsOpen
        if (Library.startupFolder.length > 0)
            Library.openFolderPath(Library.startupFolder, Library.startupAutoPlay)
        else
            Library.restoreLastSession()
    }
}
