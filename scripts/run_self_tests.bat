@echo off
REM Builds every user-story file in features\ and runs its self-test.
REM Run from the repository root. Needs g++ and SFML 3 on PATH (e.g. MSYS2 UCRT64:
REM   set PATH=C:\msys64\ucrt64\bin;%PATH%
REM ). If SFML is in C:\SFML instead, set SFML=C:\SFML before running.
setlocal
set EXTRA=
if not "%SFML%"=="" set EXTRA=-I%SFML%\include -L%SFML%\lib
if not exist bin mkdir bin
for %%f in (features\*.cpp) do (
  g++ -std=c++17 -Wall -Wextra %%f -o bin\%%~nf.exe %EXTRA% -lsfml-graphics -lsfml-window -lsfml-system || exit /b 1
  echo == %%~nf
  bin\%%~nf.exe --test || exit /b 1
)
echo All self-tests passed.
