import Resonance
import QtQuick
import QtQuick.Controls.Basic

Slider {
    id: control

    property color trackColor: Theme.surfaceAlt
    property color fillColor: Theme.accent
    property color handleColor: Theme.accent
    property real trackThickness: 4
    property string accessibleLabel: ""

    implicitHeight: 24
    Accessible.name: accessibleLabel
    hoverEnabled: true

    background: Item {
        x: control.leftPadding
        y: control.topPadding + (control.availableHeight - height) / 2
        width: control.availableWidth
        height: control.trackThickness
        opacity: control.enabled ? 1.0 : 0.35

        Rectangle {
            anchors.fill: parent
            radius: height / 2
            color: control.trackColor
        }

        Rectangle {
            width: control.visualPosition * parent.width
            height: parent.height
            radius: height / 2
            color: control.fillColor
        }
    }

    handle: Rectangle {
        x: control.leftPadding + control.visualPosition
           * (control.availableWidth - width)
        y: control.topPadding + control.availableHeight / 2 - height / 2
        width: control.pressed ? 15 : 12
        height: width
        radius: width / 2
        color: control.handleColor
        border.width: control.activeFocus ? 2 : 0
        border.color: Theme.text

        Behavior on width {
            NumberAnimation { duration: Theme.animFast }
        }
    }

    HoverHandler {
        cursorShape: control.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
    }
}
