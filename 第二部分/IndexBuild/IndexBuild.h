#ifndef INDEX_BUILD_H
#define INDEX_BUILD_H
#include "../../common/common.h"
#define DLL_EXPORT __declspec(dllexport)

extern "C" DLL_EXPORT std::tuple<InvertIndex, std::vector<int>, int> BuildInvertIndex(const std::vector<DocInfo>& docData);
#endif
