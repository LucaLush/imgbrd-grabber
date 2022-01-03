import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs

import ".."

Item {
    id: root

    property string name
    property Setting setting

    implicitHeight: item.implicitHeight

    SettingItem {
        id: item

        name: root.name
        subtitle: setting.value.startsWith("/storage/emulated/0/")
            ? setting.value.substr(19)
            : setting.value
        anchors.fill: parent

        onClicked: dialog.open()

        FileDialog {
            id: dialog

            title: qsTr("Please choose a directory")
            folder: setting.value
            selectFolder: true

            onAccepted: setting.setValue(backend.toLocalFile(dialog.fileUrl.toString()))
        }
    }
}
