#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "module1.h"

// 定义DLL函数指针类型
typedef CleanedDocument* (*LoadFileFunc)(const char*);
typedef CleanedDocument** (*LoadFolderFunc)(const char*, int*);
typedef CleanedDocument* (*OpenDialogFunc)(void);
typedef CleanedDocument* (*CleanTextFunc)(const char*, const char*);
typedef void (*FreeDocFunc)(CleanedDocument*);
typedef void (*FreeDocsFunc)(CleanedDocument**, int);
typedef void (*PrintReportFunc)(const CleanedDocument*);
typedef void (*SetStopwordFunc)(int);
typedef void (*SetMinLenFunc)(int);
typedef const char* (*GetVersionFunc)(void);

void show_menu(void) {
    printf("\n========================================\n");
    printf("  轻量文本检索 - 模块1：文本清洗\n");
    printf("========================================\n");
    printf("  1. 从文件路径读取并清洗\n");
    printf("  2. 清洗整个文件夹\n");
    printf("  3. 弹出文件选择对话框\n");
    printf("  4. 直接输入文本并清洗\n");
    printf("  -----------------------------\n");
    printf("  5. 切换停用词过滤\n");
    printf("  6. 设置最小词长\n");
    printf("  7. 版本信息\n");
    printf("  8. 运行演示\n");
    printf("  -----------------------------\n");
    printf("  0. 退出\n");
    printf("========================================\n");
    printf("请选择: ");
}

void run_demo(CleanTextFunc clean_text, PrintReportFunc print_report,
              FreeDocFunc free_doc) {
    printf("\n========== 中英文混合分词演示 ==========\n\n");
    const char* test_texts[] = {
        "今天天气真好，我们去公园散步吧！",
        "人工智能正在改变世界，深度学习是重要技术。",
        "Hello world! 你好世界！Mini Text Search Engine 项目。",
        "数据结构与算法是计算机科学的核心课程。",
        "I love programming. 编程让生活更美好。"
    };
    int num_tests = sizeof(test_texts) / sizeof(test_texts[0]);
    for (int i = 0; i < num_tests; i++) {
        printf("----------------------------------------\n");
        printf("原始文本: %s\n", test_texts[i]);
        CleanedDocument* result = clean_text(test_texts[i], "演示文本");
        if (result) {
            print_report(result);
            free_doc(result);
        }
        printf("\n");
    }
}

int main(int argc, char* argv[]) {
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);

    // 加载DLL
    HMODULE dll = LoadLibraryA("bin/module1.dll");
    if (!dll) {
        printf("错误: 无法加载 bin/module1.dll\n");
        printf("请先运行 build.bat 编译\n");
        return 1;
    }

    // 获取所有函数指针
    LoadFileFunc load_file = (LoadFileFunc)GetProcAddress(dll, "module1_load_file");
    LoadFolderFunc load_folder = (LoadFolderFunc)GetProcAddress(dll, "module1_load_folder");
    OpenDialogFunc open_dialog = (OpenDialogFunc)GetProcAddress(dll, "module1_open_file_dialog");
    CleanTextFunc clean_text = (CleanTextFunc)GetProcAddress(dll, "module1_clean_text");
    FreeDocFunc free_doc = (FreeDocFunc)GetProcAddress(dll, "module1_free_document");
    FreeDocsFunc free_docs = (FreeDocsFunc)GetProcAddress(dll, "module1_free_documents");
    PrintReportFunc print_report = (PrintReportFunc)GetProcAddress(dll, "module1_print_report");
    SetStopwordFunc set_stopword = (SetStopwordFunc)GetProcAddress(dll, "module1_set_stopword_filter");
    SetMinLenFunc set_min_len = (SetMinLenFunc)GetProcAddress(dll, "module1_set_min_word_length");
    GetVersionFunc get_version = (GetVersionFunc)GetProcAddress(dll, "module1_get_version");

    if (!load_file || !clean_text || !free_doc || !print_report) {
        printf("错误: 无法获取 DLL 中的函数\n");
        FreeLibrary(dll);
        return 1;
    }

    // 命令行参数模式
    if (argc >= 2) {
        printf("\n使用方式: 命令行参数传入文件路径\n");
        CleanedDocument* result = load_file(argv[1]);
        if (result) {
            print_report(result);
            free_doc(result);
        }
        FreeLibrary(dll);
        return 0;
    }

    // 交互模式
    int choice;
    while (1) {
        show_menu();
        if (scanf("%d", &choice) != 1) break;
        getchar();

        CleanedDocument* result = NULL;

        switch (choice) {
            case 1: {
                char path[512];
                printf("请输入文件路径: ");
                fgets(path, sizeof(path), stdin);
                path[strcspn(path, "\n")] = 0;
                result = load_file(path);
                break;
            }
            case 2: {
                char path[512];
                printf("请输入文件夹路径: ");
                fgets(path, sizeof(path), stdin);
                path[strcspn(path, "\n")] = 0;
                int doc_count = 0;
                CleanedDocument** results = load_folder(path, &doc_count);
                if (results) {
                    for (int i = 0; i < doc_count; i++)
                        if (results[i]) print_report(results[i]);
                    free_docs(results, doc_count);
                }
                continue;
            }
            case 3:
                result = open_dialog();
                break;
            case 4: {
                printf("请输入文本（输入 END 结束）:\n");
                char buffer[4096] = {0};
                char line[256];
                while (fgets(line, sizeof(line), stdin)) {
                    if (strcmp(line, "END\n") == 0) break;
                    strcat(buffer, line);
                }
                result = clean_text(buffer, "用户输入");
                break;
            }
            case 5: {
                static int enabled = 1;
                enabled = !enabled;
                if (set_stopword) set_stopword(enabled);
                continue;
            }
            case 6: {
                int len;
                printf("请输入最小词长 (1~10): ");
                scanf("%d", &len);
                getchar();
                if (set_min_len) set_min_len(len);
                continue;
            }
            case 7:
                if (get_version) printf("\n%s\n\n", get_version());
                continue;
            case 8:
                run_demo(clean_text, print_report, free_doc);
                continue;
            case 0:
                printf("程序结束。\n");
                FreeLibrary(dll);
                return 0;
            default:
                printf("无效选择，请重试。\n");
                continue;
        }

        if (result) {
            print_report(result);
            free_doc(result);
        }
    }

    FreeLibrary(dll);
    return 0;
}
