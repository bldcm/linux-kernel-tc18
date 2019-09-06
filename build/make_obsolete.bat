rem This Version of Make was working, but it contains some obsolete functions. The current version is make.bat
@ECHO ON
echo start make %*
echo start make %* >&2

ECHO TOPCALL BuildCentral build environment: make.bat
ECHO.

setlocal
cd /d "%~dp0"


ECHO System Information:
ECHO Host:       %COMPUTERNAME%
ECHO Domain:     %USERDNSDOMAIN%
ECHO User:       %USERNAME%
ECHO.
                   
REM These directories are used to exchange files with the cygwin_shell 
REM Create subdir for file sharing in working directory
SET CYGWINSHARE=Cygwinshare
REM SET pool=\\at01cl12.topcall.co.at\pool\nave\buildserver\admin
SET pool=\\at01cl12.emea.kofax.com\pool\ISKO\buildserver\admin

REM From now on, modify the environment only locally
VERIFY other 2>nul
SETLOCAL enableextensions
IF errorlevel 1 ECHO Unable to enable command extensions; exit value may be incorrect

REM Remember working dir for cleanup
REM Do not cd from now until bash is called
SET workingdir=%cd%
IF EXIST %CYGWINSHARE% (
	ECHO Warning: Directory %workingdir%\%CYGWINSHARE% already exists
	ECHO Existing files may be overwritten
) ELSE (
	MKDIR %CYGWINSHARE%
	set CLEANUP=yes
)

REM Copy any files from the pool to the local share, e.g. admin scripts
REM First test if the directory exists and is not empty 
ECHO Checking for shared files in %pool% >&2
DIR %pool% /a-d 1>nul 2>nul
IF %errorlevel%==0 (
	ECHO Copying shared files to %workingdir%\%CYGWINSHARE%
	COPY %pool%\*.* %workingdir%\%CYGWINSHARE%\ /y
) >&2
                   
PATH %PATH%;C:\cygwin\bin

ECHO.
ECHO Passing the build script to the cygwin-shell >&2
ECHO. 

SET executable=checkout_and_build
SET buildserver=10.20.30.69
SET builduser=tcbuild 
SET buildversion=%1

bash cygwin_shell %executable% %buildserver% %builduser% %buildversion% >&2
SET exitcode=%errorlevel%

rem check if build was started from build sub-directory
if not exist ../build/make.bat goto get_result_files

rem We are now in directory .../linux_kernel_tc18/build (%cd%)
rem and we have to move result files from import\export to export directory
dir export
md ..\export
move export\*.* ..\export

:get_result_files

echo Get back the result files, if any
ECHO.
REM ECHO make.bat: Checking for result files in %workingdir%\%CYGWINSHARE%
DIR %workingdir%\%CYGWINSHARE% /a-d 1>nul 2>nul
IF %errorlevel%==0 (
 	ECHO Copying result files to %pool% >&2
 	COPY %workingdir%\%CYGWINSHARE%\*.* %pool% /y
)

echo Check for Cleanup (cd=%CD%, %CLEANUP%) >&2
IF "%CLEANUP%"=="yes" (
	ECHO Cleaning up %workingdir%\%CYGWINSHARE% >&2
	DEL %workingdir%\%CYGWINSHARE%\*.* /q
	RMDIR %workingdir%\%CYGWINSHARE% /q 
)

ECHO.  
ECHO Finished make.bat with exit code %exitcode% >&2
ECHO.
   
EXIT /b %exitcode%

