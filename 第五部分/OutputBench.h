#ifndef OUTPUT_BENCH_H
#define OUTPUT_BENCH_H

#include "../common/common.h"
#include <vector>
#include <string>

#ifdef _WIN32
#define DLL_EXPORT extern "C" __declspec(dllexport)
#else
#define DLL_EXPORT extern "C"
#endif

// 打印TopK检索结果
DLL_EXPORT void PrintSearchResult(const ScorePair& topkResult, const std::vector<DocInfo>& docList);

#endif
