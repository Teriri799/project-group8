#ifndef MODULE1_H
#define MODULE1_H

#include "common.h"

// ====== DLL导出函数 ======

// 方式1：从文件路径读取并清洗
DLL_API CleanedDocument* module1_load_file(const char* filepath);

// 方式2：从文件夹读取所有文件（返回文档数组）
DLL_API CleanedDocument** module1_load_folder(const char* folderpath, int* doc_count);

// 方式3：弹出文件选择对话框并清洗
DLL_API CleanedDocument* module1_open_file_dialog(void);

// 方式4：直接清洗文本
DLL_API CleanedDocument* module1_clean_text(const char* text, const char* source_name);

// 设置停用词过滤（1开启，0关闭）
DLL_API void module1_set_stopword_filter(int enable);

// 设置最小词长
DLL_API void module1_set_min_word_length(int length);

// 获取版本信息
DLL_API const char* module1_get_version(void);

// 打印报告（用于调试）
DLL_API void module1_print_report(const CleanedDocument* doc);

// 释放单个文档
DLL_API void module1_free_document(CleanedDocument* doc);

// 释放文档数组
DLL_API void module1_free_documents(CleanedDocument** docs, int count);

#endif
