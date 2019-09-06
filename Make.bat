@setlocal ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION
@CALL %MAKEVERSION_DIR%\ModuleBuildEntry "linux-kernel-tc18" - %*
CALL %MAKEVERSION_DIR%\SetVersionEnv v4 %1

set AGENT=Agent465
echo %TIME% Start build of linux-kernel-tc18 %* >linux-build.log

echo Cleanup ..
if exist tc18.zip         del tc18.zip
if exist export           rd /s /q export 

echo Create ZIP file ...
%ZIP_CMD% a -r -tZip -x^^!linux-build.log tc18.zip >>linux-build.log

echo Cleanup working folder ...
call %MAKEVERSION_DIR%\Linux 112 Run "cd TFS-Linux && pwd && ls -la && if [ -d %AGENT% ]; then sudo rm -rf %AGENT%; fi && ls -la && mkdir -p %AGENT%/LineServer && ls -la"
if errorlevel 1 goto build_error

echo Copy ZIP to Linux server ...
call %MAKEVERSION_DIR%\Linux 112 Copy "tc18.zip" "{LINUX-USER@LINUX-HOST}:TFS-Linux/%AGENT%/LineServer"

echo Extract ZIP file on Linux Server ...
call %MAKEVERSION_DIR%\Linux 142 Run "cd TFS-Linux/%AGENT%/LineServer && pwd && ls -la && unzip -d linux-kernel-tc18 -o tc18.zip"

echo Change permissions ...
call %MAKEVERSION_DIR%\Linux 112 Run "cd TFS-Linux/%AGENT% && ls -la && chmod -R 777 * && ls -la"

echo Convert DOS to Unix ...
call %MAKEVERSION_DIR%\Linux 110 Run "cd TFS-Linux/%AGENT%/LineServer/linux-kernel-tc18 && ls -la && find . -type f -exec dos2unix {} \;"

echo Run build ...
call %MAKEVERSION_DIR%\Linux 112 Run "cd TFS-Linux/%AGENT%/LineServer/linux-kernel-tc18 && ls -la && build/checkout_and_build n/a %VERSION%"

echo Get result ...
call %MAKEVERSION_DIR%\Linux 112 Copy -r "{LINUX-USER@LINUX-HOST}:TFS-Linux/%AGENT%/LineServer/linux-kernel-tc18/build/results/export" "."

call %MAKEVERSION_DIR%\AssertFile export\system18.img export\system18.sre

:exit_build
%MAKEVERSION_DIR%\ModuleBuildExit
rem This line is never reached

:build_error
rem Avoid further builds after an error has been detected.
set BUILD_ERROR_FLAG=1
%MAKEVERSION_DIR%\ModuleBuildExit
