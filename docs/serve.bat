@echo off
cd /d "%~dp0"
call "%~dp0..\.venv\Scripts\activate.bat"
set MKDOCS_EXE=%~dp0..\.venv\Scripts\mkdocs.exe
if not exist "%MKDOCS_EXE%" (
	echo mkdocs.exe not found: %MKDOCS_EXE%
	pause
	exit /b 1
)
"%MKDOCS_EXE%" serve --livereload --watch docs
pause
