# Mini Text Search Engine（轻量文本检索与关键词搜索工具）

> 一个基于 **C++ Qt6 GUI** 的轻量文本搜索引擎，采用 **DLL 架构** 实现模块化开发。  
> 支持加载文本文件夹、TF-IDF 关键词搜索、词频统计、文档管理、Benchmark 性能测试等功能。

📎 **GitHub 仓库**：https://github.com/Teriri799/project-group8.git

---

## 📦 项目架构

project-group8/
├── SearchBridge.cpp # 核心算法 DLL 源码（C 风格导出）
├── gui_main.cpp # Qt6 GUI 主程序（含 MainWindow）
├── SearchGUI.pro # Qt 项目文件
├── text_data/ # 测试文档文件夹
├── README.md # 项目说明文档
└── .gitignore # Git 忽略规则
> 本项目采用 **DLL 动态链接库** 架构，核心算法编译为 `SearchBridge64.dll`，GUI 程序通过 `QLibrary` 动态加载。

---

## 🚀 快速开始

### 环境要求

| 项目 | 说明 |
|------|------|
| **Qt 版本** | Qt 6.11.1 (MinGW 64-bit) |
| **编译器** | `D:\Qt\Tools\mingw1310_64\bin\g++.exe`（64 位 MinGW） |
| **系统** | Windows 10/11 64 位 |

### 一键编译运行


```bash
仅重新链接（修改代码后快速编译）

cd /d D:\cyuyanshijian\all\xiaoxueqishijian
set PATH=D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;%PATH%

:: 1. 生成 MOC
D:\Qt\6.11.1\mingw_64\bin\moc.exe -DUNICODE -D_UNICODE -DWIN32 ^
  -DMINGW_HAS_SECURE_API=1 -DQT_NO_DEBUG -DQT_WIDGETS_LIB -DQT_GUI_LIB ^
  -DQT_CORE_LIB -I. -ID:\Qt\6.11.1\mingw_64\include ^
  -ID:\Qt\6.11.1\mingw_64\include\QtWidgets ^
  -ID:\Qt\6.11.1\mingw_64\include\QtGui ^
  -ID:\Qt\6.11.1\mingw_64\include\QtCore ^
  -ID:\Qt\6.11.1\mingw_64\mkspecs\win32-g++ ^
  -o gui_main.moc gui_main.cpp

:: 2. 编译
D:\Qt\Tools\mingw1310_64\bin\g++.exe -c -O2 -std=gnu++1z ^
  -DQT_NO_DEBUG -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB ^
  -I. -ID:\Qt\6.11.1\mingw_64\include ^
  -ID:\Qt\6.11.1\mingw_64\include\QtWidgets ^
  -ID:\Qt\6.11.1\mingw_64\include\QtGui ^
  -ID:\Qt\6.11.1\mingw_64\include\QtCore ^
  -ID:\Qt\6.11.1\mingw_64\mkspecs\win32-g++ ^
  -o gui_main.o gui_main.cpp

:: 3. 链接
D:\Qt\Tools\mingw1310_64\bin\g++.exe -Wl,-s -Wl,-subsystem,windows ^
  -mthreads -o release\SearchGUI.exe gui_main.o ^
  "D:\Qt\6.11.1\mingw_64\lib\libQt6Widgets.a" ^
  "D:\Qt\6.11.1\mingw_64\lib\libQt6Gui.a" ^
  "D:\Qt\6.11.1\mingw_64\lib\libQt6Core.a" ^
  -lmingw32 -lshell32

:: 4. 部署并运行
copy /Y SearchBridge64.dll release\
cd release
SearchGUI.exe
cd /d D:\cyuyanshijian\all\xiaoxueqishijian
set PATH=D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;%PATH%

D:\Qt\Tools\mingw1310_64\bin\g++.exe -Wl,-s -Wl,-subsystem,windows ^
  -mthreads -o release\SearchGUI.exe gui_main.o ^
  "D:\Qt\6.11.1\mingw_64\lib\libQt6Widgets.a" ^
  "D:\Qt\6.11.1\mingw_64\lib\libQt6Gui.a" ^
  "D:\Qt\6.11.1\mingw_64\lib\libQt6Core.a" ^
  -lmingw32 -lshell32

copy /Y SearchBridge64.dll release\
cd release
SearchGUI.exe

🖥️ 界面功能
主界面布局
┌───────────────────────────────────────────────────────────┐
│                    搜索引擎 - DLL版                        │
├────────────┬──────────────────┬───────────────────────────┤
│  📊 词频   │   📁 文档列表    │      🔍 搜索              │
│   统计     │                  │                           │
│            │ [加载文件夹]     │ 查询词: [________] Top-K:  │
│ [返回全局] │ [删除选中]      │ [10] [搜索]               │
│   词频     │ [清空所有]      │                           │
│            │ [Benchmark]     │  📋 搜索结果               │
│  排名      │                  │  (#1) 文件名 (得分)       │
│  词        │  文档数: X      │    摘要内容...             │
│  频次      │  [文件列表]     │                           │
│            │                  │ [首页][上页][下页][末页]   │
│            │                  │ 跳转: [____] [GO]         │
└────────────┴──────────────────┴───────────────────────────┘

功能列表
✅ 文档管理
功能	说明
加载文件夹	选择包含文本文件的文件夹，自动加载所有支持的文本格式
删除选中	从已加载文档中移除选中的文档
清空所有	清除所有已加载文档
文件列表	左侧显示所有已加载文档及词数
✅ 搜索功能
功能	说明
关键词搜索	输入关键词，基于 TF-IDF 算法返回 Top-K 相关文档
分页显示	支持首页、上页、下页、末页、跳转至指定页
点击打开文件	搜索结果中的文件名可点击，自动用系统默认程序打开
高亮标记	排名前三的结果用红色加粗显示
✅ 词频统计
功能	说明
全局词频	显示所有文档的全局词频统计（排名、词、频次）
文件词频	点击左侧文件列表中的文件，显示该文件的词频统计
返回全局	通过"返回全局词频"按钮切换回全局统计
✅ Benchmark 批量测试
从文本文件读取关键词列表
批量执行搜索并统计性能
输出包含：总检索次数、总运行时长、单次平均耗时、最快/最慢检索
✅ 支持的文件格式
.txt  .md  .csv  .py  .cpp  .h  .c  .java  .js  .ts  .css
.html  .htm  .xml  .json  .log  .ini  .cfg  .conf

🔧 DLL 导出函数接口
函数名	功能
InitEngine()	初始化引擎，清空所有数据
LoadDocuments(path)	加载指定路径文件夹中的文档
GetDocCount()	获取文档总数
GetFileList(outBuf, bufSize)	获取文件列表（JSON 格式）
GetFileWordFreq(docId, outBuf, bufSize)	获取指定文件的词频
GetGlobalWordFreq(outBuf, bufSize)	获取全局词频
DeleteDoc(docId)	删除指定文档
DeleteAllDocs()	删除所有文档
Search(query, topK, outBuf, bufSize)	搜索关键词，返回 JSON 结果
TokenizeQuery(text, outBuf, bufSize)	分词查询文本
ReleaseEngine()	释放引擎资源

🧪 算法说明
TF-IDF 相关性评分
TF(t, d)  = 词 t 在文档 d 中出现的次数 / 文档 d 的总词数
IDF(t)    = log(1 + 总文档数 / (1 + 包含词 t 的文档数))
Score(d, q) = Σ TF(t, d) × IDF(t)   (对查询 q 中的每个词 t)

预处理流程
文本清洗 — 去除标点符号
大小写统一 — 全部转为小写
分词 — 按空格、制表符、换行符分割
过滤停用词 — 去除 a, an, the, or, and 等常见停用词
构建倒排索引 — 建立 词 → (文档ID, 词频) 的映射

🐛 开发历程 & 已解决问题
问题	解决方案
窗口不显示（编译时加了 -DQT_NEEDS_QMAIN）	去掉 -DQT_NEEDS_QMAIN 和 libQt6EntryPoint.a
链接报 undefined reference（vtable）	补回 #include "gui_main.moc"
refreshAll() 栈溢出崩溃	改为 std::vector<char> 堆分配
'\\\\' 多字符字面量 Bug	改为 QChar('\\')
DLL 加载失败（32位/64位不匹配）	使用 64 位 MinGW 编译器重新编译 DLL
点击搜索结果链接后页面变空白	添加 setOpenLinks(false) 阻止自动跳转
文件列表不显示	refreshAll() 中添加 GetFileList 调用

📝 开发日志
v1.0 — 基础功能
 命令行文本检索工具
 文本文件读取与预处理
 词频统计与倒排索引
 TF-IDF 关键词搜索
 DLL 模块化架构
v1.1 — GUI 界面
 Qt6 GUI 界面（完全鼠标点击交互）
 加载文件夹对话框
 搜索结果显示与分页
 左侧词频统计面板
 文档删除与清空
 中文路径支持
v1.2 — 功能增强
 Benchmark 批量性能测试
 搜索结果点击打开文件
 返回全局词频按钮
 文件列表显示
 搜索结果高亮标记（前三名红色加粗）

👥 团队分工
成员	负责模块	主要贡献
丘雯杰	模块1 + GUI 整合	文本文件读取与预处理、GUI 界面设计与实现、DLL接接口封装
肖晨曦	模块2	词频统计与倒排索引构建
胡可妍	模块3	DLL 关键词/短语查询解析 (QueryParse.dll)
林瑞	模块4	TF-IDF 打分、文档排序与 Top-K 截取
赵梓妍	模块5	结果输出与 Benchmark 性能测评

📄 许可证
本项目为课程设计作品，仅供学习参考。

