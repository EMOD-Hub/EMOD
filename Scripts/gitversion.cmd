:: Get commit/branch info from git for build version info
:: Expects one argument - path for utils directory

@ECHO OFF

IF [%1]==[] ECHO %0: Missing required path to utils project directory. & GOTO NOPATH

SET TEMPLATE_FILENAME=%1\version_info.tmpl
SET HEADER_FILENAME=%1\version_info.h

ECHO Looking for template file at '%TEMPLATE_FILENAME%'

IF NOT EXIST %TEMPLATE_FILENAME% ECHO %0: Didn't find template file at '%TEMPLATE_FILENAME%' & GOTO NOTEMPLATE

SET TEMP_SCRATCH_FILE="%TEMP%\version.%RANDOM%.txt"

CALL where git.exe > NUL: 2> NUL:
IF %ERRORLEVEL% NEQ 0 ECHO Didn't find git.exe to gather commit information. & GOTO NOGIT

CALL git status > NUL: 2> NUL:
IF %ERRORLEVEL% NEQ 0 ECHO Doesn't appear to be a Git repository. & GOTO NOGIT

ECHO %USERNAME%> %TEMP_SCRATCH_FILE%

:: Seed temp version info file with branch name
git rev-parse --abbrev-ref HEAD >> %TEMP_SCRATCH_FILE%

:: Append short commit hash and date to version info file (use --pretty=format to prevent extra newline)
git show --no-patch --pretty="format:%%h%%n%%ai%%n" HEAD >> %TEMP_SCRATCH_FILE%

:: Append length (count) of commit chain to version info file for revision number
git rev-list --count HEAD >> %TEMP_SCRATCH_FILE%

GOTO TEMPLATE

:NOGIT

:: butt redirects up against text to prevent extraneous spacing
ECHO %USERNAME%> %TEMP_SCRATCH_FILE%
ECHO unknown-branch>> %TEMP_SCRATCH_FILE%
ECHO unknown>> %TEMP_SCRATCH_FILE%
ECHO date time unknown>> %TEMP_SCRATCH_FILE%
ECHO 00>> %TEMP_SCRATCH_FILE%

GOTO TEMPLATE

:TEMPLATE

SET TEMP_HEADER_FILE="%TEMP%\version_info.%RANDOM%.h"

:: Use Windows [Console] Script Host to execute jscript which replaces macros in the template with actual info
:: %~dp0 represents the path of this script, gitversion.js should be in the same directory
cscript //NoLogo %~dp0\gitversion.js %TEMP_SCRATCH_FILE% %TEMPLATE_FILENAME% %TEMP_HEADER_FILE%

fc %TEMP_HEADER_FILE% %HEADER_FILENAME%
IF %ERRORLEVEL% NEQ 0 (
    ECHO Writing new %HEADER_FILENAME% file
    MOVE /Y %TEMP_HEADER_FILE% %HEADER_FILENAME%
) ELSE (
    ECHO Skipping write of %HEADER_FILENAME% file; nothing changed
    DEL %TEMP_HEADER_FILE%
)

GOTO EXITOK

:EXITOK
EXIT 0

:NOPATH
EXIT 1

:NOTEMPLATE
EXIT 2
