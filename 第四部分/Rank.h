#ifndef RANK_H
#define RANK_H
#include "../common/common.h"

#ifdef _WIN32
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif

double CalcTFIDF(int docId, const std::vector<std::string>& qWords, const InvertIndex& idx, const std::vector<int>& docLen, int totalN);

extern "C" DLL_EXPORT void AddPhraseBoostScore(ScorePair& scoreList, const std::vector<std::vector<std::string>>& phraseList);

extern "C" DLL_EXPORT ScorePair GetTopKResult(std::set<int> candidateDocs,
    const std::vector<std::string>& qWords,
    const InvertIndex& idx,
    const std::vector<int>& docLen,
    int totalN,
    int K);
#endif
