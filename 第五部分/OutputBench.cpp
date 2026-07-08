#include "OutputBench.h"
#include <iostream>

DLL_EXPORT void PrintSearchResult(const ScorePair& topkResult, const std::vector<DocInfo>& docList)
{
    std::cout << "\n======= 检索结果TOP-K =======\n";
    if (topkResult.empty())
    {
        std::cout << "未匹配到相关文档！" << std::endl;
        return;
    }

    for (int i = 0; i < topkResult.size(); i++)
    {
        int docId = topkResult[i].first;
        double score = topkResult[i].second;
        std::string filename = docList[docId].fileName;
        std::cout << "第" << i + 1 << "名 | 文档名称：" << filename << "  匹配分数：" << score << std::endl;
    }
}
