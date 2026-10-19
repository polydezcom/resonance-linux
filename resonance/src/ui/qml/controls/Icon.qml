import Resonance
import QtQuick
import QtQuick.Effects

Item {
    id: root

    property string name: "music"
    property color color: Theme.text
    property int size: 20

    implicitWidth: size
    implicitHeight: size
    Accessible.ignored: true

    Image {
        id: source
        anchors.fill: parent
        source: "qrc:/icons/" + root.name + ".svg"
        sourceSize.width: Math.max(1, root.size * 2)
        sourceSize.height: Math.max(1, root.size * 2)
        fillMode: Image.PreserveAspectFit
        smooth: true
        visible: false
    }

    MultiEffect {
        source: source
        anchors.fill: parent
        colorization: 1.0
        colorizationColor: root.color
    }
}
