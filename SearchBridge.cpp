#include <windows.h>
#include <string>
#include <vector>
#include <set>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <fstream>
#include <iostream>
#include <chrono>
#include <tuple>
#include <iomanip>

using namespace std;

struct DocInfo {
    int docId;
    int originalId;
    string fileName;
    vector<string> words;
    string rawContent;
};

using InvertIndex = unordered_map<string, vector<pair<int, int>>>;
using ScorePair = vector<pair<int, double>>;

const vector<string> stopWords = {
    "a","an","the","or","and","on","in","at","is","are","was","were","be",
    "been","being","have","has","had","do","does","did","but","if","so",
    "no","not","of","for","with","as","by","to","it","its","this","that"
};

bool isStopWord(const string& word) {
    for (const string& s : stopWords) if (s == word) return true;
    return false;
}

string TextClean(string text) {
    string result;
    for (char ch : text) {
        char lower = tolower((unsigned char)ch);
        if (!ispunct((unsigned char)lower)) result += lower;
    }
    return result;
}

vector<string> SplitWord(string text) {
    vector<string> words;
    string tmp;
    for (char ch : text) {
        if (ch == ' ' || ch == '\t' || ch == '\n') {
            if (tmp.size() > 1 && !isStopWord(tmp)) words.push_back(tmp);
            tmp.clear();
        } else { tmp += ch; }
    }
    if (tmp.size() > 1 && !isStopWord(tmp)) words.push_back(tmp);
    return words;
}

bool hasExtension(const string& ext) {
    vector<string> exts = {".txt",".md",".csv",".py",".cpp",".h",".c",
                           ".java",".js",".ts",".css",".html",".htm",
                           ".xml",".json",".log",".ini",".cfg",".conf"};
    string lower = ext;
    transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    for (const string& e : exts) { if (lower == e) return true; }
    return false;
}

string readFileContent(const string& filepath) {
    ifstream file(filepath.c_str());
    if (!file.is_open()) return "";
    string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();
    return content;
}

vector<DocInfo> LoadAndPreprocessFile(const string& dirPath) {
    vector<DocInfo> docs;
    int docId = 0;
    string searchPath = dirPath + "\\*";

    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) return docs;

    do {
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        string name(findData.cFileName);
        size_t dot = name.find_last_of('.');
        if (dot == string::npos) continue;
        string ext = name.substr(dot);
        if (!hasExtension(ext)) continue;

        string fp = dirPath + "\\" + name;
        string content = readFileContent(fp);
        if (content.empty()) continue;

        string cleaned = TextClean(content);
        vector<string> words = SplitWord(cleaned);
        if (words.empty()) continue;

        DocInfo doc;
        doc.docId = docId;
        doc.originalId = docId;
        doc.fileName = fp;
        doc.words = words;
        doc.rawContent = content.substr(0, 500);
        docs.push_back(doc);
        docId++;
    } while (FindNextFileA(hFind, &findData));
    FindClose(hFind);

    return docs;
}

tuple<InvertIndex, vector<int>, int> BuildInvertIndex(const vector<DocInfo>& docData) {
    InvertIndex index;
    vector<int> docWordLen;
    int totalDocNum = docData.size();
    for (auto& doc : docData) {
        docWordLen.push_back(doc.words.size());
        unordered_map<string, int> tfCount;
        for (auto& word : doc.words) tfCount[word]++;
        for (auto& item : tfCount)
            index[item.first].emplace_back(doc.docId, item.second);
    }
    return {index, docWordLen, totalDocNum};
}

vector<string> QuerySplit(const string& query) {
    string text;
    for (char ch : query) {
        char lower = tolower((unsigned char)ch);
        if (!ispunct((unsigned char)lower)) text += lower;
    }
    vector<string> words;
    string tmp;
    for (char ch : text) {
        if (ch == ' ' || ch == '\t' || ch == '\n') {
            if (tmp.size() > 1 && !isStopWord(tmp)) words.push_back(tmp);
            tmp.clear();
        } else { tmp += ch; }
    }
    if (tmp.size() > 1 && !isStopWord(tmp)) words.push_back(tmp);
    return words;
}

set<int> PhraseSearch(const InvertIndex& index, const vector<string>& queryWords) {
    set<int> resultSet;
    if (queryWords.empty() || index.empty()) return resultSet;
    auto it = index.find(queryWords[0]);
    if (it == index.end()) return resultSet;
    for (const auto& pair : it->second) resultSet.insert(pair.first);
    for (size_t i = 1; i < queryWords.size(); i++) {
        auto itNext = index.find(queryWords[i]);
        if (itNext == index.end()) { resultSet.clear(); break; }
        set<int> nextSet;
        for (const auto& pair : itNext->second) nextSet.insert(pair.first);
        set<int> intersection;
        set_intersection(resultSet.begin(), resultSet.end(), nextSet.begin(), nextSet.end(), inserter(intersection, intersection.begin()));
        resultSet = intersection;
        if (resultSet.empty()) break;
    }
    return resultSet;
}

double CalcTFIDF(int docId, const vector<string>& qWords, const InvertIndex& idx, const vector<int>& docLen, int totalN) {
    double score = 0;
    for (const string& word : qWords) {
        auto it = idx.find(word);
        if (it == idx.end()) continue;
        int df = it->second.size();
        double idf = log(1 + (double)totalN / (1 + df));
        double tf = 0;
        for (const auto& p : it->second) {
            if (p.first == docId) { tf = (double)p.second / docLen[docId]; break; }
        }
        score += tf * idf;
    }
    return score;
}

ScorePair GetTopKResult(set<int> candidateDocs, const vector<string>& qWords,
    const InvertIndex& idx, const vector<int>& docLen, int totalN, int K) {
    ScorePair scoreList;
    for (int docId : candidateDocs) {
        double s = CalcTFIDF(docId, qWords, idx, docLen, totalN);
        scoreList.push_back({docId, s});
    }
    sort(scoreList.begin(), scoreList.end(), [](const pair<int,double>& a, const pair<int,double>& b) { return a.second > b.second; });
    if ((int)scoreList.size() > K) scoreList.resize(K);
    return scoreList;
}

string escapeJson(const string& s) {
    string out;
    for (char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c;
        }
    }
    return out;
}

static void safeCpy(char* dst, const string& src, int sz) {
    if (sz <= 0) return;
    strncpy(dst, src.c_str(), sz - 1);
    dst[sz - 1] = '\0';
}

static vector<DocInfo> g_allDocs;
static InvertIndex g_invertIndex;
static vector<int> g_docWordLenArr;
static int g_totalDocNum = 0;
static string g_currentFolder = "";

void rebuildAll() {
    if (g_currentFolder.empty()) return;
    g_allDocs = LoadAndPreprocessFile(g_currentFolder);
    if (!g_allDocs.empty()) {
        auto result = BuildInvertIndex(g_allDocs);
        g_invertIndex = get<0>(result);
        g_docWordLenArr = get<1>(result);
        g_totalDocNum = get<2>(result);
    } else {
        g_invertIndex.clear();
        g_docWordLenArr.clear();
        g_totalDocNum = 0;
    }
}

extern "C" {

__declspec(dllexport) int InitEngine() {
    g_allDocs.clear(); g_invertIndex.clear(); g_docWordLenArr.clear();
    g_totalDocNum = 0; g_currentFolder = "";
    return 0;
}

__declspec(dllexport) int LoadDocuments(const char* path) {
    try {
        g_currentFolder = string(path);
        rebuildAll();
        return g_totalDocNum;
    } catch (const exception& e) {
        return -1;
    } catch (...) { return -1; }
}

__declspec(dllexport) int GetDocCount() { return g_totalDocNum; }

__declspec(dllexport) int GetFileList(char* outBuf, int bufSize) {
    try {
        ostringstream json;
        json << "[";
        for (size_t i = 0; i < g_allDocs.size(); i++) {
            if (i > 0) json << ",";
            json << "{\"docId\":" << g_allDocs[i].docId
                 << ",\"file\":\"" << escapeJson(g_allDocs[i].fileName)
                 << "\",\"wordCount\":" << g_allDocs[i].words.size() << "}";
        }
        json << "]";
        safeCpy(outBuf, json.str(), bufSize);
        return (int)g_allDocs.size();
    } catch (...) { safeCpy(outBuf, "[]", bufSize); return -1; }
}

__declspec(dllexport) int GetFileWordFreq(int docId, char* outBuf, int bufSize) {
    try {
        if (docId < 0 || docId >= (int)g_allDocs.size()) { safeCpy(outBuf, "[]", bufSize); return 0; }
        unordered_map<string, int> freq;
        for (auto& w : g_allDocs[docId].words) freq[w]++;
        vector<pair<string, int>> sorted(freq.begin(), freq.end());
        sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) {
            if (a.second != b.second) return a.second > b.second;
            return a.first < b.first;
        });
        ostringstream json;
        json << "[";
        int limit = min((int)sorted.size(), 200);
        for (int i = 0; i < limit; i++) {
            if (i > 0) json << ",";
            json << "{\"word\":\"" << escapeJson(sorted[i].first)
                 << "\",\"freq\":" << sorted[i].second
                 << ",\"rank\":" << (i+1) << "}";
        }
        json << "]";
        safeCpy(outBuf, json.str(), bufSize);
        return limit;
    } catch (...) { safeCpy(outBuf, "[]", bufSize); return -1; }
}

__declspec(dllexport) int GetGlobalWordFreq(char* outBuf, int bufSize) {
    try {
        unordered_map<string, int> globalFreq;
        for (auto& doc : g_allDocs) {
            for (auto& w : doc.words) globalFreq[w]++;
        }
        vector<pair<string, int>> sorted(globalFreq.begin(), globalFreq.end());
        sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) {
            if (a.second != b.second) return a.second > b.second;
            return a.first < b.first;
        });
        ostringstream json;
        json << "[";
        int limit = min((int)sorted.size(), 500);
        for (int i = 0; i < limit; i++) {
            if (i > 0) json << ",";
            json << "{\"word\":\"" << escapeJson(sorted[i].first)
                 << "\",\"freq\":" << sorted[i].second
                 << ",\"rank\":" << (i+1) << "}";
        }
        json << "]";
        safeCpy(outBuf, json.str(), bufSize);
        return limit;
    } catch (...) { safeCpy(outBuf, "[]", bufSize); return -1; }
}

__declspec(dllexport) int DeleteDoc(int docId) {
    try {
        if (docId < 0 || docId >= (int)g_allDocs.size()) return -1;
        g_allDocs.erase(g_allDocs.begin() + docId);
        rebuildAll();
        return g_totalDocNum;
    } catch (...) { return -1; }
}

__declspec(dllexport) int DeleteAllDocs() {
    try { InitEngine(); return 0; } catch (...) { return -1; }
}

__declspec(dllexport) int Search(const char* query, int topK, char* outBuf, int bufSize) {
    try {
        vector<string> words = QuerySplit(string(query));
        if (words.empty()) { safeCpy(outBuf, "[]", bufSize); return 0; }
        set<int> matchDocIds = PhraseSearch(g_invertIndex, words);
        ScorePair results = GetTopKResult(matchDocIds, words, g_invertIndex, g_docWordLenArr, g_totalDocNum, topK);
        ostringstream json;
        json << "[";
        for (size_t i = 0; i < results.size(); i++) {
            if (i > 0) json << ",";
            int docId = results[i].first;
            double score = results[i].second;
            string filename = (docId >= 0 && docId < (int)g_allDocs.size()) ? g_allDocs[docId].fileName : "unknown";
            string snippet = (docId >= 0 && docId < (int)g_allDocs.size()) ? escapeJson(g_allDocs[docId].rawContent.substr(0,200)) : "";
            json << "{\"rank\":" << (i+1) << ",\"docId\":" << docId
                 << ",\"file\":\"" << escapeJson(filename) << "\""
                 << ",\"score\":" << score
                 << ",\"snippet\":\"" << snippet << "\"}";
        }
        json << "]";
        safeCpy(outBuf, json.str(), bufSize);
        return (int)results.size();
    } catch (...) { safeCpy(outBuf, "[]", bufSize); return -1; }
}

__declspec(dllexport) int TokenizeQuery(const char* text, char* outBuf, int bufSize) {
    try {
        string cleaned = TextClean(string(text));
        vector<string> words = SplitWord(cleaned);
        ostringstream json;
        json << "[";
        for (size_t i = 0; i < words.size(); i++) {
            if (i > 0) json << ",";
            json << "\"" << escapeJson(words[i]) << "\"";
        }
        json << "]";
        safeCpy(outBuf, json.str(), bufSize);
        return (int)words.size();
    } catch (...) { safeCpy(outBuf, "[]", bufSize); return -1; }
}

__declspec(dllexport) void ReleaseEngine() {
    g_allDocs.clear(); g_invertIndex.clear(); g_docWordLenArr.clear();
    g_totalDocNum = 0; g_currentFolder = "";
}

}
