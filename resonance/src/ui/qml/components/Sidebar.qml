import Resonance
import QtQuick
import QtQuick.Layouts

Rectangle {
    id: sidebar

    signal openFolderRequested()
    signal playFolderRequested()

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
        SectionLabel { text: "LIBRARY" }

        Rectangle {
            id: folderCard
            Layout.fillWidth: true
            Layout.preferredHeight: 58
            radius: Theme.radius
            color: folderMouse.containsMouse && Library.hasTracks
                   ? Theme.hover : Theme.selected
            border.width: 1
            border.color: Theme.border

            Behavior on color {
                ColorAnimation { duration: Theme.animFast }
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 10
                spacing: 10

                Icon {
                    name: Library.scanning ? "refresh" : "folder-open"
                    size: 18
                    color: Theme.accent
                    Layout.alignment: Qt.AlignVCenter
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 1

                    Text {
                        Layout.fillWidth: true
                        text: Library.folderName
                        color: Theme.text
                        font.family: Theme.fontFamily
                        font.pixelSize: 13
                        font.weight: Font.Medium
                        elide: Text.ElideRight
                    }

                    Text {
                        Layout.fillWidth: true
                        text: Library.scanning
                              ? qsTr("Scanning…")
                              : Library.hasTracks
                                ? qsTr("%1 tracks").arg(Library.trackCount)
                                : qsTr("No tracks loaded")
                        color: Theme.textMuted
                        font.family: Theme.fontFamily
                        font.pixelSize: 11
                        elide: Text.ElideRight
                    }
                }

                Icon {
                    name: "play"
                    size: 13
                    color: Theme.accent
                    visible: Library.hasTracks && !Library.scanning
                    Layout.alignment: Qt.AlignVCenter
                }
            }

            MouseArea {
                id: folderMouse
                anchors.fill: parent
                hoverEnabled: true
                enabled: Library.hasTracks
                cursorShape: Library.hasTracks ? Qt.PointingHandCursor : Qt.ArrowCursor
                onClicked: sidebar.playFolderRequested()
                Accessible.name: qsTr("Play %1").arg(Library.folderName)
            }

            Accessible.role: Accessible.ListItem
            Accessible.name: qsTr("Current folder: %1").arg(Library.folderName)
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 42
            radius: Theme.radiusSmall
            color: "transparent"
            border.width: 1
            border.color: Theme.border

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: 9

                Icon {
                    name: "list"
                    size: 16
                    color: Theme.textFaint
                }
                Text {
                    Layout.fillWidth: true
                    text: qsTr("Playlists")
                    color: Theme.textMuted
                    font.family: Theme.fontFamily
                    font.pixelSize: 13
                }
                Text {
                    text: "M3"
                    color: Theme.textFaint
                    font.family: Theme.monoFamily
                    font.pixelSize: 10
                }
            }
        }

        Item { Layout.fillHeight: true }

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
            text: "v0.1 · M1"
            color: Theme.textFaint
            font.family: Theme.monoFamily
            font.pixelSize: 10
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
