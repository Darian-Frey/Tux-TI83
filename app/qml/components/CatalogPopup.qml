import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

// Modal popup: alphabetical browser of every insertable token (2ND + 0
// on a real TI-83). Source of truth is `uiController.catalogEntries()`,
// so the list automatically picks up any new kTokens row without
// touching this file.
//
// Behavioural contract: clicking an entry sends it through
// `processExpression` (so multi-token sequences like `^-1` would still
// work, though the catalog is single-token by construction) and closes
// the popup. Includes a search field that filters incrementally —
// users can type a few characters to jump straight to `cosh(` without
// scrolling.
Popup {
    id: root

    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    width: 320
    height: 540
    padding: 14

    x: (parent.width - width) / 2
    y: (parent.height - height) / 2

    background: Rectangle {
        color: Style.bgSurface
        radius: 8
        border.color: Style.opBorder
        border.width: 1
    }

    // Cached entries — fetched on first open so the popup picks up any
    // late kTokens additions without us having to listen for changes
    // (kTokens is compile-time-static, so a one-shot fetch is fine).
    property var allEntries: []
    property string filterText: ""

    onOpened: {
        if (allEntries.length === 0)
            allEntries = uiController.catalogEntries()
        filterText = ""
        searchField.forceActiveFocus()
    }

    // Filtered view — case-insensitive substring match. Computed every
    // time the search field changes; the lists are short enough
    // (~80 entries) that this is fine without memoising.
    function filteredEntries() {
        if (filterText.length === 0) return allEntries
        const needle = filterText.toLowerCase()
        return allEntries.filter(function(e) {
            return e.toLowerCase().indexOf(needle) >= 0
        })
    }

    contentItem: ColumnLayout {
        spacing: 10

        // ── Header ──
        Text {
            Layout.fillWidth: true
            text: "CATALOG"
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

        // ── Search field ──
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 28
            radius: 4
            color: Style.bgDisplay
            border.color: searchField.activeFocus ? Style.textExpr : Style.keyBorderNeutral
            border.width: 1

            TextField {
                id: searchField
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                placeholderText: "search…"
                placeholderTextColor: Style.textMuted
                color: Style.textDisplay
                font.family: Style.monoFamily
                font.pixelSize: Style.keyLabelPixelSize
                background: Item {}     // suppress default background
                selectByMouse: true
                onTextChanged: root.filterText = text
            }
        }

        // ── Filtered list ──
        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: root.filteredEntries()
            spacing: 3
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            delegate: Rectangle {
                width: list.width
                height: 28
                radius: 3
                color: rowArea.containsMouse
                       ? Qt.lighter(Style.bgSection, 1.0 + Style.keyHoverLighten)
                       : Style.bgSection

                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                    text: modelData
                    color: Style.textPrimary
                    font.family: Style.monoFamily
                    font.pixelSize: Style.keyLabelPixelSize
                }

                MouseArea {
                    id: rowArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        uiController.processExpression(modelData)
                        root.close()
                    }
                }
            }

            // Empty-state when the filter excludes everything.
            Text {
                anchors.centerIn: parent
                visible: list.count === 0
                text: "no matches"
                color: Style.textMuted
                font.family: Style.monoFamily
                font.pixelSize: Style.funcKeyLabelPixelSize
                font.italic: true
            }
        }
    }
}
