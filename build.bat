@echo off
chcp 65001 >nul
echo ================================
echo   编译 Mini Text Search Engine
echo ================================

if not exist dll mkdir dll
if not exist build mkdir build

echo.
echo [模块1] 编译 ReadPreprocess.dll ...
g++ -shared -o dll/ReadPreprocess.dll "第一部分/src/module1_preprocess/ReadPreprocess.cpp" -I"第一部分/include" -I"common" -std=c++17 -O2 -lcomdlg32
if %errorlevel% neq 0 (
    echo 编译 ReadPreprocess.dll 失败！
    pause
    exit /b 1
)
echo  ReadPreprocess.dll 编译成功！

echo.
echo [模块2] 编译 IndexBuild.dll ...
g++ -shared -o dll/IndexBuild.dll "第二部分/IndexBuild/IndexBuild.cpp" -I"第二部分/IndexBuild" -I"common" -std=c++17 -O2
if %errorlevel% neq 0 (
    echo 编译 IndexBuild.dll 失败！
    pause
    exit /b 1
)
echo  IndexBuild.dll 编译成功！

echo.
echo [模块3] 编译 QueryParse.dll ...
g++ -shared -o dll/QueryParse.dll "第三部分/src/QueryParse.cpp" -I"第三部分/src" -I"common" -std=c++17 -O2
if %errorlevel% neq 0 (
    echo 编译 QueryParse.dll 失败！
    pause
    exit /b 1
)
echo  QueryParse.dll 编译成功！

echo.
echo [模块4] 编译 Rank.dll ...
g++ -shared -o dll/Rank.dll "第四部分/Rank.cpp" -I"第四部分" -I"common" -std=c++17 -O2
if %errorlevel% neq 0 (
    echo 编译 Rank.dll 失败！
    pause
    exit /b 1
)
echo  Rank.dll 编译成功！

echo.
echo [模块5] 编译 OutputBench.dll ...
g++ -shared -o dll/OutputBench.dll "第五部分/OutputBench.cpp" -I"第五部分" -I"common" -std=c++17 -O2
if %errorlevel% neq 0 (
    echo 编译 OutputBench.dll 失败！
    pause
    exit /b 1
)
echo  OutputBench.dll 编译成功！

echo.
echo [主程序] 编译 main.exe ...
g++ -shared -o dll/OutputBench.dll "第五部分/OutputBench.cpp" -I"第五部分" -I"common" -std=c++17 -O2

if %errorlevel% neq 0 (
    echo 编译主程序失败！
    pause
    exit /b 1
)
echo  main.exe 编译成功！

echo.
echo ================================
echo   全部编译完成！
echo   DLL文件: dll/ 目录
echo   主程序: build/main.exe
echo ================================
pause
