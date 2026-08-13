import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

// Memory management (2ND++). Shows what's currently in use and offers
// targeted clearing per category, plus the full factory RESET. Counts
// refresh on open and after every action.
Popup {
    id: root

    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    width: 320
    height: 560
    padding: 14

    x: (parent.width - width) / 2
    y: (parent.height - height) / 2

    background: Rectangle {
        color: Style.bgSurface
        radius: 8
        border.color: Style.opBorder
        border.width: 1
    }

    property var info: ({})
    property int savesRev: 0   // bumped to refresh the saves list
    function refresh() { info = uiController.memInfo() }
    onOpened: refresh()

    contentItem: ColumnLayout {
        spacing: 10

        Text {
            Layout.fillWidth: true
            text: "MEMORY"
            color: Style.textMuted
            font.family: Style.monoFamily
            font.pixelSize: Style.popupTitlePixelSize
            font.letterSpacing: Style.popupTitlePixelSize * Style.sectionLabelLetterSpacing
            font.capitalization: Font.AllUppercase
            horizontalAlignment: Text.AlignHCenter
        }
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Style.bgSection
        }

        // ── In-use summary ──
        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: 16
            rowSpacing: 3
            Repeater {
                model: [
                    { k: "vars",      label: "Vars A–Z" },
                    { k: "matrices",  label: "Matrices" },
                    { k: "lists",     label: "Lists" },
                    { k: "functions", label: "Y= funcs" },
                    { k: "entries",   label: "History" }
                ]
                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: modelData.label
                        color: Style.textSecondary
                        font.family: Style.monoFamily
                        font.pixelSize: Style.funcKeyLabelPixelSize
                        Layout.fillWidth: true
                    }
                    Text {
                        text: (root.info && root.info[modelData.k] !== undefined)
                              ? root.info[modelData.k] : "0"
                        color: Style.textPrimary
                        font.family: Style.monoFamily
                        font.pixelSize: Style.funcKeyLabelPixelSize
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Style.bgSection
        }

        // ── Targeted clears ──
        CalcKey {
            Layout.fillWidth: true
            label: "Clear All Lists"
            keyType: "function"
            onPressed: { uiController.clearAllLists(); root.refresh() }
        }
        CalcKey {
            Layout.fillWidth: true
            label: "Clear All Matrices"
            keyType: "function"
            onPressed: { uiController.clearAllMatrices(); root.refresh() }
        }
        CalcKey {
            Layout.fillWidth: true
            label: "Clear Vars A–Z"
            keyType: "function"
            onPressed: { uiController.clearAllVars(); root.refresh() }
        }
        CalcKey {
            Layout.fillWidth: true
            label: "Clear Entries"
            keyType: "function"
            onPressed: { uiController.clearEntries(); root.refresh() }
        }
        CalcKey {
            Layout.fillWidth: true
            label: "RESET (everything)"
            keyType: "control"
            onPressed: { uiController.resetAll(); root.close() }
        }

        // ── Save / load named snapshots ──
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Style.bgSection
        }
        Text {
            text: "SAVE / LOAD"
            color: Style.textMuted
            font.family: Style.monoFamily
            font.pixelSize: Style.funcKeyLabelPixelSize
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            TextField {
                id: nameField
                Layout.fillWidth: true
                Layout.preferredHeight: 30
                placeholderText: "save name"
                color: Style.textDisplay
                font.family: Style.monoFamily
                font.pixelSize: Style.funcKeyLabelPixelSize
                selectByMouse: true
                background: Rectangle {
                    color: Style.bgDisplay
                    radius: 4
                    border.color: nameField.activeFocus ? Style.textExpr : Style.keyBorderNeutral
                    border.width: 1
                }
            }
            CalcKey {
                Layout.preferredWidth: 80
                label: "Export"
                keyType: "enter"
                onPressed: {
                    if (uiController.exportState(nameField.text)) {
                        nameField.text = ""
                        root.savesRev++
                    }
                }
            }
        }
        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 60
            clip: true
            spacing: 3
            boundsBehavior: Flickable.StopAtBounds
            model: (root.savesRev, uiController.listSaves())
            delegate: RowLayout {
                width: ListView.view.width
                height: 26
                spacing: 6
                Text {
                    Layout.fillWidth: true
                    text: modelData
                    color: Style.textPrimary
                    font.family: Style.monoFamily
                    font.pixelSize: Style.funcKeyLabelPixelSize
                    elide: Text.ElideRight
                }
                Rectangle {
                    Layout.preferredWidth: 44
                    Layout.preferredHeight: 22
                    radius: 4
                    color: loadArea.containsMouse
                           ? Qt.lighter(Style.bgSection, 1.0 + Style.keyHoverLighten)
                           : Style.bgSection
                    border.width: 1
                    border.color: Style.enterBorder
                    Text {
                        anchors.centerIn: parent
                        text: "Load"
                        color: Style.textResult
                        font.family: Style.monoFamily
                        font.pixelSize: Style.funcKeyLabelPixelSize
                    }
                    MouseArea {
                        id: loadArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            uiController.importState(modelData)
                            root.refresh()
                            root.close()
                        }
                    }
                }
                Rectangle {
                    Layout.preferredWidth: 24
                    Layout.preferredHeight: 22
                    radius: 4
                    color: delArea.containsMouse
                           ? Qt.lighter(Style.bgSection, 1.0 + Style.keyHoverLighten)
                           : Style.bgSection
                    border.width: 1
                    border.color: Style.keyBorderNeutral
                    Text {
                        anchors.centerIn: parent
                        text: "✕"
                        color: Style.textError
                        font.family: Style.monoFamily
                        font.pixelSize: Style.funcKeyLabelPixelSize
                    }
                    MouseArea {
                        id: delArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: { uiController.deleteSave(modelData); root.savesRev++ }
                    }
                }
            }
        }

        CalcKey {
            label: "DONE"
            keyType: "enter"
            onPressed: root.close()
        }
    }
}
