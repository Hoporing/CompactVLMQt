import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs
import CompactVLMQt 1.0

// Left panel: video playback and control buttons
Item {
    id: root

    // ── External signal ────────────────────────────────────────────
    signal frameCaptured(string base64)

    // ── VideoSurface instance ─────────────────────────────────
    property alias videoSurface: videoItem

    // ── Image file state ──────────────────────────────────────
    property url loadedImageUrl: ""
    readonly property int maxImageDim: 3840

    // ── File dialog (video + image unified) ──────────────────
    readonly property var videoExts: ["mp4","avi","mkv","mov","wmv","flv","ts","webm"]
    readonly property var imageExts: ["png","jpg","jpeg","bmp","tiff","webp"]

    function openFile(fileUrl) {
        var ext = fileUrl.toString().split(".").pop().toLowerCase()
        if (videoExts.indexOf(ext) >= 0) {
            root.loadedImageUrl = ""
            imageLoader.source  = ""
            videoItem.open(fileUrl.toString().replace("file:///", ""))
        } else if (imageExts.indexOf(ext) >= 0) {
            videoItem.stop()
            root.loadedImageUrl = fileUrl
            imageLoader.source  = fileUrl
        }
    }

    FileDialog {
        id: fileDialog
        title:       "파일 선택"
        nameFilters: ["지원 파일 (*.mp4 *.avi *.mkv *.mov *.wmv *.flv *.ts *.webm *.png *.jpg *.jpeg *.bmp *.tiff *.webp)",
                      "동영상 (*.mp4 *.avi *.mkv *.mov *.wmv *.flv *.ts *.webm)",
                      "이미지 (*.png *.jpg *.jpeg *.bmp *.tiff *.webp)",
                      "모든 파일 (*)"]
        onAccepted: root.openFile(selectedFile)
    }

    // Image loader for preview only (base64 conversion done in C++)
    Image {
        id: imageLoader
        visible: false
        cache:   false
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ── Video display area ──────────────────────────────────────
        Rectangle {
            Layout.fillWidth:  true
            Layout.fillHeight: true
            color:             "black"
            clip:              true

            VideoSurface {
                id: videoItem
                anchors.fill: parent
            }

            // Static image preview (shown when image is loaded and not playing)
            Image {
                anchors.fill: parent
                fillMode:     Image.PreserveAspectFit
                smooth:       true
                cache:        false
                source:       (imageLoader.status === Image.Ready && !videoItem.playing)
                              ? imageLoader.source : ""
            }

            // Status overlay (shown when not playing and no image loaded)
            Text {
                anchors.centerIn: parent
                visible:  !videoItem.playing && imageLoader.status !== Image.Ready
                text:     "파일 열기 또는 RTSP 주소를 입력하세요"
                color:    Theme.textDisabled
                font { family: Theme.fontFamily; pixelSize: Theme.fontSizeNormal }
            }

            // FPS display
            Text {
                anchors { right: parent.right; bottom: parent.bottom; margins: 6 }
                visible: videoItem.playing
                text:    videoItem.fps.toFixed(1) + " fps"
                color:   Theme.accentAlt
                font { family: Theme.fontFamily; pixelSize: Theme.fontSizeSmall }
            }

            // Image resolution badge (top-left, shown when image is loaded)
            Rectangle {
                anchors.top:     parent.top
                anchors.left:    parent.left
                anchors.margins: 6
                width:  imgInfoText.implicitWidth + 12
                height: 20; radius: 3
                color:  "#99000000"
                visible: imageLoader.status === Image.Ready && !videoItem.playing

                Text {
                    id: imgInfoText
                    anchors.centerIn: parent
                    color:          Theme.textDisabled
                    font { family: Theme.fontFamily; pixelSize: Theme.fontSizeSmall }
                    text: {
                        var w = imageLoader.implicitWidth
                        var h = imageLoader.implicitHeight
                        var sc = Math.min(1.0, Math.min(root.maxImageDim / w, root.maxImageDim / h))
                        if (sc < 0.999)
                            return w + "\u00D7" + h + "  \u2192  "
                                   + Math.round(w * sc) + "\u00D7" + Math.round(h * sc)
                        return w + "\u00D7" + h
                    }
                }
            }
        }

        // ── RTSP input area ────────────────────────────────
        Rectangle {
            id: rtspPanel
            Layout.fillWidth: true
            height: 78
            color:  Theme.bgDark

            function connectRtsp() {
                var url = rtspField.text.trim()
                if (url.length === 0) return
                var id = rtspIdField.text.trim()
                var pw = rtspPwField.text
                if (id.length > 0 && url.indexOf("@") < 0)
                    url = url.replace("rtsp://", "rtsp://" + id + ":" + pw + "@")
                videoItem.open(url)
            }

            ColumnLayout {
                anchors { fill: parent; margins: 6 }
                spacing: 4

                // ── ID / PW row ─────────────────────────────────────
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Text {
                        text:  "ID"
                        color: Theme.textDisabled
                        font { family: Theme.fontFamily; pixelSize: Theme.fontSizeSmall }
                        verticalAlignment: Text.AlignVCenter
                    }

                    TextField {
                        id: rtspIdField
                        Layout.preferredWidth: 100
                        placeholderText: "username"
                        color:           Theme.textPrimary
                        font { family: Theme.fontFamily; pixelSize: Theme.fontSizeSmall }
                        background: Rectangle {
                            color:        Theme.inputBg
                            border.color: rtspIdField.activeFocus ? Theme.accent : Theme.inputBorder
                            radius:       Theme.radiusSmall
                        }
                    }

                    Text {
                        text:  "PW"
                        color: Theme.textDisabled
                        font { family: Theme.fontFamily; pixelSize: Theme.fontSizeSmall }
                        verticalAlignment: Text.AlignVCenter
                    }

                    TextField {
                        id: rtspPwField
                        Layout.preferredWidth: 100
                        placeholderText: "password"
                        echoMode:        TextInput.Password
                        color:           Theme.textPrimary
                        font { family: Theme.fontFamily; pixelSize: Theme.fontSizeSmall }
                        background: Rectangle {
                            color:        Theme.inputBg
                            border.color: rtspPwField.activeFocus ? Theme.accent : Theme.inputBorder
                            radius:       Theme.radiusSmall
                        }
                    }

                    Item { Layout.fillWidth: true }
                }

                // ── URL / connect row ──────────────────────────────────
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    TextField {
                        id: rtspField
                        Layout.fillWidth: true
                        placeholderText: "rtsp://192.168.0.1:554/stream"
                        color:           Theme.textPrimary
                        font { family: Theme.fontFamily; pixelSize: Theme.fontSizeSmall }
                        background: Rectangle {
                            color:        Theme.inputBg
                            border.color: rtspField.activeFocus ? Theme.accent : Theme.inputBorder
                            radius:       Theme.radiusSmall
                        }
                        onAccepted: rtspPanel.connectRtsp()
                    }

                    StyledButton {
                        text:    "연결"
                        compact: true
                        onClicked: rtspPanel.connectRtsp()
                    }
                }
            }
        }

        // ── Control button row ─────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            height:           46
            color:            Theme.bgPanel

            RowLayout {
                anchors { fill: parent; margins: 8 }
                spacing: 8

                StyledButton {
                    text:    "파일 열기"
                    onClicked: fileDialog.open()
                }

                StyledButton {
                    text:      "정지"
                    enabled:   videoItem.playing
                    onClicked: videoItem.stop()
                }

                Item { Layout.fillWidth: true }

                StyledButton {
                    text:    "Capture"
                    enabled: videoItem.playing
                    onClicked: {
                        var b64 = videoItem.grabFrameBase64()
                        if (b64.length > 0) root.frameCaptured(b64)
                    }
                }

                StyledButton {
                    text:    "이미지 전송"
                    enabled: imageLoader.status === Image.Ready && !videoItem.playing
                    onClicked: {
                        var b64 = imageUtils.loadFile(root.loadedImageUrl.toString(),
                                                      root.maxImageDim)
                        if (b64.length > 0) root.frameCaptured(b64)
                    }
                }
            }
        }
    }
}
