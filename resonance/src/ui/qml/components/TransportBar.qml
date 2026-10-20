import Resonance
import QtQuick
import QtQuick.Layouts

Rectangle {
    id: transport

    implicitHeight: Theme.transportHeight
    color: Theme.surface

    Rectangle {
        anchors.top: parent.top
        width: parent.width
        height: 1
        color: Theme.border
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 20
        anchors.rightMargin: 20
        anchors.topMargin: 14
        anchors.bottomMargin: 14
        spacing: 16

        Rectangle {
            Layout.preferredWidth: 64
            Layout.preferredHeight: 64
            radius: 15
            clip: true
            opacity: Player.hasTrack ? 1.0 : 0.5

            Rectangle {
                anchors.fill: parent
                gradient: Gradient {
                    GradientStop { position: 0.0; color: Qt.lighter(Theme.accent, 1.35) }
                    GradientStop { position: 0.55; color: Theme.accent }
                    GradientStop { position: 1.0; color: Qt.darker(Theme.accent, 1.35) }
                }
            }

            Image {
                anchors.fill: parent
                source: Player.coverUrl
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                cache: false
                visible: status === Image.Ready
            }

            Icon {
                anchors.centerIn: parent
                name: "music"
                size: 26
                color: Theme.onAccent
                visible: Player.coverUrl.length === 0
            }
        }

        ColumnLayout {
            Layout.preferredWidth: 220
            Layout.minimumWidth: 150
            spacing: 2

            Text {
                Layout.fillWidth: true
                text: Player.hasTrack ? Player.title : qsTr("Nothing playing")
                color: Theme.text
                font.family: Theme.fontFamily
                font.pixelSize: 14
                font.weight: Font.DemiBold
                elide: Text.ElideRight
            }

            Text {
                Layout.fillWidth: true
                text: Player.hasTrack ? Player.subtitle : qsTr("Open a folder to begin")
                color: Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: 11
                elide: Text.ElideRight
            }

            Text {
                Layout.fillWidth: true
                text: Player.queueSize > 0
                      ? qsTr("%1 / %2 · %3")
                        .arg(Player.queueIndex + 1)
                        .arg(Player.queueSize)
                        .arg(Player.status)
                      : Player.status
                color: Player.error.length > 0 ? Theme.danger
                                               : Player.audioReady ? Theme.textFaint : Theme.danger
                font.family: Theme.fontFamily
                font.pixelSize: 10
                elide: Text.ElideRight
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 240
            spacing: 2

            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 3

                IconButton {
                    iconName: "shuffle"
                    iconSize: 15
                    buttonSize: 30
                    accent: Player.shuffle
                    onClicked: Player.toggleShuffle()
                    accessibleLabel: Player.shuffle ? qsTr("Disable shuffle") : qsTr("Shuffle folder")
                }

                IconButton {
                    iconName: "skip-back"
                    iconSize: 18
                    buttonSize: 34
                    enabled: Player.hasTrack
                    onClicked: Player.previous()
                    accessibleLabel: qsTr("Previous track")
                }

                IconButton {
                    iconName: Player.playing ? "pause" : "play"
                    iconSize: 20
                    buttonSize: 46
                    accent: true
                    enabled: Player.hasTrack
                    onClicked: Player.togglePlayPause()
                    accessibleLabel: Player.playing ? qsTr("Pause") : qsTr("Play")
                }

                IconButton {
                    iconName: "skip-forward"
                    iconSize: 18
                    buttonSize: 34
                    enabled: Player.hasTrack
                    onClicked: Player.next()
                    accessibleLabel: qsTr("Next track")
                }

                IconButton {
                    iconName: Player.repeatMode === 1 ? "repeat-one" : "repeat"
                    iconSize: 15
                    buttonSize: 30
                    accent: Player.repeatMode > 0
                    onClicked: Player.cycleRepeat()
                    accessibleLabel: Player.repeatMode === 0
                                     ? qsTr("Repeat off")
                                     : Player.repeatMode === 1
                                       ? qsTr("Repeat track")
                                       : qsTr("Repeat folder")
                }

                Item { Layout.preferredWidth: 4 }

                IconButton {
                    iconName: "stop"
                    iconSize: 14
                    buttonSize: 30
                    enabled: Player.hasTrack
                    onClicked: Player.stop()
                    accessibleLabel: qsTr("Stop")
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Text {
                    Layout.preferredWidth: 38
                    text: Player.formatTime(seekSlider.pressed ? seekSlider.value
                                                               : Player.positionMs)
                    color: Theme.textMuted
                    font.family: Theme.monoFamily
                    font.pixelSize: 10
                    horizontalAlignment: Text.AlignRight
                }

                ResonanceSlider {
                    id: seekSlider
                    Layout.fillWidth: true
                    from: 0
                    to: Math.max(1, Player.durationMs)
                    value: 0
                    enabled: Player.durationMs > 0
                    accessibleLabel: qsTr("Seek")
                    onMoved: Player.seek(Math.round(value))

                    Connections {
                        target: Player
                        function onPositionChanged() {
                            if (!seekSlider.pressed)
                                seekSlider.value = Player.positionMs
                        }
                        function onDurationChanged() {
                            if (!seekSlider.pressed)
                                seekSlider.value = Player.positionMs
                        }
                    }

                    Component.onCompleted: value = Player.positionMs
                }

                Text {
                    Layout.preferredWidth: 38
                    text: Player.durationMs > 0 ? Player.formatTime(Player.durationMs)
                                                : "--:--"
                    color: Theme.textMuted
                    font.family: Theme.monoFamily
                    font.pixelSize: 10
                }
            }
        }

        RowLayout {
            Layout.preferredWidth: 158
            spacing: 6

            Icon {
                name: Player.volume <= 0.001 ? "volume-x"
                                              : Player.volume < 0.55 ? "volume-1" : "volume-2"
                size: 17
                color: Theme.textMuted
            }

            ResonanceSlider {
                id: volumeSlider
                Layout.fillWidth: true
                Layout.preferredWidth: 100
                from: 0
                to: 100
                value: 0
                accessibleLabel: qsTr("Volume")
                onMoved: Player.volume = value / 100

                Connections {
                    target: Player
                    function onVolumeChanged() {
                        if (!volumeSlider.pressed)
                            volumeSlider.value = Player.volume * 100
                    }
                }

                Component.onCompleted: value = Player.volume * 100
            }

            Text {
                text: Math.round(Player.volume * 100)
                color: Theme.textFaint
                font.family: Theme.monoFamily
                font.pixelSize: 9
                horizontalAlignment: Text.AlignRight
                Layout.preferredWidth: 20
            }
        }
    }
}
