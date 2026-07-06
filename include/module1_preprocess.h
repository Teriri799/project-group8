#ifndef MODULE1_PREPROCESS_H
#define MODULE1_PREPROCESS_H

#ifdef MODULE1_EXPORTS
    #define MODULE1_API __declspec(dllexport)
#else
    #define MODULE1_API __declspec(dllimport)
#endif

typedef struct {
    char** words;       
    int*  counts;       
    int   word_count;   
} PreprocessResult;

// 方式1：通过路径加载单个文件
MODULE1_API PreprocessResult* load_and_preprocess_file(const char* filepath);

// 方式2：直接处理一段文本
MODULE1_API PreprocessResult* preprocess_text(const char* raw_text);

// 释放内存
MODULE1_API void free_result(PreprocessResult* result);

#endif
