import Resonance
import QtQuick
import QtQuick.Layouts

Rectangle {
    id: header

    property string title: "Your music"
    property string detail: ""
    property bool scanning: false
    property bool toolsOpen: false

    signal openFolderRequested()
    signal toggleToolsRequested()

    implicitHeight: Theme.headerHeight
    color: "transparent"

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.pagePadding
        anchors.rightMargin: Theme.pagePadding
        spacing: 14

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            SectionLabel { text: "COLLECTION" }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Text {
                    Layout.fillWidth: true
                    text: header.title
                    color: Theme.text
                    font.family: Theme.fontFamily
                    font.pixelSize: 19
                    font.weight: Font.DemiBold
                    elide: Text.ElideRight
                }

                Rectangle {
                    Layout.preferredWidth: 7
                    Layout.preferredHeight: 7
                    radius: width / 2
                    color: Theme.accent
                    visible: header.scanning
                    Layout.alignment: Qt.AlignVCenter

                    SequentialAnimation on opacity {
                        running: header.scanning
                        loops: Animation.Infinite
                        NumberAnimation { to: 0.3; duration: 650; easing.type: Easing.InOutSine }
                        NumberAnimation { to: 1.0; duration: 650; easing.type: Easing.InOutSine }
                    }
                }
            }

            Text {
                Layout.fillWidth: true
                text: header.scanning ? qsTr("Scanning folder…") : header.detail
                color: Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: 12
                elide: Text.ElideMiddle
                visible: text.length > 0
            }
        }

        ResonanceButton {
            text: qsTr("Open folder")
            iconName: "folder-open"
            compact: true
            onClicked: header.openFolderRequested()
            Accessible.name: qsTr("Open a music folder")
        }

        IconButton {
            iconName: "sliders"
            iconSize: 19
            accent: header.toolsOpen
            accessibleLabel: header.toolsOpen ? qsTr("Hide playback tools") : qsTr("Show playback tools")
            onClicked: header.toggleToolsRequested()
        }
    }
}
