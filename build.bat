@echo off
chcp 65001 >nul
echo ================================
echo   编译 Mini Text Search Engine
echo ================================

if not exist bin mkdir bin

echo.
echo [1/1] 编译 module1.dll ...
gcc -shared -o bin\module1.dll src\module1_preprocess\preprocess.c -I include -lcomdlg32
if %errorlevel% neq 0 (
    echo 编译 DLL 失败！
    pause
    exit /b 1
)
echo  module1.dll 编译成功！

echo.
echo [主程序] 编译 main.exe ...
gcc -o bin\main.exe src\main.c -I include
if %errorlevel% neq 0 (
    echo 编译主程序失败！
    pause
    exit /b 1
)
echo  main.exe 编译成功！

echo.
echo ================================
echo   全部编译完成！
echo   bin\main.exe
echo   bin\module1.dll
echo ================================
pause
