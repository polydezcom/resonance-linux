import Resonance
import QtQuick
import QtQuick.Controls.Basic

TextField {
    id: control

    color: Theme.text
    placeholderTextColor: Theme.textFaint
    selectionColor: Theme.accentSoft
    selectedTextColor: Theme.text
    font.family: Theme.fontFamily
    font.pixelSize: 13
    selectByMouse: true
    leftPadding: 11
    rightPadding: 11
    topPadding: 9
    bottomPadding: 9
    implicitHeight: 38

    background: Rectangle {
        radius: Theme.radiusSmall
        color: Theme.surfaceAlt
        border.width: control.activeFocus ? 2 : 1
        border.color: control.activeFocus ? Theme.accent : Theme.border
    }
}
