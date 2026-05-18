CompactVLM Installation Guide

1.  Installation Overview

Current Installation Structure

Main Program - Application + Translations - Offline mode

Ollama - Application + Model - Online mode

2.  Required Files

CompactVLM-1.0.0-Setup.exe - Installer executable

3.  Build Scripts

CompactVLMQt.nsi - NSIS script used to build the installer

add_bom.ps1 - PowerShell script that adds BOM to the NSIS script

Build Procedure:

1.  Open CompactVLMQt.nsi using HM NIS Edit or another NSIS editor.

2.  Run add_bom.ps1 if BOM needs to be applied.

3.  Build the installer using makensisw.

4.  Optional Components (For Offline Installation)

OllamaSetup.exe - Ollama installer

VisionLM Model - Must be placed under:
C:`\Users`{=tex}`\user`{=tex}.ollama`\models`{=tex}

5.  Offline Installation Procedure

6.  Install the main program using CompactVLM-1.0.0-Setup.exe.

7.  Install Ollama using OllamaSetup.exe.

8.  Copy the VisionLM model files to:
    C:`\Users`{=tex}`\user`{=tex}.ollama`\models`{=tex}

9.  Launch the application.
