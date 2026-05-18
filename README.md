# CompactVLMQt

Qt6/QML 기반 VLM(Vision Language Model) 영상 Chat 애플리케이션.

RTSP 스트림 또는 웹캠 영상을 실시간으로 캡처하여 Llama 3.2 Vision 모델에 질의하고, 한국어로 번역된 응답을 채팅 UI로 표시합니다.

## Features

- **실시간 영상 스트리밍** — RTSP / 웹캠 입력, VLC 기반 프레임 캡처
- **VLM Chat** — Ollama (llama3.2-vision) 로컬 추론, 멀티턴 대화 히스토리 유지
- **한↔영 번역** — ArgosTranslate (오프라인) 또는 Ollama(qwen2.5:3b) 선택 가능
- **QML UI** — Qt Quick Controls 2, 다크 테마

## Tech Stack

| 항목 | 내용 |
|------|------|
| UI | Qt 6.10 / QML / Qt Quick Controls 2 |
| Backend | C++17 |
| VLM | Ollama — llama3.2-vision |
| Translation | ArgosTranslate (en↔ko), Ollama (qwen2.5:3b) |
| Video | libvlc (VLCFrameGrabber) |
| Build | CMake 3.20+ |

## Requirements

- Qt 6.6+ (MinGW 64-bit)
- CMake 3.20+
- [Ollama](https://ollama.com) — llama3.2-vision 모델 pull 필요
- libvlc (VLC 설치 경로 지정)
- ArgosTranslate 번역 모델 (`tools/argos_data/` 에 배치, 별도 다운로드)

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

## Usage

1. Ollama 실행 후 모델 pull
```bash
ollama pull llama3.2-vision
```
2. 앱 실행 → RTSP URL 또는 웹캠 선택
3. 영상 프레임 캡처 후 채팅창에 질의 입력

## License

본 프로젝트 코드는 MIT License.  
Qt 라이브러리는 LGPL v3 조건에 따라 동적 링크로 사용됩니다.
