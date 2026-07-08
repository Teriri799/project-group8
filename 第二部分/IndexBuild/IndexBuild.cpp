#include "IndexBuild.h"
#include <unordered_map>

extern "C" DLL_EXPORT std::tuple<InvertIndex, std::vector<int>, int> BuildInvertIndex(const std::vector<DocInfo>& docData)
{
    InvertIndex index;
    std::vector<int> docWordLen;
    int totalDocNum = docData.size();

    for(auto& doc : docData)
    {
        docWordLen.push_back(doc.words.size());
        std::unordered_map<std::string, int> tfCount;
        for(auto& word : doc.words)
            tfCount[word]++;
        for(auto& item : tfCount)
            index[item.first].emplace_back(doc.docId, item.second);
    }
    return {index, docWordLen, totalDocNum};
}
