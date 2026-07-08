#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable : 4251)
#ifndef QUERY_PARSE_H
#define QUERY_PARSE_H

#include "../../common/common.h"

#ifdef _WIN32
    #ifdef QUERY_PARSE_EXPORTS
        #define DLL_EXPORT extern "C" __declspec(dllexport)
    #else
        #define DLL_EXPORT extern "C" __declspec(dllimport)
    #endif
#endif

// 分词函数
DLL_EXPORT std::vector<std::string> QuerySplit(const std::string& query);
// 短语交集检索
DLL_EXPORT std::set<int> PhraseSearch(const InvertIndex& index, const std::vector<std::string>& words);

#endif
