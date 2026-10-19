pragma Singleton
import QtQuick

QtObject {
    id: theme

    property bool dark: true
    property color accent: "#7C9CFF"

    readonly property color background: dark ? "#0B0D12" : "#EEF1F6"
    readonly property color sidebar: dark ? "#0F1219" : "#E7EBF2"
    readonly property color surface: dark ? "#141821" : "#FFFFFF"
    readonly property color surfaceAlt: dark ? "#1A1F2B" : "#F4F6FA"
    readonly property color surfaceElevated: dark ? "#222837" : "#FFFFFF"
    readonly property color hover: dark ? "#202634" : "#E9EDF5"
    readonly property color border: dark ? "#252B39" : "#DCE2EC"
    readonly property color borderStrong: dark ? "#333B4D" : "#C9D2E0"
    readonly property color text: dark ? "#EDF0F7" : "#141923"
    readonly property color textMuted: dark ? "#9099AC" : "#5F6B7E"
    readonly property color textFaint: dark ? "#626C7E" : "#8B96A8"
    readonly property color danger: dark ? "#FF6B7A" : "#D92D4A"
    readonly property color success: dark ? "#5AD6A5" : "#168A62"
    readonly property color shadow: dark ? "#000000" : "#1B2430"

    readonly property color selected: Qt.rgba(accent.r, accent.g, accent.b,
                                              dark ? 0.16 : 0.12)
    readonly property color accentSoft: Qt.rgba(accent.r, accent.g, accent.b,
                                                 dark ? 0.14 : 0.10)
    readonly property color accentHover: Qt.lighter(accent, dark ? 1.12 : 1.04)
    readonly property color onAccent: (accent.r * 0.299 + accent.g * 0.587
                                       + accent.b * 0.114) > 0.62
                                      ? "#11151D" : "#FFFFFF"

    readonly property int radiusSmall: 8
    readonly property int radius: 12
    readonly property int radiusLarge: 18
    readonly property int spacing: 12
    readonly property int pagePadding: 24
    readonly property int sidebarWidth: 248
    readonly property int rightPanelWidth: 300
    readonly property int headerHeight: 68
    readonly property int transportHeight: 106
    readonly property int animFast: 150
    readonly property int anim: 200

    // An empty family means "use the application's default UI font".
    readonly property string fontFamily: ""
    readonly property string monoFamily: "monospace"
    readonly property int fontSizeSmall: 11
    readonly property int fontSize: 13
    readonly property int fontSizeLarge: 17
}
