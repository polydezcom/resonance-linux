import Resonance
import QtQuick
import QtQuick.Layouts

Rectangle {
    id: panel

    signal closeRequested()

    color: Theme.surface
    border.width: 1
    border.color: Theme.border
    radius: Theme.radiusLarge

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 12

        RowLayout {
            Layout.fillWidth: true

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                SectionLabel { text: "PLAYBACK TOOLS" }
                Text {
                    text: qsTr("Shape the sound")
                    color: Theme.text
                    font.family: Theme.fontFamily
                    font.pixelSize: 18
                    font.weight: Font.DemiBold
                }
            }

            IconButton {
                iconName: "x"
                iconSize: 15
                buttonSize: 30
                accessibleLabel: qsTr("Close playback tools")
                onClicked: panel.closeRequested()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: Theme.radius
            color: Theme.surfaceAlt
            border.width: 1
            border.color: Theme.border

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 14

                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("10-band equalizer")
                        color: Theme.text
                        font.family: Theme.fontFamily
                        font.pixelSize: 13
                        font.weight: Font.Medium
                    }
                    Text {
                        text: "M4"
                        color: Theme.accent
                        font.family: Theme.monoFamily
                        font.pixelSize: 10
                    }
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("31 Hz – 16 kHz, preamp, presets and smooth live changes.")
                    color: Theme.textMuted
                    font.family: Theme.fontFamily
                    font.pixelSize: 11
                    wrapMode: Text.WordWrap
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: Theme.border
                }

                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("Speed · pitch · reverb")
                        color: Theme.text
                        font.family: Theme.fontFamily
                        font.pixelSize: 13
                        font.weight: Font.Medium
                    }
                    Text {
                        text: "M5"
                        color: Theme.accent
                        font.family: Theme.monoFamily
                        font.pixelSize: 10
                    }
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Tempo and pitch, Freeverb-style space, and a slowed + reverb preset.")
                    color: Theme.textMuted
                    font.family: Theme.fontFamily
                    font.pixelSize: 11
                    wrapMode: Text.WordWrap
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: Theme.border
                }

                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("A–B loop")
                        color: Theme.text
                        font.family: Theme.fontFamily
                        font.pixelSize: 13
                        font.weight: Font.Medium
                    }
                    Text {
                        text: "M5"
                        color: Theme.accent
                        font.family: Theme.monoFamily
                        font.pixelSize: 10
                    }
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Loop a phrase, optionally ramping the tempo on each pass.")
                    color: Theme.textMuted
                    font.family: Theme.fontFamily
                    font.pixelSize: 11
                    wrapMode: Text.WordWrap
                }

                Item { Layout.fillHeight: true }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: Theme.border
                }

                SectionLabel { text: "NOW PLAYING" }

                Text {
                    Layout.fillWidth: true
                    text: Player.hasTrack ? Player.title : qsTr("Nothing playing")
                    color: Theme.text
                    font.family: Theme.fontFamily
                    font.pixelSize: 13
                    font.weight: Font.Medium
                    elide: Text.ElideRight
                }

                Text {
                    Layout.fillWidth: true
                    text: Player.hasTrack ? Player.path : qsTr("Open a folder to begin.")
                    color: Theme.textFaint
                    font.family: Theme.monoFamily
                    font.pixelSize: 10
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    maximumLineCount: 3
                    elide: Text.ElideRight
                }

                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: Player.status
                        color: Theme.textMuted
                        font.family: Theme.fontFamily
                        font.pixelSize: 10
                    }
                    Item { Layout.fillWidth: true }
                    Text {
                        text: Player.durationMs > 0 ? Player.formatTime(Player.durationMs) : "--:--"
                        color: Theme.textMuted
                        font.family: Theme.monoFamily
                        font.pixelSize: 10
                    }
                }
            }
        }
    }
}
