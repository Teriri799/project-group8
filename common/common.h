#ifndef COMMON_H
#define COMMON_H
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <set>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <tuple>

struct DocInfo {
    int docId;                
    std::string fileName;     
    std::vector<std::string> words; 
};

using InvertIndex = std::unordered_map<std::string, std::vector<std::pair<int, int>>>;
using ScorePair = std::vector<std::pair<int, double>>;

const std::vector<std::string> stopWords = {
    "的","了","是","在","和","也","就","都",
    "我","你","他","她","它","们","这","那","有","不","人",
    "个","上","下","中","大","小","多","少","去","来",
    "a","an","the","or","and","on","in","at","is","are"
};


inline bool isStopWord(const std::string& word)
{
    for(const std::string &s : stopWords)
    {
        if(s == word)
            return true;
    }
    return false;
}

#endif
