#ifndef MODULE1_PREPROCESS_H
#define MODULE1_PREPROCESS_H

#ifdef MODULE1_EXPORTS
    #define MODULE1_API __declspec(dllexport)
#else
    #define MODULE1_API __declspec(dllimport)
#endif

// 预处理结果：清洗后的词条列表
typedef struct {
    char** tokens;          // 清洗后的词条数组
    int    token_count;     // 词条数量
    int    total_chars;     // 原文总字符数
    char   source_name[256]; // 来源文件名或标识
} PreprocessResult;

// ========== 4种输入方式 ==========

// 方式1：通过文件路径读取并清洗
MODULE1_API PreprocessResult* load_and_preprocess_file(const char* filepath);

// 方式2：处理整个文件夹
MODULE1_API PreprocessResult** load_and_preprocess_folder(const char* folderpath, int* doc_count);

// 方式3：弹出文件选择对话框
MODULE1_API PreprocessResult* open_file_dialog_and_preprocess(void);

// 方式4：直接清洗一段文本（注意：有两个参数！）
MODULE1_API PreprocessResult* preprocess_text(const char* raw_text, const char* source_name);

// ========== 配置项 ==========

// 开启/关闭停用词过滤（默认开启）
MODULE1_API void enable_stopword_filter(int enable);

// 设置最小词长（英文按字母数，中文按字数，默认2）
MODULE1_API void set_min_word_length(int length);

// ========== 报告与释放 ==========

// 打印清洗报告
MODULE1_API void print_cleaning_report(const PreprocessResult* result);

// 释放单个结果
MODULE1_API void free_result(PreprocessResult* result);

// 释放文件夹批处理结果
MODULE1_API void free_folder_result(PreprocessResult** results, int doc_count);

#endif
