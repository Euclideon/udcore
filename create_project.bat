@ECHO OFF

ECHO Select the type of project you would like to create:
ECHO 1. Visual Studio 2013 Solution
ECHO 2. Visual Studio 2012 Solution
ECHO 3. Visual Studio 2010 Solution
ECHO 4. CodeLite
ECHO 5. GNU Makefile

CHOICE /N /C:12345 /M "[1-5]:"

IF ERRORLEVEL ==5 GOTO FIVE
IF ERRORLEVEL ==4 GOTO FOUR
IF ERRORLEVEL ==3 GOTO THREE
IF ERRORLEVEL ==2 GOTO TWO
IF ERRORLEVEL ==1 GOTO ONE
GOTO END

:FIVE
 ECHO Creating GNU Makefile...
 bin\premake\premake5.exe gmake
 GOTO END
:FOUR
 ECHO Creating CodeLite Project...
 bin\premake\premake5.exe codelite
 GOTO END
:THREE
 ECHO Creating VS2010 Project...
 bin\premake\premake5.exe vs2010
 GOTO END
:TWO
 ECHO Creating VS2012 Project...
 bin\premake\premake5.exe vs2012
 GOTO END
:ONE
 ECHO Creating VS2013 Project...
 bin\premake\premake5.exe vs2013
 GOTO END

:END
