#include "common/common.h"
#include <sstream>
#include <cstring>

// 这些函数在其他 .cpp 文件中定义
extern "C" {
    __declspec(dllexport) std::vector<DocInfo> LoadAndPreprocessFile(const std::string& dirPath);
    __declspec(dllexport) std::string TextClean(std::string text);
    __declspec(dllexport) std::vector<std::string> SplitWord(std::string text);
    __declspec(dllexport) std::tuple<InvertIndex, std::vector<int>, int> BuildInvertIndex(const std::vector<DocInfo>& docData);
    __declspec(dllexport) std::vector<std::string> QuerySplit(const std::string& query);
    __declspec(dllexport) std::set<int> PhraseSearch(const InvertIndex&, const std::vector<std::string>&);
    __declspec(dllexport) ScorePair GetTopKResult(std::set<int>, std::vector<std::string>, InvertIndex, std::vector<int>, int, int);
}

// 全局状态
static std::vector<DocInfo> g_allDocs;
static InvertIndex g_invertIndex;
static std::vector<int> g_docWordLenArr;
static int g_totalDocNum = 0;

// ========== JSON 助手 ==========
static std::string escapeJson(const std::string& s) {
    std::string out;
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

static void safeCpy(char* dst, const std::string& src, int sz) {
    strncpy(dst, src.c_str(), sz - 1);
    dst[sz - 1] = '\0';
}

// ========== 导出 C 风格函数（Python 可用 ctypes 调用）==========
extern "C" {

__declspec(dllexport) int InitEngine() {
    g_allDocs.clear();
    g_invertIndex.clear();
    g_docWordLenArr.clear();
    g_totalDocNum = 0;
    return 0;
}

__declspec(dllexport) int LoadDocuments(const char* path) {
    try {
        g_allDocs = LoadAndPreprocessFile(std::string(path));
        if (g_allDocs.empty()) return 0;
        auto result = BuildInvertIndex(g_allDocs);
        g_invertIndex = std::get<0>(result);
        g_docWordLenArr = std::get<1>(result);
        g_totalDocNum = std::get<2>(result);
        return g_totalDocNum;
    } catch (...) { return -1; }
}

__declspec(dllexport) int GetDocCount() {
    return g_totalDocNum;
}

__declspec(dllexport) int Search(const char* query, int topK, char* outBuf, int bufSize) {
    try {
        std::vector<std::string> words = QuerySplit(std::string(query));
        if (words.empty()) { safeCpy(outBuf, "[]", bufSize); return 0; }

        std::set<int> matchDocIds = PhraseSearch(g_invertIndex, words);
        ScorePair results = GetTopKResult(matchDocIds, words, g_invertIndex, g_docWordLenArr, g_totalDocNum, topK);

        std::ostringstream json;
        json << "[";
        for (size_t i = 0; i < results.size(); i++) {
            if (i > 0) json << ",";
            int docId = results[i].first;
            double score = results[i].second;
            std::string filename = (docId >= 0 && docId < (int)g_allDocs.size())
                ? g_allDocs[docId].fileName : "未知";
            json << "{"
                 << "\"rank\":" << (i+1) << ","
                 << "\"docId\":" << docId << ","
                 << "\"file\":\"" << escapeJson(filename) << "\","
                 << "\"score\":" << score
                 << "}";
        }
        json << "]";
        safeCpy(outBuf, json.str(), bufSize);
        return (int)results.size();
    } catch (...) { safeCpy(outBuf, "[]", bufSize); return -1; }
}

__declspec(dllexport) int TokenizeQuery(const char* text, char* outBuf, int bufSize) {
    try {
        std::string cleaned = TextClean(std::string(text));
        std::vector<std::string> words = SplitWord(cleaned);
        std::ostringstream json;
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
    g_allDocs.clear();
    g_invertIndex.clear();
    g_docWordLenArr.clear();
    g_totalDocNum = 0;
}

} // extern "C"
