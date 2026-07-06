#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

typedef struct {
    char** tokens;
    int    token_count;
    int    total_chars;
    char   source_name[256];
} PreprocessResult;

typedef PreprocessResult* (*LoadFileFunc)(const char*);
typedef PreprocessResult** (*LoadFolderFunc)(const char*, int*);
typedef PreprocessResult* (*OpenFileDialogFunc)(void);
typedef PreprocessResult* (*PreprocessTextFunc)(const char*, const char*);
typedef void (*FreeResultFunc)(PreprocessResult*);
typedef void (*FreeFolderFunc)(PreprocessResult**, int);
typedef void (*PrintReportFunc)(const PreprocessResult*);
typedef void (*EnableStopwordFunc)(int);
typedef void (*SetMinWordLenFunc)(int);

void show_menu(void) {
    printf("\n========================================\n");
    printf("  轻量文本检索 - 模块1：文本清洗\n");
    printf("  (中文分词引擎已启用)\n");
    printf("========================================\n");
    printf("  1. 从文件路径读取并清洗\n");
    printf("  2. 清洗整个文件夹\n");
    printf("  3. 弹出文件选择对话框\n");
    printf("  4. 直接输入文本并清洗\n");
    printf("  -----------------------------\n");
    printf("  5. 切换停用词过滤 (默认: 开启)\n");
    printf("  6. 设置最小词长 (默认: 2)\n");
    printf("  -----------------------------\n");
    printf("  7. 运行演示 (中英文混合测试)\n");
    printf("  -----------------------------\n");
    printf("  0. 退出\n");
    printf("========================================\n");
    printf("请选择: ");
}

void run_demo(PreprocessTextFunc preprocess_text, PrintReportFunc print_report, 
              FreeResultFunc free_result) {
    printf("\n========== 中英文混合分词演示 ==========\n\n");
    
    const char* test_texts[] = {
        "今天天气真好，我们去公园散步吧！",
        "人工智能正在改变世界，深度学习是重要技术。",
        "Hello world! 你好世界！Mini Text Search Engine 项目。",
        "数据结构与算法是计算机科学的核心课程。",
        "我们正在学习C语言编程和软件开发。",
        "I love programming. 编程让生活更美好。"
    };
    
    int num_tests = sizeof(test_texts) / sizeof(test_texts[0]);
    
    for (int i = 0; i < num_tests; i++) {
        printf("----------------------------------------\n");
        printf("原始文本: %s\n", test_texts[i]);
        PreprocessResult* result = preprocess_text(test_texts[i], "演示文本");
        if (result) {
            print_report(result);
            free_result(result);
        }
        printf("\n");
    }
}

int main(int argc, char* argv[]) {
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);

    HMODULE dll = LoadLibraryA("bin/module1.dll");
    if (!dll) {
        printf("错误: 无法加载 bin/module1.dll\n");
        return 1;
    }

    LoadFileFunc load_file = (LoadFileFunc)GetProcAddress(dll, "load_and_preprocess_file");
    LoadFolderFunc load_folder = (LoadFolderFunc)GetProcAddress(dll, "load_and_preprocess_folder");
    OpenFileDialogFunc open_dialog = (OpenFileDialogFunc)GetProcAddress(dll, "open_file_dialog_and_preprocess");
    PreprocessTextFunc preprocess_text = (PreprocessTextFunc)GetProcAddress(dll, "preprocess_text");
    FreeResultFunc free_result = (FreeResultFunc)GetProcAddress(dll, "free_result");
    FreeFolderFunc free_folder = (FreeFolderFunc)GetProcAddress(dll, "free_folder_result");
    PrintReportFunc print_report = (PrintReportFunc)GetProcAddress(dll, "print_cleaning_report");
    EnableStopwordFunc enable_stopword = (EnableStopwordFunc)GetProcAddress(dll, "enable_stopword_filter");
    SetMinWordLenFunc set_min_len = (SetMinWordLenFunc)GetProcAddress(dll, "set_min_word_length");

    if (!load_file || !preprocess_text || !free_result || !print_report) {
        printf("错误: 无法获取 DLL 中的函数\n");
        FreeLibrary(dll);
        return 1;
    }

    // 命令行参数模式
    if (argc >= 2) {
        printf("\n使用方式: 命令行参数传入文件路径\n");
        PreprocessResult* result = load_file(argv[1]);
        if (result) {
            print_report(result);
            free_result(result);
        }
        FreeLibrary(dll);
        return 0;
    }

    // 交互模式
    int choice;
    int stopword_enabled = 1;

    while (1) {
        show_menu();
        if (scanf("%d", &choice) != 1) break;
        getchar();

        PreprocessResult* result = NULL;

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
                PreprocessResult** results = load_folder(path, &doc_count);
                if (results) {
                    for (int i = 0; i < doc_count; i++) {
                        if (results[i]) print_report(results[i]);
                    }
                    free_folder(results, doc_count);
                }
                continue;
            }
            case 3: {
                result = open_dialog();
                break;
            }
            case 4: {
                printf("请输入文本（输入 END 结束）:\n");
                char buffer[4096] = {0};
                char line[256];
                while (fgets(line, sizeof(line), stdin)) {
                    if (strcmp(line, "END\n") == 0) break;
                    strcat(buffer, line);
                }
                result = preprocess_text(buffer, "用户输入");
                break;
            }
            case 5: {
                stopword_enabled = !stopword_enabled;
                if (enable_stopword) enable_stopword(stopword_enabled);
                continue;
            }
            case 6: {
                printf("请输入最小词长 (1~10): ");
                int len;
                scanf("%d", &len);
                getchar();
                if (set_min_len) set_min_len(len);
                continue;
            }
            case 7: {
                run_demo(preprocess_text, print_report, free_result);
                continue;
            }
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
            free_result(result);
        }
    }

    FreeLibrary(dll);
    return 0;
}
