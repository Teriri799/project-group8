#define MODULE1_EXPORTS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../../include/module1_preprocess.h"

// 判断字符是否是分隔符
static int is_delimiter(char c) {
    return isspace(c) || ispunct(c);
}

// 字符串转小写
static void str_to_lower(char* str) {
    for (int i = 0; str[i]; i++) {
        str[i] = tolower((unsigned char)str[i]);
    }
}

// 读取整个文件内容到字符串
static char* read_file_content(const char* filepath) {
    FILE* f = fopen(filepath, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* content = (char*)malloc(size + 1);
    if (!content) {
        fclose(f);
        return NULL;
    }

    fread(content, 1, size, f);
    content[size] = '\0';
    fclose(f);
    return content;
}

// 将文本拆分为词条
static PreprocessResult* tokenize(const char* text) {
    PreprocessResult* result = (PreprocessResult*)malloc(sizeof(PreprocessResult));
    result->words = NULL;
    result->counts = NULL;
    result->word_count = 0;

    if (!text) return result;

    char* text_copy = strdup(text);
    if (!text_copy) return result;

    int capacity = 64;
    int count = 0;
    char** words = (char**)malloc(capacity * sizeof(char*));

    char* token = strtok(text_copy, " \t\n\r\f\v.,;:!?\"'()[]{}-–—/\\|@#$%^&*+=<>~`\r\n");
    while (token) {
        str_to_lower(token);
        if (strlen(token) > 0) {
            if (count >= capacity) {
                capacity *= 2;
                words = (char**)realloc(words, capacity * sizeof(char*));
            }
            words[count++] = strdup(token);
        }
        token = strtok(NULL, " \t\n\r\f\v.,;:!?\"'()[]{}-–—/\\|@#$%^&*+=<>~`\r\n");
    }
    free(text_copy);

    result->words = words;
    result->word_count = count;
    return result;
}

MODULE1_API PreprocessResult* load_and_preprocess_file(const char* filepath) {
    printf("[模块1] 正在读取文件: %s\n", filepath);

    char* content = read_file_content(filepath);
    if (!content) {
        printf("[模块1] 错误: 无法打开文件 %s\n", filepath);
        return NULL;
    }

    printf("[模块1] 文件读取成功，大小: %lld 字节\n", 
           (long long)strlen(content));

    PreprocessResult* result = tokenize(content);
    free(content);

    printf("[模块1] 预处理完成，共提取 %d 个词条\n", result->word_count);
    return result;
}

MODULE1_API PreprocessResult* preprocess_text(const char* raw_text) {
    printf("[模块1] 直接处理输入文本...\n");
    PreprocessResult* result = tokenize(raw_text);
    printf("[模块1] 预处理完成，共提取 %d 个词条\n", result->word_count);
    return result;
}

MODULE1_API void free_result(PreprocessResult* result) {
    if (!result) return;
    if (result->words) {
        for (int i = 0; i < result->word_count; i++) {
            free(result->words[i]);
        }
        free(result->words);
    }
    if (result->counts) {
        free(result->counts);
    }
    free(result);
}
