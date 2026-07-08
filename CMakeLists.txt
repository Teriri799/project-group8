cmake_minimum_required(VERSION 3.16)
project(MiniTextSearchEngine VERSION 2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 搜索所有源文件
file(GLOB_RECURSE SOURCES 
    "第一部分/src/module1_preprocess/*.cpp"
    "第二部分/IndexBuild/*.cpp"
    "第三部分/src/*.cpp"
    "第四部分/*.cpp"
    "第五部分/*.cpp"
)

# 主程序
add_executable(main "第五部分/main.cpp")
target_include_directories(main PRIVATE "common" "第一部分/include" "第二部分/IndexBuild" "第三部分/src" "第四部分" "第五部分")
target_link_libraries(main comdlg32)

# DLL 模块
add_library(ReadPreprocess SHARED "第一部分/src/module1_preprocess/ReadPreprocess.cpp")
target_include_directories(ReadPreprocess PRIVATE "common" "第一部分/include")
target_link_libraries(ReadPreprocess comdlg32)

add_library(IndexBuild SHARED "第二部分/IndexBuild/IndexBuild.cpp")
target_include_directories(IndexBuild PRIVATE "common" "第二部分/IndexBuild")

add_library(QueryParse SHARED "第三部分/src/QueryParse.cpp")
target_include_directories(QueryParse PRIVATE "common" "第三部分/src")

add_library(Rank SHARED "第四部分/Rank.cpp")
target_include_directories(Rank PRIVATE "common" "第四部分")

add_library(OutputBench SHARED "第五部分/OutputBench.cpp")
target_include_directories(OutputBench PRIVATE "common" "第五部分")
