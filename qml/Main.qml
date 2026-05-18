import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Basic 2.15
import QtQuick.Layouts 1.15
import CompactVLMQt 1.0

ApplicationWindow {
    id: mainWindow

    title: "CompactVLM"
    width:  1200
    height: 700
    minimumWidth:  900
    minimumHeight: 500
    visible: true

    // Center on screen and auto-initialize VLM
    Component.onCompleted: {
        x = Screen.width  / 2 - width  / 2
        y = Screen.height / 2 - height / 2
        if (vlmBridge) vlmBridge.initialize()
    }

    // ── Background color ───────────────────────────────────────────────────────
    color: Theme.bgDark

    // ── Menu bar ───────────────────────────────────────────────────────────
    menuBar: MenuBar {
        implicitHeight: Theme.menuBarHeight
        background: Rectangle { color: Theme.bgPanel }

        delegate: MenuBarItem {
            implicitHeight: Theme.menuBarHeight
            leftPadding:  10
            rightPadding: 10

            contentItem: Text {
                text:                parent.text
                font.family:         Theme.fontFamily
                font.pixelSize:      Theme.fontSizeSmall
                color:               parent.highlighted ? Theme.textPrimary
                                                        : Theme.textSecondary
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment:   Text.AlignVCenter
            }

            background: Rectangle {
                color: parent.highlighted ? Theme.btnHover : "transparent"
            }
        }

        Menu {
            title: "File"
            implicitHeight: Theme.menuItemHeight
            palette.window:          Theme.bgPanel
            palette.text:            Theme.textSecondary
            palette.highlightedText: Theme.textPrimary
            palette.highlight:       Theme.btnHover
            MenuItem { text: "Exit (&X)"; implicitHeight: Theme.menuItemHeight; onTriggered: Qt.quit() }
        }

        Menu {
            title: "Help"
            implicitHeight: Theme.menuItemHeight
            palette.window:          Theme.bgPanel
            palette.text:            Theme.textSecondary
            palette.highlightedText: Theme.textPrimary
            palette.highlight:       Theme.btnHover
            MenuItem { text: "Information (&A)"; implicitHeight: Theme.menuItemHeight; onTriggered: aboutDialog.open() }
        }
    }

    // ── Main layout: left (video) + right (chat) ──────────────────
    SplitView {
        anchors.fill: parent
        orientation:  Qt.Horizontal
        handle: Rectangle {
            implicitWidth: 4
            color: SplitHandle.hovered ? Theme.accent : Theme.divider
        }

        // Left: video panel
        VideoPanel {
            id:                        videoPanel
            SplitView.preferredWidth:  380
            SplitView.minimumWidth:    280
            SplitView.maximumWidth:    600

            // Capture image → update image provider and forward to chat panel
            onFrameCaptured: (base64) => {
                                 captureProvider.setImageBase64(base64)
                                 chatPanel.previewVersion    = captureProvider.version()
                                 chatPanel.pendingImageBase64 = base64
                             }
        }

        // Right: chat panel
        ChatPanel {
            id:                    chatPanel
            SplitView.fillWidth:   true
            SplitView.minimumWidth: 300
        }
    }

    // ── About dialog ────────────────────────────────────────
    AboutDialog {
        id: aboutDialog
    }
}
