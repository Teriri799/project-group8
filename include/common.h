#ifndef COMMON_H
#define COMMON_H

// ====== DLL 导出宏 ======
#ifdef __cplusplus
#define DLL_API extern "C" __declspec(dllexport)
#else
#define DLL_API __declspec(dllexport)
#endif

// ====== 模块1输出：清洗后的文档 ======
typedef struct {
    char**  tokens;        // 词条数组
    int     token_count;   // 词条数量
    int     total_chars;   // 原文长度
    char    source_name[256]; // 文件名
} CleanedDocument;

// ====== 模块2输出：倒排索引条目 ======
typedef struct {
    char    word[64];      // 词条
    int*    doc_ids;       // 包含该词的文档ID数组
    int*    frequencies;   // 对应频率数组
    int     doc_count;     // 包含该词的文档数
} InvertedIndexEntry;

// ====== 模块3输出：查询结果 ======
typedef struct {
    int    doc_id;         // 文档ID
    char   doc_name[256]; // 文档名
    double score;          // 相关度评分
} SearchResult;

// ====== 模块4输出：排序结果 ======
typedef SearchResult RankedResult;

// ====== 模块5输出：最终结果 ======
typedef struct {
    RankedResult* results;
    int           result_count;
    double        query_time_ms;  // 查询耗时
} FinalOutput;

#endif
