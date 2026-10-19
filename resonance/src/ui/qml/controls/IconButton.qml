import Resonance
import QtQuick
import QtQuick.Controls.Basic

Button {
    id: control

    property string iconName: "play"
    property int iconSize: 20
    property int buttonSize: 36
    property bool accent: false
    property bool subtle: true
    property color iconColor: Theme.textMuted
    property string accessibleLabel: ""

    implicitWidth: buttonSize
    implicitHeight: buttonSize
    padding: 0
    hoverEnabled: true
    Accessible.name: accessibleLabel

    contentItem: Icon {
        anchors.centerIn: parent
        name: control.iconName
        size: control.iconSize
        opacity: control.enabled ? 1.0 : 0.35
        color: control.accent
               ? Theme.onAccent
               : control.hovered && control.enabled ? Theme.text : control.iconColor
    }

    background: Rectangle {
        radius: width / 2
        color: control.accent
               ? (control.down ? Qt.darker(Theme.accent, 1.12)
                               : control.hovered ? Theme.accentHover : Theme.accent)
               : control.subtle
                 ? (control.down ? Theme.surfaceAlt
                                 : control.hovered ? Theme.hover : "transparent")
                 : (control.hovered ? Theme.surfaceAlt : Theme.surface)
        border.width: control.visualFocus ? 2 : (control.subtle || control.accent ? 0 : 1)
        border.color: control.visualFocus ? Theme.accent : Theme.border
        opacity: control.enabled ? 1.0 : 0.35

        Behavior on color {
            ColorAnimation { duration: Theme.animFast }
        }
    }

    HoverHandler {
        cursorShape: control.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
    }
}
