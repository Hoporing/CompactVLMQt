; ============================================================
; CompactVLM Installer Script
; NSIS 3.x  (makensis CompactVLMQt.nsi)
; Uses built-in NSISdl plugin for downloads (no extra plugins required)
; One-click flow:
;   1) Install app files
;   2) Download & install Ollama (skip if already installed)
;   3) Wait for Ollama service to be ready
;   4) Pull llama3.2-vision model (skip if already present)
; ============================================================

Unicode True
!include "MUI2.nsh"
!include "LogicLib.nsh"

; ── Basic info ────────────────────────────────────────────
!define APP_NAME        "CompactVLM"
!define APP_VERSION     "1.0.0"
!define APP_PUBLISHER   "CompactVLM"
!define APP_EXE         "CompactVLMQt.exe"
!define OLLAMA_EXE      "OllamaSetup.exe"
!define OLLAMA_URL      "https://ollama.com/download/OllamaSetup.exe"
!define OLLAMA_PATH     "$LOCALAPPDATA\Programs\Ollama\ollama.exe"
!define OLLAMA_MODEL    "llama3.2-vision"
!define OLLAMA_MANIFEST "$PROFILE\.ollama\models\manifests\registry.ollama.ai\library\llama3.2-vision\latest"
!define UNINSTALL_KEY   "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}"
!define INSTALL_KEY     "Software\${APP_NAME}"
!define BUILD_DIR       "..\build"

Name            "${APP_NAME}"
OutFile         "${APP_NAME}-${APP_VERSION}-Setup.exe"
InstallDir      "$PROGRAMFILES64\${APP_NAME}"
InstallDirRegKey HKLM "${INSTALL_KEY}" "InstallDir"
RequestExecutionLevel admin
BrandingText    "${APP_NAME} ${APP_VERSION}"
Icon            "..\resources\app.ico"
UninstallIcon   "..\resources\app.ico"

; ── MUI settings ──────────────────────────────────────────
!define MUI_ICON   "..\resources\app.ico"
!define MUI_UNICON "..\resources\app.ico"
!define MUI_ABORTWARNING
!define MUI_FINISHPAGE_RUN         "$INSTDIR\${APP_EXE}"
!define MUI_FINISHPAGE_RUN_TEXT    "CompactVLM 시작"
!define MUI_WELCOMEPAGE_TEXT       "CompactVLM ${APP_VERSION} 설치를 시작합니다.$\r$\n$\r$\n\
영상 분석 AI 솔루션 (VLC 스트리밍 + Ollama VLM + 번역 서버)$\r$\n$\r$\n\
다음 항목이 자동으로 설치됩니다:$\r$\n\
  - CompactVLM 애플리케이션$\r$\n\
  - Ollama (AI 엔진, 약 150MB)$\r$\n\
  - llama3.2-vision 모델 (약 8GB)$\r$\n$\r$\n\
인터넷 연결이 필요하며, 모델 다운로드에 수분 이상 소요될 수 있습니다."

; ── Pages ─────────────────────────────────────────────────
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; ── Language ──────────────────────────────────────────────
!insertmacro MUI_LANGUAGE "Korean"
!insertmacro MUI_LANGUAGE "English"

; ============================================================
; Function: InstallOllama
; Downloads and silently installs Ollama if not present.
; ============================================================
Function InstallOllama

    IfFileExists "${OLLAMA_PATH}" OllamaAlreadyInstalled

    DetailPrint "Ollama 다운로드 중... (약 150MB)"
    NSISdl::download "${OLLAMA_URL}" "$TEMP\${OLLAMA_EXE}"
    Pop $R0
    ${If} $R0 != "success"
        MessageBox MB_OK \
            "Ollama 다운로드 실패: $R0$\r$\n$\r$\n\
설치 완료 후 https://ollama.com 에서 수동으로 설치하세요.$\r$\n\
Ollama 없이는 AI 기능을 사용할 수 없습니다."
        Goto OllamaDone
    ${EndIf}

    DetailPrint "Ollama 설치 중..."
    ExecWait '"$TEMP\${OLLAMA_EXE}" /VERYSILENT /SUPPRESSMSGBOXES /NORESTART'
    Delete "$TEMP\${OLLAMA_EXE}"

    IfFileExists "${OLLAMA_PATH}" OllamaInstalled
    MessageBox MB_OK \
        "Ollama 설치 확인 실패.$\r$\n\
설치 완료 후 https://ollama.com 에서 수동으로 설치하세요."
    Goto OllamaDone

    OllamaInstalled:
        DetailPrint "Ollama 설치 완료."
        Goto OllamaDone

    OllamaAlreadyInstalled:
        DetailPrint "Ollama가 이미 설치되어 있습니다. 건너뜁니다."

    OllamaDone:

FunctionEnd

; ============================================================
; Function: WaitForOllamaService
; Waits up to ~30s for the Ollama HTTP service to become ready.
; Sets $R9 = "ready" or "timeout".
; ============================================================
Function WaitForOllamaService

    StrCpy $R9 "timeout"
    StrCpy $R1 0    ; retry counter

    ; Ensure ollama app is running (it self-serves on first launch)
    IfFileExists "${OLLAMA_PATH}" 0 ServiceCheckDone
    Exec '"${OLLAMA_PATH}" serve'

    ServiceWaitLoop:
        IntCmp $R1 15 ServiceCheckDone   ; max 15 retries × 2s = 30s
        Sleep 2000
        nsExec::ExecToStack '"${OLLAMA_PATH}" list'
        Pop $R0   ; exit code (0 = service ready)
        Pop $R2   ; stdout (discard)
        ${If} $R0 == "0"
            StrCpy $R9 "ready"
            Goto ServiceCheckDone
        ${EndIf}
        IntOp $R1 $R1 + 1
        DetailPrint "Ollama 서비스 대기 중... ($R1/15)"
        Goto ServiceWaitLoop

    ServiceCheckDone:

FunctionEnd

; ============================================================
; Function: InstallOllamaModel
; Pulls llama3.2-vision if not already present.
; ============================================================
Function InstallOllamaModel

    IfFileExists "${OLLAMA_MANIFEST}" ModelAlreadyExists

    ; Wait for Ollama service first
    DetailPrint "Ollama 서비스 시작 대기 중..."
    Call WaitForOllamaService
    ${If} $R9 != "ready"
        MessageBox MB_OK \
            "Ollama 서비스가 응답하지 않습니다.$\r$\n$\r$\n\
설치 완료 후 명령 프롬프트에서 직접 실행하세요:$\r$\n\
  ollama pull ${OLLAMA_MODEL}"
        Goto ModelDone
    ${EndIf}

    DetailPrint "llama3.2-vision 모델 다운로드 중... (약 8GB, 수분 이상 소요)"
    DetailPrint "아래 로그에서 진행 상황을 확인할 수 있습니다."
    nsExec::ExecToLog '"${OLLAMA_PATH}" pull ${OLLAMA_MODEL}'
    Pop $R0
    ${If} $R0 != "0"
        MessageBox MB_OK \
            "모델 다운로드 중 오류가 발생했습니다.$\r$\n$\r$\n\
설치 완료 후 명령 프롬프트에서 직접 실행하세요:$\r$\n\
  ollama pull ${OLLAMA_MODEL}"
        Goto ModelDone
    ${EndIf}
    DetailPrint "llama3.2-vision 모델 준비 완료."
    Goto ModelDone

    ModelAlreadyExists:
        DetailPrint "llama3.2-vision 모델이 이미 존재합니다. 건너뜁니다."

    ModelDone:

FunctionEnd

; ============================================================
; Install section
; ============================================================
Section "CompactVLM" SecMain
    SectionIn RO

    ; ── App root files ─────────────────────────────────────
    SetOutPath "$INSTDIR"
    File "${BUILD_DIR}\${APP_EXE}"
    File "${BUILD_DIR}\translate_server.exe"
    File "${BUILD_DIR}\*.dll"

    ; ── Qt plugin folders ──────────────────────────────────
    SetOutPath "$INSTDIR\platforms"
    File /r "${BUILD_DIR}\platforms\*"

    SetOutPath "$INSTDIR\imageformats"
    File /r "${BUILD_DIR}\imageformats\*"

    SetOutPath "$INSTDIR\iconengines"
    File /r "${BUILD_DIR}\iconengines\*"

    SetOutPath "$INSTDIR\generic"
    File /r "${BUILD_DIR}\generic\*"

    SetOutPath "$INSTDIR\networkinformation"
    File /r "${BUILD_DIR}\networkinformation\*"

    SetOutPath "$INSTDIR\tls"
    File /r "${BUILD_DIR}\tls\*"

    ; ── QML modules ───────────────────────────────────────
    SetOutPath "$INSTDIR\qml"
    File /r "${BUILD_DIR}\qml\*"

    ; ── VLC plugins ───────────────────────────────────────
    SetOutPath "$INSTDIR\plugins"
    File /r "${BUILD_DIR}\plugins\*"

    ; ── Translation server data (Argos Translate models) ──
    SetOutPath "$INSTDIR\argos_data"
    File /r "${BUILD_DIR}\argos_data\*"

    ; ── Ollama: install app, then pull model ──────────────
    SetOutPath "$INSTDIR"
    Call InstallOllama
    Call InstallOllamaModel

    ; ── Shortcuts ─────────────────────────────────────────
    CreateDirectory "$SMPROGRAMS\${APP_NAME}"
    CreateShortcut "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk" \
        "$INSTDIR\${APP_EXE}" "" "$INSTDIR\${APP_EXE}" 0
    CreateShortcut "$SMPROGRAMS\${APP_NAME}\제거.lnk" \
        "$INSTDIR\Uninstall.exe"

    ; ── Uninstaller + registry ────────────────────────────
    WriteUninstaller "$INSTDIR\Uninstall.exe"

    WriteRegStr  HKLM "${INSTALL_KEY}"    "InstallDir"      "$INSTDIR"
    WriteRegStr  HKLM "${UNINSTALL_KEY}"  "DisplayName"     "${APP_NAME}"
    WriteRegStr  HKLM "${UNINSTALL_KEY}"  "UninstallString" '"$INSTDIR\Uninstall.exe"'
    WriteRegStr  HKLM "${UNINSTALL_KEY}"  "DisplayVersion"  "${APP_VERSION}"
    WriteRegStr  HKLM "${UNINSTALL_KEY}"  "Publisher"       "${APP_PUBLISHER}"
    WriteRegStr  HKLM "${UNINSTALL_KEY}"  "DisplayIcon"     "$INSTDIR\${APP_EXE}"
    WriteRegDWORD HKLM "${UNINSTALL_KEY}" "NoModify"        1
    WriteRegDWORD HKLM "${UNINSTALL_KEY}" "NoRepair"        1

SectionEnd

; ============================================================
; Uninstall section  (Ollama is NOT removed - may be shared)
; ============================================================
Section "Uninstall"

    Delete "$INSTDIR\${APP_EXE}"
    Delete "$INSTDIR\translate_server.exe"
    Delete "$INSTDIR\*.dll"
    Delete "$INSTDIR\Uninstall.exe"

    RMDir /r "$INSTDIR\platforms"
    RMDir /r "$INSTDIR\imageformats"
    RMDir /r "$INSTDIR\iconengines"
    RMDir /r "$INSTDIR\generic"
    RMDir /r "$INSTDIR\networkinformation"
    RMDir /r "$INSTDIR\tls"
    RMDir /r "$INSTDIR\qml"
    RMDir /r "$INSTDIR\plugins"
    RMDir /r "$INSTDIR\argos_data"

    RMDir "$INSTDIR"

    Delete "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk"
    Delete "$SMPROGRAMS\${APP_NAME}\제거.lnk"
    RMDir  "$SMPROGRAMS\${APP_NAME}"

    DeleteRegKey HKLM "${UNINSTALL_KEY}"
    DeleteRegKey HKLM "${INSTALL_KEY}"

SectionEnd
