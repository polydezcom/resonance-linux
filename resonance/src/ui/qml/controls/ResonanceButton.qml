import Resonance
import QtQuick
import QtQuick.Controls.Basic

Button {
    id: control

    property bool primary: false
    property bool ghost: false
    property bool compact: false
    property string iconName: ""
    property string accessibleLabel: ""

    hoverEnabled: true
    implicitHeight: compact ? 32 : 38
    leftPadding: compact ? 10 : 14
    rightPadding: compact ? 10 : 14
    topPadding: compact ? 6 : 9
    bottomPadding: compact ? 6 : 9
    font.family: Theme.fontFamily
    font.pixelSize: Theme.fontSize
    font.weight: Font.Medium
    Accessible.name: accessibleLabel.length > 0 ? accessibleLabel : text

    contentItem: Item {
        implicitWidth: row.implicitWidth
        implicitHeight: Math.max(row.implicitHeight, 18)

        Row {
            id: row
            anchors.centerIn: parent
            spacing: control.iconName.length > 0 && control.text.length > 0 ? 8 : 0

            Icon {
                visible: control.iconName.length > 0
                name: control.iconName
                size: control.compact ? 16 : 18
                color: control.primary ? Theme.onAccent : Theme.text
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                visible: control.text.length > 0
                text: control.text
                color: control.primary ? Theme.onAccent : Theme.text
                font: control.font
                verticalAlignment: Text.AlignVCenter
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    background: Rectangle {
        radius: Theme.radiusSmall
        color: {
            if (control.primary)
                return control.down ? Qt.darker(Theme.accent, 1.12)
                                    : control.hovered ? Theme.accentHover : Theme.accent
            if (control.ghost)
                return control.down ? Theme.surfaceAlt
                                    : control.hovered ? Theme.hover : "transparent"
            return control.down ? Theme.surfaceAlt
                                : control.hovered ? Theme.hover : Theme.surfaceAlt
        }
        border.width: control.visualFocus ? 2 : (control.primary || control.ghost ? 0 : 1)
        border.color: control.visualFocus
                      ? Theme.accent
                      : (control.primary ? "transparent" : Theme.border)

        Behavior on color {
            ColorAnimation { duration: Theme.animFast }
        }
    }

    HoverHandler {
        cursorShape: Qt.PointingHandCursor
    }
}
