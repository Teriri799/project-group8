#include "Rank.h"
#include <algorithm>
#include <cmath>

double CalcTFIDF(int docId, const std::vector<std::string>& qWords, const InvertIndex& idx, const std::vector<int>& docLen, int totalN)
{
    double score = 0.0;
    int wordsTotal = docLen[docId];
    for (auto &word : qWords)
    {
        if (idx.find(word) == idx.end()) continue;
        int tf = 0;
        for (auto &item : idx.at(word))
        {
            if (item.first == docId)
            {
                tf = item.second;
                break;
            }
        }
        double TF = 1.0 * tf / wordsTotal;
        double IDF = log(1.0 * totalN / idx.at(word).size());
        score += TF * IDF;
    }
    return score;
}

extern "C" DLL_EXPORT void AddPhraseBoostScore(ScorePair& scoreList, const std::vector<std::vector<std::string>>& phraseList)
{
    if (!phraseList.empty())
    {
        for (auto& doc : scoreList)
        {
            doc.second += 0.3;
        }
    }
}

extern "C" DLL_EXPORT ScorePair GetTopKResult(std::set<int> candidateDocs,
    const std::vector<std::string>& qWords,
    const InvertIndex& idx,
    const std::vector<int>& docLen,
    int totalN,
    int K)
{
    ScorePair scoreList;
    for (int did : candidateDocs)
    {
        double s = CalcTFIDF(did, qWords, idx, docLen, totalN);
        scoreList.emplace_back(did, s);
    }
    std::sort(scoreList.begin(), scoreList.end(), [](std::pair<int, double> a, std::pair<int, double> b) {
        return a.second > b.second;
    });
    if (scoreList.size() > K)
        scoreList.resize(K);
    return scoreList;
}