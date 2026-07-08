#include "../common/common.h"
#include <Windows.h>
#include <iostream>
#include <string>
#include <chrono>
#include <set>
#include <fstream>
#include <sstream>
#include <conio.h>
#include <map>
using namespace std;

// ========== DLL 函数指针 ==========
typedef vector<DocInfo>(*Func_LoadDoc)(const string&);
typedef tuple<InvertIndex, vector<int>, int>(*Func_BuildIndex)(const vector<DocInfo>&);
typedef vector<string>(*Func_QuerySplit)(const string&);
typedef set<int>(*Func_PhraseSearch)(const InvertIndex&, const vector<string>&);
typedef ScorePair(*Func_GetTopK)(set<int>, vector<string>, InvertIndex, vector<int>, int, int);
typedef void(*Func_PrintRes)(const ScorePair&, const vector<DocInfo>&);
typedef string(*Func_TextClean)(string);
typedef vector<string>(*Func_SplitWord)(string);
typedef vector<DocInfo>(*Func_LoadDialog)(void);

// ========== 全局变量 ==========
HMODULE hReadPre, hIndex, hQuery, hRank, hOutput;
Func_LoadDoc LoadDoc = nullptr;
Func_BuildIndex BuildIndex = nullptr;
Func_QuerySplit QuerySplit = nullptr;
Func_PhraseSearch PhraseSearch = nullptr;
Func_GetTopK GetTopK = nullptr;
Func_PrintRes PrintResult = nullptr;
Func_TextClean TextClean = nullptr;
Func_SplitWord SplitWord = nullptr;
Func_LoadDialog LoadDialog = nullptr;

vector<DocInfo> allDocs;
InvertIndex invertIndex;
vector<int> docWordLenArr;
int totalDocNum = 0;
string currentDataFolder = "";

// ========== 工具函数 ==========
void setColor(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

void gotoxy(int x, int y) {
    COORD coord = { (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

void clearLine(int y) {
    gotoxy(0, y);
    for (int i = 0; i < 60; i++) cout << " ";
}

void printCentered(string text, int y) {
    gotoxy((60 - text.length()) / 2, y);
    cout << text;
}

void showStatus() {
    if (totalDocNum > 0) {
        setColor(14);
        printCentered("[ 当前文档数: " + to_string(totalDocNum) + " | 数据源: " + currentDataFolder + " ]", 2);
        setColor(7);
    }
}

void rebuildIndex(const string& folder) {
    currentDataFolder = folder;
    allDocs = LoadDoc(folder);
    auto result = BuildIndex(allDocs);
    invertIndex = std::get<0>(result);
    docWordLenArr = std::get<1>(result);
    totalDocNum = std::get<2>(result);
}

void drawBox(int y1, int y2, int w) {
    setColor(11);
    for (int y = y1; y <= y2; y++) {
        gotoxy(0, y); cout << "║";
        gotoxy(w, y); cout << "║";
    }
    gotoxy(0, y1); cout << "╔"; gotoxy(w, y1); cout << "╗";
    gotoxy(0, y2); cout << "╚"; gotoxy(w, y2); cout << "╝";
    setColor(7);
}

// ========== 菜单显示 ==========
void showMenu(int selected) {
    system("cls");
    setColor(11);
    cout << "  ╔══════════════════════════════════╗" << endl;
    cout << "  ║   Mini Text Search Engine v2.0   ║" << endl;
    cout << "  ║   轻量文本检索与搜索工具         ║" << endl;
    cout << "  ╚══════════════════════════════════╝" << endl;
    setColor(7);
    showStatus();
    cout << endl;
    
    string items[] = {
        "📁 输入文件/文件夹路径",
        "📂 拖入文件或文件夹",
        "📄 弹出文件选择对话框",
        "✏️  文本框直接输入文本",
        "🔍 关键词检索",
        "💬 短语精确检索",
        "⚡ Benchmark 性能测试",
        "🧪 开发测试模式",
        "❌ 退出程序"
    };
    
    int colors[] = {10,10,10,14,9,9,14,8,12};
    
    for (int i = 0; i < 9; i++) {
        gotoxy(5, 6 + i);
        if (i == selected) {
            setColor(15);
            cout << "▸ " << items[i] << " ◂";
        } else {
            setColor(colors[i]);
            cout << "  " << items[i] << "  ";
        }
        setColor(7);
    }
    
    setColor(8);
    gotoxy(2, 16);
    cout << "↑↓ 选择  回车确认" << endl;
    setColor(7);
}

// ========== 词频统计（去重+计数） ==========
void printWordFreq(const vector<string>& words) {
    map<string, int> freq;
    for (const auto& w : words) freq[w]++;
    
    setColor(14);
    cout << "\n  分词结果 (去重统计):" << endl;
    setColor(7);
    for (const auto& [word, count] : freq) {
        setColor(10);
        cout << "    " << word;
        setColor(8);
        if (count > 1) cout << " (x" << count << ")";
        cout << endl;
    }
    setColor(14);
    cout << "  共 " << freq.size() << " 个不同词条" << endl;
    setColor(7);
}

// ========== 主函数 ==========
int main(int argc, char* argv[])
{
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    
    // 隐藏光标
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
    cursorInfo.bVisible = false;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
    
    // 加载 DLL
    hReadPre = LoadLibraryA("dll/ReadPreprocess.dll");
    hIndex = LoadLibraryA("dll/IndexBuild.dll");
    hQuery = LoadLibraryA("dll/QueryParse.dll");
    hRank = LoadLibraryA("dll/Rank.dll");
    hOutput = LoadLibraryA("dll/OutputBench.dll");
    
    if (!hReadPre || !hIndex || !hQuery || !hRank || !hOutput) {
        system("cls");
        setColor(12);
        cout << "DLL加载失败! 请确保 dll/ 目录下所有文件完整" << endl;
        setColor(7);
        system("pause"); 
        return -1;
    }
    
    LoadDoc = (Func_LoadDoc)GetProcAddress(hReadPre, "LoadAndPreprocessFile");
    BuildIndex = (Func_BuildIndex)GetProcAddress(hIndex, "BuildInvertIndex");
    QuerySplit = (Func_QuerySplit)GetProcAddress(hQuery, "QuerySplit");
    PhraseSearch = (Func_PhraseSearch)GetProcAddress(hQuery, "PhraseSearch");
    GetTopK = (Func_GetTopK)GetProcAddress(hRank, "GetTopKResult");
    PrintResult = (Func_PrintRes)GetProcAddress(hOutput, "PrintSearchResult");
    TextClean = (Func_TextClean)GetProcAddress(hReadPre, "TextClean");
    SplitWord = (Func_SplitWord)GetProcAddress(hReadPre, "SplitWord");
    LoadDialog = (Func_LoadDialog)GetProcAddress(hReadPre, "LoadSingleFileViaDialog");
    
    int selected = 0;
    int key;
    
    while (true) {
        showMenu(selected);
        
        key = _getch();
        if (key == 224) {  // 方向键
            key = _getch();
            if (key == 72 && selected > 0) selected--;           // ↑
            else if (key == 80 && selected < 8) selected++;      // ↓
        } else if (key == 13) {  // 回车
            // ===== 选项 1: 输入路径 =====
            if (selected == 0) {
                system("cls");
                setColor(9);
                cout << "  请输入文件或文件夹路径: ";
                setColor(7);
                string path;
                getline(cin, path);
                rebuildIndex(path);
                if (totalDocNum > 0) {
                    setColor(10); cout << "\n  ✓ 加载成功! " << totalDocNum << " 个文档" << endl; setColor(7);
                } else {
                    setColor(12); cout << "\n  ✗ 未找到可读取的文档" << endl; setColor(7);
                }
                cout << "\n  按回车键返回菜单...";
                cin.get();
            }
            // ===== 选项 2: 拖入路径 =====
            else if (selected == 1) {
                system("cls");
                setColor(9);
                cout << "  请将文件或文件夹拖入此窗口, 然后按回车:" << endl;
                cout << "  > ";
                setColor(7);
                string path;
                getline(cin, path);
                if (path.front() == '"') path = path.substr(1);
                if (path.back() == '"') path.pop_back();
                while (!path.empty() && path.back() == ' ') path.pop_back();
                rebuildIndex(path);
                if (totalDocNum > 0) {
                    setColor(10); cout << "\n  ✓ 加载成功! " << totalDocNum << " 个文档" << endl; setColor(7);
                } else {
                    setColor(12); cout << "\n  ✗ 未找到可读取的文档" << endl; setColor(7);
                }
                cout << "\n  按回车键返回菜单...";
                cin.get();
            }
            // ===== 选项 3: 文件对话框 =====
            else if (selected == 2) {
                if (LoadDialog) {
                    allDocs = LoadDialog();
                    if (!allDocs.empty()) {
                        auto result = BuildIndex(allDocs);
                        invertIndex = std::get<0>(result);
                        docWordLenArr = std::get<1>(result);
                        totalDocNum = std::get<2>(result);
                        currentDataFolder = "文件对话框选择";
                        system("cls");
                        setColor(10); cout << "\n  ✓ 加载成功! " << totalDocNum << " 个文档" << endl; setColor(7);
                    } else {
                        system("cls");
                        setColor(12); cout << "\n  ✗ 未选择文件" << endl; setColor(7);
                    }
                }
                cout << "\n  按回车键返回菜单...";
                cin.get();
            }
            // ===== 选项 4: 直接输入文本 =====
            else if (selected == 3) {
                system("cls");
                string inputText;
                setColor(9);
                cout << "  请输入文本内容 (输入空行+回车结束):" << endl;
                cout << "  ─────────────────────────────" << endl;
                setColor(7);
                string line;
                while (getline(cin, line)) {
                    if (line.empty()) break;
                    inputText += line + " ";
                }
                
                if (!inputText.empty()) {
                    string cleaned = TextClean(inputText);
                    vector<string> words = SplitWord(cleaned);
                    
                    // 词频统计
                    printWordFreq(words);
                    
                    // 直接构建单文档索引供检索
                    DocInfo doc;
                    doc.docId = 0;
                    doc.fileName = "用户输入文本";
                    doc.words = words;
                    allDocs.clear();
                    allDocs.push_back(doc);
                    
                    auto result = BuildIndex(allDocs);
                    invertIndex = std::get<0>(result);
                    docWordLenArr = std::get<1>(result);
                    totalDocNum = 1;
                    currentDataFolder = "直接输入";
                    
                    setColor(14);
                    cout << "\n  ✓ 已构建索引, 可进行检索测试" << endl;
                    setColor(7);
                }
                cout << "\n  按回车键返回菜单...";
                cin.get();
            }
            // ===== 选项 5: 关键词检索 =====
            else if (selected == 4) {
                if (totalDocNum == 0) {
                    system("cls");
                    setColor(12); cout << "\n  ✗ 请先加载文档!" << endl; setColor(7);
                    cout << "\n  按回车键返回菜单..."; cin.get();
                    continue;
                }
                string query; int topK = 5;
                system("cls");
                setColor(9);
                cout << "  请输入检索关键词: "; setColor(7);
                cin >> query;
                cout << "  返回文档数量 [" << topK << "]: ";
                string input; getline(cin, input);
                if (!input.empty()) topK = stoi(input);
                
                vector<string> words = QuerySplit(query);
                set<int> matchDocIds = PhraseSearch(invertIndex, words);
                ScorePair res = GetTopK(matchDocIds, words, invertIndex, docWordLenArr, totalDocNum, topK);
                PrintResult(res, allDocs);
                cout << "\n  按回车键返回菜单..."; cin.get();
            }
            // ===== 选项 6: 短语检索 =====
            else if (selected == 5) {
                if (totalDocNum == 0) {
                    system("cls");
                    setColor(12); cout << "\n  ✗ 请先加载文档!" << endl; setColor(7);
                    cout << "\n  按回车键返回菜单..."; cin.get();
                    continue;
                }
                string phrase;
                system("cls");
                setColor(9);
                cout << "  请输入短语: "; setColor(7);
                getline(cin, phrase);
                
                vector<string> words = QuerySplit(phrase);
                set<int> matchDocIds = PhraseSearch(invertIndex, words);
                ScorePair res = GetTopK(matchDocIds, words, invertIndex, docWordLenArr, totalDocNum, 999);
                PrintResult(res, allDocs);
                cout << "\n  按回车键返回菜单..."; cin.get();
            }
            // ===== 选项 7: Benchmark =====
            else if (selected == 6) {
                if (totalDocNum == 0) {
                    system("cls");
                    setColor(12); cout << "\n  ✗ 请先加载文档!" << endl; setColor(7);
                    cout << "\n  按回车键返回菜单..."; cin.get();
                    continue;
                }
                string input;
                system("cls");
                setColor(9);
                cout << "  输入关键词(逗号分隔): "; setColor(7);
                getline(cin, input);
                
                auto start = chrono::high_resolution_clock::now();
                vector<string> words = QuerySplit(input);
                set<int> matchDocIds = PhraseSearch(invertIndex, words);
                ScorePair res = GetTopK(matchDocIds, words, invertIndex, docWordLenArr, totalDocNum, 5);
                auto end = chrono::high_resolution_clock::now();
                double ms = chrono::duration<double, milli>(end - start).count();
                
                setColor(14);
                cout << "\n  ⚡ 查询耗时: " << ms << " ms" << endl;
                setColor(7);
                PrintResult(res, allDocs);
                cout << "\n  按回车键返回菜单..."; cin.get();
            }
            // ===== 选项 8: 开发测试 =====
            else if (selected == 7) {
                system("cls");
                setColor(14);
                cout << "  🧪 [开发测试模式]" << endl;
                cout << "  ─────────────────────────────" << endl;
                setColor(7);
                rebuildIndex("text_data");
                if (totalDocNum > 0) {
                    setColor(10); cout << "  ✓ 测试文档加载完成! " << totalDocNum << " 个文档" << endl; setColor(7);
                } else {
                    setColor(12); cout << "  ✗ text_data 文件夹不存在或为空" << endl; setColor(7);
                }
                cout << "\n  按回车键返回菜单..."; cin.get();
            }
            // ===== 选项 9: 退出 =====
            else if (selected == 8) {
                system("cls");
                setColor(8);
                cout << "\n  感谢使用 Mini Text Search Engine!" << endl;
                cout << "  程序退出。" << endl;
                setColor(7);
                break;
            }
        }
    }
    
    FreeLibrary(hReadPre); FreeLibrary(hIndex); FreeLibrary(hQuery);
    FreeLibrary(hRank); FreeLibrary(hOutput);
    return 0;
}
