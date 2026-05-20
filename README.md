<p align="center">
  <img src="assets/Hoporing_Banner.png" alt="CompactVLMQt Banner" width="100%"/>
</p>

# CompactVLMQt

A desktop application for real-time visual question answering over live video streams and local media files.  
Built with **Qt 6 / QML** and **libVLC**, it captures frames from RTSP cameras or video files and sends them to a locally running **Ollama** VLM (Vision Language Model) for analysis.

---

## Demo

| Church scene | Golf swing |
|:---:|:---:|
| ![church](assets/church.gif) | ![golf](assets/golf.gif) |

> Frames captured from video are sent to **llama3.2-vision** running locally via Ollama.  
> Responses are translated to Korean using **qwen2.5:3b** and displayed in the chat panel.

---

## Features

- **Live RTSP streaming** — connect to IP cameras with ID/PW authentication
- **Video file playback** — MP4, AVI, MKV, MOV, WMV, FLV, TS, WEBM
- **Still image input** — PNG, JPG, BMP, TIFF, WEBP (large images auto-scaled)
- **Frame capture** — one-click capture sends the current frame as a PNG to the VLM
- **Multi-turn VLM chat** — conversation history preserved across messages
- **Korean translation** — VLM responses (English) are translated to Korean via qwen2.5:3b
- **Letterbox rendering** — aspect ratio preserved for both landscape and portrait video
- **Dark-theme QML UI** — resizable SplitView, FPS overlay, image resolution badge

---

## Architecture

```
RTSP / Video File / Image
         │
 ┌───────┴────────────────────────────────────┐
 │           VLCFrameGrabber                  │
 │  libvlc vmem API                           │
 │  VideoSetup  → I420 buffer allocation      │
 │  VideoLock   → provide Y/U/V plane ptrs    │
 │  VideoDisplay→ I420 → RGB888 (QImage)      │
 │  Frame queue + ProcFrame thread (FPS pace) │
 └───────────────────────┬────────────────────┘
                         │ QImage
                    VideoPlayer
                         │
               VideoSurface (QQuickPaintedItem)
               QPainter letterbox render @ 60 Hz
                         │
              QML VideoPanel
                         │ grabFrameBase64()
                    base64 PNG
                  ┌──────┴──────┐
     CaptureImageProvider    ChatPanel (QML)
     image://capture/frame       │
      (thumbnail preview)   VLMBridge (QML_ELEMENT)
                                 │  QtConcurrent
                            VLMEngine
                            ├─ Ollama llama3.2-vision  (vision chat)
                            └─ Ollama qwen2.5:3b        (EN → KO)
```

**Key design decisions**

- `libvlc_video_set_format_callbacks` (not `set_format`) — VLC calls our setup callback on every resize event, so the buffer always matches the actual decoded dimensions.  
  This prevents buffer overflows when the source changes resolution mid-stream.
- Cap the longer dimension at 1920 px — works for both landscape (3840×2160 → 1920×1080) and portrait (2160×3840 → 1080×1920) without distortion.
- `VLMBridge::sendChat` runs on `QtConcurrent::run` so Ollama inference never blocks the UI thread.

---

## Tech Stack

| Component | Technology |
|-----------|------------|
| UI | Qt 6.11 / QML / Qt Quick Controls 2 |
| Language | C++17 |
| Video decode | libVLC 3.0 (vmem output, hardware-accelerated D3D11VA) |
| VLM inference | Ollama — llama3.2-vision |
| Translation | Ollama — qwen2.5:3b |
| HTTP client | ollama-hpp (header-only) |
| Build | CMake 3.20+ / Ninja |
| Platform | Windows (MSVC 2022 x64) |

---

## Requirements

- **Qt 6.6+** (MSVC 2022 64-bit)
- **CMake 3.20+** with Ninja
- **libVLC 3.0** — headers at `C:/libVLC/include`, import libs at `C:/libVLC/lib`
- **Ollama** — `llama3.2-vision` and `qwen2.5:3b` models pulled

```bash
ollama pull llama3.2-vision
ollama pull qwen2.5:3b
```

---

## Build

```bash
# Configure (Qt 6.11.1 MSVC2022 64-bit, Ninja generator)
cmake -B build -G Ninja ^
      -DCMAKE_BUILD_TYPE=Release ^
      -DQt6_DIR="C:/Qt/6.11.1/msvc2022_64/lib/cmake/Qt6"

# Build
cmake --build build --config Release
```

VLC DLLs and plugins are copied to the output directory automatically via a CMake post-build step.  
Qt runtime is deployed via `windeployqt6` automatically on Windows.

---

## Usage

1. Start Ollama  
   ```bash
   ollama serve
   ```
2. Launch `CompactVLMQt.exe` — Ollama initializes automatically on startup.
3. **Open video**: click **파일 열기** to open a local file, or enter an RTSP URL and click **연결**.
4. **Capture frame**: click **Capture** — the frame thumbnail appears above the chat input.
5. **Ask a question**: type in the chat box and press **Enter** (or click **전송**).  
   The frame is attached to the message and sent to llama3.2-vision.
6. **Still image**: open an image file and click **이미지 전송** to send it directly to the VLM.

---

## Project Structure

```
CompactVLMQt/
├── src/
│   ├── VLCFrameGrabber.{h,cpp}   # libvlc vmem pipeline, I420→RGB conversion
│   ├── VideoPlayer.{h,cpp}        # VLCFrameGrabber lifetime management
│   ├── VideoSurface.{h,cpp}       # QQuickPaintedItem, letterbox render, capture
│   ├── VLMEngine.{h,cpp}          # Ollama API, chat history, translation
│   ├── VLMBridge.{h,cpp}          # QML-exposed bridge (QML_ELEMENT)
│   ├── CaptureImageProvider.{h,cpp} # image://capture/ provider for QML preview
│   └── ImageUtils.{h,cpp}         # C++ image loader for QML (large file safe)
├── qml/
│   ├── Main.qml                   # ApplicationWindow, SplitView
│   ├── VideoPanel.qml             # Left panel: video, RTSP input, controls
│   ├── ChatPanel.qml              # Right panel: chat list, input, preview
│   ├── Theme.qml                  # Dark theme singleton
│   ├── StyledButton.qml
│   └── AboutDialog.qml
├── assets/                        # Demo GIFs and banner
├── tools/                         # translate_server.exe (ArgosTranslate wrapper)
└── CMakeLists.txt
```

---

## License

Application source code: **MIT License**  
Qt libraries are dynamically linked under **LGPL v3**.  
libVLC is dynamically linked under **LGPL v2.1**.
