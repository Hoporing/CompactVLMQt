import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

// ────────────────────────────────────────────────────────────────
// About dialog with LGPL license notice
// Qt LGPL compliance requirements:
//   1. Dynamic library linking (BUILD_SHARED_LIBS=ON in CMakeLists.txt)
//   2. Display license notice in this dialog
// ────────────────────────────────────────────────────────────────
Dialog {
    id: root

    title: "Program Information"
    modal: true
    width:  480
    height: 400

    anchors.centerIn: parent

    // Background
    background: Rectangle {
        color:        Theme.bgMedium
        radius:       Theme.radiusNormal
        border.color: Theme.divider
        border.width: 1
    }

    // Title bar
    header: Rectangle {
        width:  parent.width
        height: 44
        color:  Theme.bgPanel
        radius: Theme.radiusNormal

        // Square off bottom corners
        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: Theme.radiusNormal
            color: Theme.bgPanel
        }

        Text {
            anchors.centerIn: parent
            text:  "Program Information"
            color: Theme.textPrimary
            font { family: Theme.fontFamily; pixelSize: Theme.fontSizeLarge; bold: true }
        }
    }

    contentItem: ColumnLayout {
        spacing: Theme.marginNormal

        // App name / version
        Text {
            Layout.alignment: Qt.AlignHCenter
            text:  "CompactVLM"
            color: Theme.accent
            font { family: Theme.fontFamily; pixelSize: 22; bold: true }
        }
        Text {
            Layout.alignment: Qt.AlignHCenter
            text:  "Version 1.0.0"
            color: Theme.textSecondary
            font { family: Theme.fontFamily; pixelSize: Theme.fontSizeNormal }
        }

        Rectangle { height: 1; Layout.fillWidth: true; color: Theme.divider }

        // ── LGPL license notice ────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            height:           licenseText.implicitHeight + 20
            color:            Theme.bgDark
            radius:           Theme.radiusSmall
            border.color:     Theme.divider

            ColumnLayout {
                anchors { fill: parent; margins: 10 }
                spacing: 6

                Text {
                    text:  "Open Source License"
                    color: Theme.warning
                    font { family: Theme.fontFamily; pixelSize: Theme.fontSizeNormal; bold: true }
                }

                Text {
                    id: licenseText
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    // ────────────────────────────────────────
                    // LGPL notice text (required)
                    // ────────────────────────────────────────
                    text: "This application uses the Qt library, which is licensed under " +
                          "the GNU Lesser General Public License (LGPL) v3.\n\n" +
                          "Qt is used as a dynamically linked library. " +
                          "The Qt source code is available at the Qt official website."
                    color: Theme.textSecondary
                    font { family: Theme.fontFamily; pixelSize: Theme.fontSizeSmall }
                }

                // Qt homepage link
                Text {
                    text: '<a href="https://www.qt.io">https://www.qt.io</a>'
                    color: Theme.accent
                    font { family: Theme.fontFamily; pixelSize: Theme.fontSizeSmall }
                    onLinkActivated: (link) => Qt.openUrlExternally(link)
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: Qt.openUrlExternally("https://www.qt.io")
                    }
                }

                // LGPL full text link
                Text {
                    text: '<a href="https://www.gnu.org/licenses/lgpl-3.0.html">View LGPL v3 License Full Text</a>'
                    color: Theme.accent
                    font { family: Theme.fontFamily; pixelSize: Theme.fontSizeSmall }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: Qt.openUrlExternally("https://www.gnu.org/licenses/lgpl-3.0.html")
                    }
                }
            }
        }

        Item { Layout.fillHeight: true }
    }

    // Close button
    footer: Rectangle {
        width:  parent.width
        height: 50
        color:  Theme.bgPanel

        StyledButton {
            anchors.centerIn: parent
            text:    "닫기"
            width:   100
            onClicked: root.close()
        }
    }
}
