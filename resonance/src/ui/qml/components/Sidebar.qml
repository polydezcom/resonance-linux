import Resonance
import QtQuick
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
            text: "v0.2 · M2"
            color: Theme.textFaint
            font.family: Theme.monoFamily
            font.pixelSize: 10
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
