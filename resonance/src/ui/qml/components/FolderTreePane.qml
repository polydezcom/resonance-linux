import Resonance
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic

Item {
    id: pane

    implicitHeight: 220

    ColumnLayout {
        anchors.fill: parent
        spacing: 4

        RowLayout {
            Layout.fillWidth: true
            spacing: 2

            SectionLabel {
                Layout.fillWidth: true
                text: "FOLDERS"
            }

            IconButton {
                iconName: "chevron-left"
                iconSize: 13
                buttonSize: 26
                enabled: Library.folders.count > 1
                accessibleLabel: qsTr("Previous folder")
                onClicked: Library.previousFolder()
            }

            IconButton {
                iconName: "chevron-right"
                iconSize: 13
                buttonSize: 26
                enabled: Library.folders.count > 1
                accessibleLabel: qsTr("Next folder")
                onClicked: Library.nextFolder()
            }
        }

        ListView {
            id: tree
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 1
            model: Library.folders
            currentIndex: -1
            focus: true
            boundsBehavior: Flickable.StopAtBounds

            ScrollBar.vertical: ScrollBar {
                id: treeScrollBar
                policy: ScrollBar.AsNeeded
                contentItem: Rectangle {
                    implicitWidth: 5
                    radius: 2.5
                    color: treeScrollBar.pressed ? Theme.accent : Theme.border
                    opacity: treeScrollBar.active ? 1.0 : 0.35
                }
                background: Item {}
            }

            Keys.onReturnPressed: {
                if (tree.currentIndex >= 0)
                    Library.playFolder(Library.folders.pathAt(tree.currentIndex))
            }
            Keys.onEnterPressed: {
                if (tree.currentIndex >= 0)
                    Library.playFolder(Library.folders.pathAt(tree.currentIndex))
            }

            delegate: Rectangle {
                id: folderRow

                required property int index
                required property string name
                required property string path
                required property int depth
                required property bool hasChildren
                required property bool expanded
                required property int trackCount
                required property bool current
                required property bool root

                width: tree.width
                height: 30
                radius: 6
                color: current ? Theme.selected
                               : rowMouse.containsMouse ? Theme.hover : "transparent"

                Behavior on color {
                    ColorAnimation { duration: Theme.animFast }
                }

                MouseArea {
                    id: rowMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        tree.currentIndex = folderRow.index
                        tree.forceActiveFocus()
                        Library.playFolder(folderRow.path)
                    }
                    Accessible.name: qsTr("Play folder %1").arg(folderRow.name)
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 5 + folderRow.depth * 14
                    anchors.rightMargin: 7
                    spacing: 4

                    Item {
                        Layout.preferredWidth: 17
                        Layout.preferredHeight: 17

                        Icon {
                            anchors.centerIn: parent
                            visible: folderRow.hasChildren
                            name: folderRow.expanded ? "chevron-down" : "chevron-right"
                            size: 12
                            color: Theme.textMuted
                        }

                        MouseArea {
                            anchors.fill: parent
                            enabled: folderRow.hasChildren
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                tree.currentIndex = folderRow.index
                                Library.folders.toggle(folderRow.index)
                            }
                            Accessible.name: folderRow.expanded
                                             ? qsTr("Collapse folder")
                                             : qsTr("Expand folder")
                        }
                    }

                    Icon {
                        name: folderRow.current ? "folder-open" : "folder"
                        size: 15
                        color: folderRow.current ? Theme.accent : Theme.textMuted
                    }

                    Text {
                        Layout.fillWidth: true
                        text: folderRow.name
                        color: folderRow.current ? Theme.accent : Theme.text
                        font.family: Theme.fontFamily
                        font.pixelSize: 12
                        font.weight: folderRow.current ? Font.DemiBold : Font.Normal
                        elide: Text.ElideMiddle
                    }

                    Text {
                        visible: folderRow.trackCount > 0
                        text: folderRow.trackCount
                        color: Theme.textFaint
                        font.family: Theme.monoFamily
                        font.pixelSize: 9
                    }
                }

                Accessible.role: Accessible.ListItem
                Accessible.name: qsTr("Folder %1, %2 tracks")
                                 .arg(folderRow.name)
                                 .arg(folderRow.trackCount)
            }
        }

        Text {
            Layout.fillWidth: true
            visible: Library.folders.count === 0
            text: Library.scanning ? qsTr("Scanning folders…")
                                   : qsTr("No folder open")
            color: Theme.textFaint
            font.family: Theme.fontFamily
            font.pixelSize: 11
            horizontalAlignment: Text.AlignHCenter
        }
    }

    Connections {
        target: Library
        function onFolderChanged() {
            const row = Library.folders.rowForPath(Library.folder)
            if (row >= 0) {
                tree.currentIndex = row
                tree.positionViewAtIndex(row, ListView.Contain)
            }
        }
    }
}
