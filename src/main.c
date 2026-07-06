#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

typedef struct {
    char** words;
    int*   counts;
    int    word_count;
} PreprocessResult;

typedef PreprocessResult* (*LoadFileFunc)(const char*);
typedef PreprocessResult* (*PreprocessTextFunc)(const char*);
typedef void (*FreeResultFunc)(PreprocessResult*);

int main(int argc, char* argv[]) {
    // ====== 解决中文乱码：设置控制台编码为 UTF-8 ======
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);

    printf("========================================\n");
    printf("  轻量文本检索工具 (Mini Text Search)\n");
    printf("========================================\n\n");

    HMODULE dll = LoadLibraryA("bin/module1.dll");
    if (!dll) {
        printf("错误: 无法加载 bin/module1.dll\n");
        return 1;
    }

    LoadFileFunc load_file = (LoadFileFunc)GetProcAddress(dll, "load_and_preprocess_file");
    PreprocessTextFunc preprocess_text = (PreprocessTextFunc)GetProcAddress(dll, "preprocess_text");
    FreeResultFunc free_result = (FreeResultFunc)GetProcAddress(dll, "free_result");

    if (!load_file || !preprocess_text || !free_result) {
        printf("错误: 无法获取 DLL 中的函数\n");
        FreeLibrary(dll);
        return 1;
    }

    PreprocessResult* result = NULL;

    if (argc >= 2) {
        printf("使用方式: 命令行参数传入文件路径\n");
        result = load_file(argv[1]);
    }
    else {
        printf("使用方式: 直接输入文本\n");
        printf("请输入需要检索的文本（输入 END 结束）:\n");

        char buffer[4096] = {0};
        char line[256];
        while (fgets(line, sizeof(line), stdin)) {
            if (strcmp(line, "END\n") == 0) break;
            strcat(buffer, line);
        }

        result = preprocess_text(buffer);
    }

    if (result) {
        printf("\n========== 预处理结果 ==========\n");
        printf("词条总数: %d\n", result->word_count);
        printf("词条列表（前20个）:\n");
        for (int i = 0; i < result->word_count && i < 20; i++) {
            printf("  [%d] %s\n", i, result->words[i]);
        }
        free_result(result);
    }

    FreeLibrary(dll);
    printf("\n程序结束。\n");
    return 0;
}
