@echo off
cd /d "%~dp0"
call .venv\Scripts\activate.bat
mkdocs serve
pause
