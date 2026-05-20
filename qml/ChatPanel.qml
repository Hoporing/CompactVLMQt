import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

// Right panel: VLM chat UI
Item {
    id: root

    // Capture image injected from outside
    property string pendingImageBase64: ""
    property int    previewVersion:     0


    // ── Chat message model ──────────────────────────
    ListModel { id: chatModel }

    // ── VLMBridge signal connections ────────────────────────────
    Connections {
        target: vlmBridge

        function onResponseReceived(text) {
            chatModel.append({ role: "assistant", content: text, hasImage: false, imageBase64: "" })
            chatListView.positionViewAtEnd()
        }
        function onErrorOccurred(error) {
            chatModel.append({ role: "error", content: "오류: " + error, hasImage: false, imageBase64: "" })
            chatListView.positionViewAtEnd()
        }
        function onInitializationDone(success) {
            if (success) {
                statusBar.text = "준비됨 (" + vlmBridge.modelName + ")"
            } else {
                statusBar.text = "초기화 실패 - Ollama 실행 여부 확인"
                statusBar.color = Theme.error
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing:      0

        // ── Status bar ───────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            height:           40
            color:            Theme.bgPanel

            RowLayout {
                anchors { fill: parent; margins: 8 }
                spacing: 8

                Rectangle {
                    width:  10; height: 10; radius: 5
                    color:  (vlmBridge && vlmBridge.ollamaReady) ? Theme.accentAlt : Theme.error
                }
                Text {
                    text:  "Ollama"
                    color: Theme.textSecondary
                    font { family: Theme.fontFamily; pixelSize: Theme.fontSizeSmall }
                }

                Item { Layout.fillWidth: true }

                Text {
                    id:    statusBar
                    text:  "초기화 필요"
                    color: Theme.textDisabled
                    font { family: Theme.fontFamily; pixelSize: Theme.fontSizeSmall }
                }

                StyledButton {
                    text:    "초기화"
                    compact: true
                    visible: vlmBridge ? !vlmBridge.ollamaReady : true
                    onClicked: if (vlmBridge) vlmBridge.initialize()
                }
            }
        }

        Rectangle { height: 1; Layout.fillWidth: true; color: Theme.divider }

        // ── Chat list ────────────────────────────────
        ListView {
            id:               chatListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip:             true
            spacing:          8
            model:            chatModel

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }

            add:    Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 150 } }
            remove: Transition { NumberAnimation { property: "opacity"; to: 0; duration: 150 } }

            delegate: Item {
                width: chatListView.width
                height: bubble.height + 12

                // User: right-aligned, assistant: left-aligned
                readonly property bool isUser:  model.role === "user"
                readonly property bool isError: model.role === "error"

                Rectangle {
                    id: bubble
                    anchors {
                        right:   isUser  ? parent.right  : undefined
                        left:    !isUser ? parent.left   : undefined
                        top:     parent.top
                        margins: Theme.marginNormal
                    }
                    // Image bubble: fixed reasonable width
                    // Text-only: natural text width capped at 78%
                    width: model.hasImage
                           ? Math.min(220, chatListView.width * 0.78)
                           : Math.min(bubbleText.implicitWidth + 16,
                                      chatListView.width * 0.78)
                    height: (bubbleImage.visible ? bubbleImage.height + 6 : 0)
                            + bubbleText.implicitHeight + 16
                    radius: Theme.radiusNormal
                    color:  isError ? "#4a1010"
                           : isUser ? Theme.bubbleUser
                           : Theme.bubbleAssistant
                    border.color: Theme.bubbleBorder
                    border.width: 1
                    clip: true

                    // Thumbnail shown above text when image is attached
                    Image {
                        id: bubbleImage
                        visible: model.hasImage && model.imageBase64 !== ""
                        source:  visible ? ("data:image/png;base64," + model.imageBase64) : ""
                        anchors { top: parent.top; left: parent.left; right: parent.right; margins: 8 }
                        height:  visible ? 120 : 0
                        fillMode: Image.PreserveAspectFit
                    }

                    Text {
                        id: bubbleText
                        width: bubble.width - 16
                        anchors {
                            top:        bubbleImage.visible ? bubbleImage.bottom : parent.top
                            topMargin:  bubbleImage.visible ? 4 : 8
                            left:       parent.left
                            leftMargin: 8
                        }
                        text:     model.content
                        wrapMode: Text.Wrap
                        color:    isError ? Theme.error : Theme.textPrimary
                        font { family: Theme.fontFamily; pixelSize: Theme.fontSizeNormal }
                    }
                }
            }

            // Empty state message
            Text {
                anchors.centerIn: parent
                visible:  chatModel.count === 0
                text:     "채팅을 시작하세요\n영상을 캡처하거나 질문을 입력하면 VLM이 분석합니다"
                color:    Theme.textDisabled
                font { family: Theme.fontFamily; pixelSize: Theme.fontSizeNormal }
                horizontalAlignment: Text.AlignHCenter
            }
        }

        Rectangle { height: 1; Layout.fillWidth: true; color: Theme.divider }

        // ── Captured image preview ──────────────────────────
        Rectangle {
            Layout.fillWidth:      true
            Layout.preferredHeight: 60
            Layout.maximumHeight:   60   // ColumnLayout allocation hard cap
            visible:               previewVersion > 0
            color:                 Theme.bgPanel
            border.color:          Theme.divider
            border.width:          1
            clip:                  true  // Clip overflowing content

            RowLayout {
                anchors { fill: parent; margins: 6 }
                spacing: 8

                Image {
                    width:              80
                    height:             48
                    Layout.maximumWidth:  80
                    Layout.maximumHeight: 48
                    fillMode:           Image.PreserveAspectFit
                    source:   previewVersion > 0
                              ? "image://capture/frame?v=" + previewVersion : ""
                }
                Text {
                    Layout.fillWidth: true
                    text:     "캡처된 이미지가 다음 메시지에 첨부됩니다"
                    color:    Theme.textSecondary
                    font { family: Theme.fontFamily; pixelSize: Theme.fontSizeSmall }
                    wrapMode: Text.WordWrap
                }
                StyledButton {
                    text:        "취소"
                    compact:     true
                    normalColor: Theme.btnDanger
                    hoverColor:  "#7a1c1c"
                    onClicked: {
                        root.pendingImageBase64 = ""
                        root.previewVersion     = 0
                    }
                }
            }
        }

        // ── Input area ──────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            height:           inputRow.height + 12
            color:            Theme.bgPanel

            RowLayout {
                id:       inputRow
                anchors { left: parent.left; right: parent.right; top: parent.top; margins: 6 }
                height:   Math.max(inputField.implicitHeight, 36)
                spacing:  6

                TextArea {
                    id:               inputField
                    Layout.fillWidth: true
                    placeholderText:  "질문을 입력하세요 (Shift+Enter: 줄바꿈, Enter: 전송)"
                    wrapMode:         TextArea.Wrap
                    color:            Theme.textPrimary
                    font { family: Theme.fontFamily; pixelSize: Theme.fontSizeNormal }
                    selectByMouse:    true

                    background: Rectangle {
                        color:        Theme.inputBg
                        border.color: inputField.activeFocus ? Theme.accent : Theme.inputBorder
                        radius:       Theme.radiusSmall
                    }

                    Keys.onPressed: (event) => {
                        if (event.key === Qt.Key_Return && !(event.modifiers & Qt.ShiftModifier)) {
                            event.accepted = true
                            sendBtn.clicked()
                        }
                    }
                }

                StyledButton {
                    id:       sendBtn
                    text:     (vlmBridge && vlmBridge.processing) ? "처리 중..." : "전송"
                    enabled:  !(vlmBridge && vlmBridge.processing) && inputField.text.trim().length > 0
                    onClicked: {
                        var prompt = inputField.text.trim()
                        if (prompt.length === 0) return

                        // Append user message to chat list
                        chatModel.append({
                            role:        "user",
                            content:     prompt,
                            hasImage:    pendingImageBase64.length > 0,
                            imageBase64: pendingImageBase64
                        })
                        chatListView.positionViewAtEnd()

                        // Call VLMBridge
                        if (vlmBridge) vlmBridge.sendChat(prompt, pendingImageBase64)

                        // Reset input
                        inputField.text         = ""
                        root.pendingImageBase64 = ""
                        root.previewVersion     = 0
                    }
                }

                StyledButton {
                    text:       "초기화"
                    normalColor: Theme.btnDanger
                    hoverColor:  "#7a1c1c"
                    onClicked: {
                        chatModel.clear()
                        vlmBridge.clearHistory()
                    }
                }
            }
        }
    }
}
