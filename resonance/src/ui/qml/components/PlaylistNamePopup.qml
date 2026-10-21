import Resonance
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Popup {
    id: popup

    property string playlistId: ""
    property bool renameMode: false
    property string initialName: ""

    function openCreate() {
        renameMode = false
        playlistId = ""
        initialName = qsTr("New playlist")
        open()
    }

    function openRename(id, name) {
        renameMode = true
        playlistId = id
        initialName = name
        open()
    }

    function submit() {
        const name = nameField.text.trim()
        if (name.length === 0)
            return

        if (renameMode)
            Playlists.renamePlaylist(playlistId, name)
        else
            Playlists.createPlaylist(name)
        close()
    }

    parent: Overlay.overlay
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    width: 340
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    padding: 18
    onOpened: {
        nameField.text = initialName
        nameField.forceActiveFocus()
        nameField.selectAll()
    }

    background: Rectangle {
        color: Theme.surfaceElevated
        radius: Theme.radiusLarge
        border.width: 1
        border.color: Theme.borderStrong
    }

    contentItem: ColumnLayout {
        spacing: 12

        SectionLabel { text: popup.renameMode ? qsTr("RENAME PLAYLIST") : qsTr("NEW PLAYLIST") }

        Text {
            Layout.fillWidth: true
            text: popup.renameMode ? qsTr("Choose a new name.") : qsTr("Name this playlist.")
            color: Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: 12
            wrapMode: Text.WordWrap
        }

        ResonanceTextField {
            id: nameField
            Layout.fillWidth: true
            placeholderText: qsTr("Playlist name")
            Accessible.name: qsTr("Playlist name")
            onAccepted: popup.submit()
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Item { Layout.fillWidth: true }

            ResonanceButton {
                text: qsTr("Cancel")
                ghost: true
                compact: true
                onClicked: popup.close()
            }

            ResonanceButton {
                text: popup.renameMode ? qsTr("Rename") : qsTr("Create")
                primary: true
                compact: true
                enabled: nameField.text.trim().length > 0
                onClicked: popup.submit()
            }
        }
    }
}
