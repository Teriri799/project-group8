#include "../../common/common.h"
#include "QueryParse.h"

std::vector<std::string> QuerySplit(const std::string& query) {
    std::string text;

    for (char ch : query) {
        char lower = tolower(static_cast<unsigned char>(ch));
        if (!ispunct(static_cast<unsigned char>(lower))) {
            text += lower;
        }
    }
    std::vector<std::string> words;
    std::string tmp;
    for (char ch : text) {
        if (ch == ' ' || ch == '\t' || ch == '\n') {
            if (tmp.size() > 1 && !isStopWord(tmp)) {
                words.push_back(tmp);
            }
            tmp.clear();
        } else {
            tmp += ch;
        }
    }
    
    if (tmp.size() > 1 && !isStopWord(tmp)) {
        words.push_back(tmp);
    }

    return words;
}

std::set<int> PhraseSearch(
    const InvertIndex& index,
    const std::vector<std::string>& queryWords
) {
    std::set<int> resultSet;

    if (queryWords.empty() || index.empty()) {
        return resultSet;
    }

    auto it = index.find(queryWords[0]);
    if (it == index.end()) {
        return resultSet;
    }

    for (const auto& pair : it->second) {
        resultSet.insert(pair.first);
    }

    for (size_t i = 1; i < queryWords.size(); i++) {
        auto itNext = index.find(queryWords[i]);
        if (itNext == index.end()) {
            resultSet.clear();
            break;
        }

        std::set<int> nextSet;
        for (const auto& pair : itNext->second) {
            nextSet.insert(pair.first);
        }

        std::set<int> intersection;
        std::set_intersection(
            resultSet.begin(), resultSet.end(),
            nextSet.begin(), nextSet.end(),
            std::inserter(intersection, intersection.begin())
        );

        resultSet = intersection;

        if (resultSet.empty()) {
            break;
        }
    }

    return resultSet;
}
