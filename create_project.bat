@ECHO OFF

ECHO Select the type of project you would like to create:
ECHO 1. Visual Studio 2019 Solution
ECHO 2. Visual Studio 2019 Solution (Android)
ECHO 3. Visual Studio 2015 Solution (Emscripten)
ECHO 4. Visual Studio 2015 Solution

CHOICE /N /C:1234 /M "[1-4]:"

IF ERRORLEVEL ==4 GOTO FOUR
IF ERRORLEVEL ==3 GOTO THREE
IF ERRORLEVEL ==2 GOTO TWO
IF ERRORLEVEL ==1 GOTO ONE
GOTO END

:FOUR
 ECHO Creating VS2015 Project...
 bin\premake-bin\premake5.exe vs2015
 GOTO END
:THREE
 ECHO Creating VS2015 Emscripten Project...
 bin\-bin\premake5.exe vs2015 --os=emscripten
 GOTO END
:TWO
 ECHO Creating VS2019 Android Project...
 bin\premake-bin\premake5.exe vs2019 --os=android
 GOTO END
:ONE
 ECHO Creating VS2019 Project...
 bin\premake-bin\premake5.exe vs2019
 GOTO END

:END
