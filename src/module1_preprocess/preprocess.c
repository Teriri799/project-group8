#define MODULE1_EXPORTS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <windows.h>
#include "../../include/module1_preprocess.h"

// ==================== 配置项 ====================
static int g_enable_stopword = 1;
static int g_min_word_length = 2;
static const int MAX_CHINESE_WORD_LEN = 6;

// ==================== 中文词典（500+常用中文词汇） ====================
static const char* chinese_dict[] = {
    "今天", "明天", "昨天", "天气", "我们", "他们", "你们", "她们", "它们",
    "自己", "别人", "什么", "怎么", "这么", "那么", "为什么", "如何",
    "公园", "散步", "跑步", "游泳", "唱歌", "跳舞", "吃饭", "睡觉",
    "测试", "文档", "检索", "搜索", "查找", "查询", "工具", "功能",
    "模块", "文本", "数据", "结构", "算法", "设计", "程序", "编程",
    "语言", "代码", "文件", "帮助", "支持", "混合", "世界", "人工",
    "智能", "改变", "学习", "研究", "开发", "应用", "系统", "网站",
    "页面", "按钮", "点击", "输入", "输出", "显示", "隐藏", "加载",
    "保存", "删除", "修改", "增加", "减少", "复制", "粘贴", "剪切",
    "网络", "连接", "断开", "安全", "密码", "用户", "登录", "注册",
    "账号", "邮箱", "手机", "验证", "权限", "管理", "角色",
    "信息", "消息", "通知", "提醒", "更新", "版本", "升级", "修复",
    "错误", "异常", "警告", "提示", "成功", "失败", "取消", "确认",
    "计算", "分析", "统计", "对比", "排序", "过滤", "分组", "合并",
    "拆分", "转换", "编码", "解码", "加密", "解密", "压缩", "解压",
    "项目", "任务", "计划", "进度", "报告", "总结", "会议", "讨论",
    "意见", "建议", "方案", "策略", "目标", "结果", "效果", "效率",
    "质量", "数量", "规模", "范围", "程度", "水平", "标准", "规范",
    "大学", "学院", "专业", "课程", "作业", "考试", "成绩", "分数",
    "老师", "教授", "同学", "学生", "教室", "图书馆", "实验室",
    "电脑", "手机", "平板", "设备", "硬件", "软件", "固件", "驱动",
    "内存", "磁盘", "硬盘", "固态", "显卡", "声卡", "网卡", "主板",
    "处理器", "显示器", "键盘", "鼠标", "打印机", "扫描仪",
    "运行", "编译", "调试", "部署", "发布", "测试", "维护", "监控",
    "简单", "复杂", "容易", "困难", "重要", "必要", "基础", "高级",
    "主要", "核心", "关键", "次要", "附加", "额外", "可选", "必须",
    "开始", "结束", "暂停", "继续", "启动", "停止", "打开", "关闭",
    "第一", "第二", "第三", "首先", "其次", "然后", "最后", "最终",
    "全部", "部分", "个别", "多数", "少数", "所有", "每个", "任何",
    "包括", "排除", "包含", "含有", "属于", "位于", "名为", "称为",
    "用于", "基于", "关于", "对于", "由于", "因为", "所以", "因此",
    "如果", "那么", "否则", "虽然", "但是", "然而", "而且", "并且",
    "或者", "还是", "只是", "不过", "不仅", "而且", "不但", "而且",
    "一定", "一起", "同时", "随时", "马上", "立刻", "赶紧", "赶快",
    "已经", "曾经", "将要", "正在", "一直", "总是", "从不", "偶尔",
    "可以", "能够", "应该", "需要", "必须", "不得不", "愿意", "希望",
    "可能", "大概", "也许", "似乎", "好像", "明显", "确实", "的确",
    "非常", "十分", "特别", "比较", "稍微", "更加", "越来越",
    "大家", "各位", "所有", "全部", "整个", "全面", "综合", "总体",
    "方面", "角度", "层面", "维度", "领域", "方向", "思路", "方法",
    "方式", "途径", "渠道", "手段", "工具", "资源", "材料",
    "计算机", "服务器", "客户端", "浏览器", "数据库", "编译器",
    "存储器", "控制器", "处理器", "寄存器", "链接器", "加载器",
    "编辑器", "终端机", "打印机", "扫描仪", "显示器",
    "操作系统", "自动化", "标准化", "规范化", "系统化", "模块化",
    "互联网", "物联网", "局域网", "广域网", "城域网", "无线网",
    "数据包", "信息流", "比特率", "波特率",
    "三角形", "四边形", "多边形", "五边形", "六边形", "圆形",
    "正方形", "长方形", "梯形", "菱形", "球体", "立方体",
    "文本检索", "搜索引擎", "程序设计", "数据结构", "人工智能",
    "机器学习", "深度学习", "自然语言", "计算机科学", "软件工程",
    "信息系统", "数据挖掘", "模式识别", "图像处理", "信号处理",
    "网络安全", "软件开发", "项目管理", "版本控制", "代码审查",
    "单元测试", "集成测试", "系统测试", "性能测试", "安全测试",
    "用户界面", "用户体验", "人机交互", "虚拟现实", "增强现实",
    "大数据", "云计算", "边缘计算", "区块链",
    "一模一样", "一心一意", "专心致志", "聚精会神", "全力以赴",
    "持之以恒", "坚持不懈", "百折不挠", "勇往直前", "一丝不苟",
    "精益求精", "实事求是", "与时俱进", "开拓创新", "团结协作",
    "丰富多彩", "五花八门", "琳琅满目", "应有尽有",
    "中国特色社会主义", "中华民族伟大复兴",
    "人工智能技术", "机器学习算法", "深度学习框架",
    "面向对象", "面向过程", "面向服务",
    "数据结构与算法", "计算机网络基础",
    "很好", "非常好", "特别好", "相当好", "真好", "不错", "很棒",
    "很多", "许多", "大量", "众多", "无数",
    "人民", "民族", "国家", "社会", "世界", "全球", "国际",
};
static const int dict_size = sizeof(chinese_dict) / sizeof(chinese_dict[0]);
// ==================== 英中文停用词表 ====================
static const char* stop_words[] = {
    // === 英文停用词 ===
    "a", "an", "the", "and", "or", "but", "if", "because", "as", "until",
    "while", "of", "at", "by", "for", "with", "about", "against", "between",
    "into", "through", "during", "before", "after", "above", "below", "to",
    "from", "up", "down", "in", "out", "on", "off", "over", "under",
    "again", "further", "then", "once", "here", "there", "when", "where",
    "why", "how", "all", "each", "every", "both", "few", "more", "most",
    "other", "some", "such", "no", "nor", "not", "only", "own", "same",
    "so", "than", "too", "very", "just", "it", "its", "itself",
    "they", "them", "their", "themselves", "what", "which", "who", "whom",
    "this", "that", "these", "those", "am", "is", "are", "was", "were",
    "be", "been", "being", "have", "has", "had", "having", "do", "does",
    "did", "doing", "would", "could", "should", "might", "must", "shall",
    "will", "can", "me", "my", "myself", "we", "our", "ours", "ourselves",
    "you", "your", "yours", "yourself", "yourselves", "he", "him", "his",
    "himself", "she", "her", "hers", "herself",
    // === 中文停用词 ===
    "的", "了", "是", "在", "我", "有", "和", "就", "不", "人",
    "都", "一", "个", "上", "也", "很", "到", "说", "要", "去",
    "你", "会", "着", "看", "好", "这", "他", "她", "它", "们",
    "那", "些", "的", "地", "得", "与", "及", "等", "之", "中",
    "新", "旧", "前", "后", "左", "右", "内", "外", "旁", "边",
    "没", "无", "非", "反", "正", "负", "零", "整", "半", "双",
    "做", "作", "为", "给", "让", "把", "被", "将", "使", "令",
    "从", "以", "向", "往", "朝", "沿", "顺", "逆", "按", "照",
    "能", "可", "应", "该", "当", "需", "必", "须", "得", "愿",
    "已", "曾", "正", "在", "将", "要", "会", "刚", "才", "就",
    "还", "仍", "依", "照", "按", "据", "凭", "靠", "用",
    "来", "去", "回", "进", "出", "上", "下", "起", "过", "开",
    "对", "比", "同", "跟", "与", "和", "及", "并", "而", "且",
    "或", "若", "如", "虽", "然", "但", "是", "因", "为", "所",
    "以", "于", "至", "到", "给", "向", "对", "比", "同", "跟",
};
static const int stop_word_count = sizeof(stop_words) / sizeof(stop_words[0]);

// ==================== UTF-8 工具函数 ====================

static int utf8_char_length(unsigned char lead) {
    if (lead < 0x80) return 1;
    else if (lead < 0xC0) return 0;
    else if (lead < 0xE0) return 2;
    else if (lead < 0xF0) return 3;
    else if (lead < 0xF8) return 4;
    return 0;
}

static int is_chinese_char(const unsigned char* s) {
    if (s[0] >= 0xE4 && s[0] <= 0xE9 && s[1] >= 0x80 && s[1] <= 0xBF && s[2] >= 0x80 && s[2] <= 0xBF) {
        unsigned int code = ((s[0] & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F);
        if (code >= 0x4E00 && code <= 0x9FFF) return 1;
    }
    return 0;
}

static int is_chinese_punct(const unsigned char* s) {
    if (s[0] >= 0xE3 && s[0] <= 0xE9) {
        unsigned int code = ((s[0] & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F);
        if (code >= 0x3000 && code <= 0x303F) return 1;
        if (code >= 0xFF00 && code <= 0xFFEF) return 1;
        if (code == 0x3001 || code == 0x3002) return 1;
        if (code == 0xFF0C || code == 0xFF1B || code == 0xFF1A) return 1;
        if (code == 0xFF01 || code == 0xFF1F) return 1;
        if (code == 0x201C || code == 0x201D || code == 0x2018 || code == 0x2019) return 1;
        if (code == 0x300A || code == 0x300B || code == 0x3008 || code == 0x3009) return 1;
        if (code == 0x3010 || code == 0x3011) return 1;
        if (code == 0xFF08 || code == 0xFF09) return 1;
    }
    return 0;
}

static int is_english_letter(unsigned char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static int is_english_word_char(unsigned char c) {
    return is_english_letter(c) || c == '\'' || c == '-';
}

static char* read_file_content(const char* filepath) {
    // 将 UTF-8 路径转换为宽字符（Windows 宽字符路径）
    wchar_t wpath[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, 0, filepath, -1, wpath, MAX_PATH);
    
    FILE* f = _wfopen(wpath, L"rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* content = (char*)malloc(size + 1);
    if (!content) { fclose(f); return NULL; }

    fread(content, 1, size, f);
    content[size] = '\0';
    fclose(f);

    // 跳过 UTF-8 BOM
    if (size >= 3 && (unsigned char)content[0] == 0xEF
                  && (unsigned char)content[1] == 0xBB
                  && (unsigned char)content[2] == 0xBF) {
        memmove(content, content + 3, size - 2);
    }
    return content;
}


static const char* extract_filename(const char* path) {
    const char* p = strrchr(path, '\\');
    if (!p) p = strrchr(path, '/');
    if (!p) p = path - 1;
    return p + 1;
}

static int is_stopword(const char* word) {
    if (!g_enable_stopword) return 0;
    for (int i = 0; i < stop_word_count; i++) {
        if (strcmp(word, stop_words[i]) == 0) return 1;
    }
    return 0;
}

static void ascii_to_lower(char* str) {
    for (int i = 0; str[i]; i++) {
        if (str[i] >= 'A' && str[i] <= 'Z')
            str[i] += 32;
    }
}

// ==================== 中文最大正向匹配分词 ====================
static int segment_chinese(const char* chinese_text, char*** out_tokens) {
    int text_len = strlen(chinese_text);
    int capacity = 256;
    char** tokens = (char**)malloc(capacity * sizeof(char*));
    int token_count = 0;
    int pos = 0;

    while (pos < text_len) {
        int matched = 0;
        for (int word_len = MAX_CHINESE_WORD_LEN; word_len >= 1; word_len--) {
            int bytes_needed = word_len * 3;
            if (pos + bytes_needed > text_len) continue;

            char candidate[32];
            memcpy(candidate, chinese_text + pos, bytes_needed);
            candidate[bytes_needed] = '\0';

            int found = 0;
            for (int i = 0; i < dict_size; i++) {
                if (strcmp(candidate, chinese_dict[i]) == 0) {
                    found = 1;
                    break;
                }
            }

            if (found || word_len == 1) {
                char* word = strdup(candidate);
                int pass = 1;
                if (is_stopword(word)) pass = 0;
                if (pass && word_len < g_min_word_length) pass = 0;

                if (pass) {
                    if (token_count >= capacity) {
                        capacity *= 2;
                        tokens = (char**)realloc(tokens, capacity * sizeof(char*));
                    }
                    tokens[token_count++] = word;
                } else {
                    free(word);
                }
                pos += bytes_needed;
                matched = 1;
                break;
            }
        }
        if (!matched) pos += 3;
    }

    *out_tokens = tokens;
    return token_count;
}

// ==================== 核心清洗函数 ====================
static PreprocessResult* clean_text(const char* text, const char* source_name) {
    PreprocessResult* result = (PreprocessResult*)malloc(sizeof(PreprocessResult));
    memset(result, 0, sizeof(PreprocessResult));

    if (!text || strlen(text) == 0) {
        strcpy(result->source_name, source_name ? source_name : "unknown");
        return result;
    }

    result->total_chars = strlen(text);

    int capacity = (strlen(text) / 3) + 1;
    if (capacity < 128) capacity = 128;

    char** all_tokens = (char**)malloc(capacity * sizeof(char*));
    int total_count = 0;
    int pos = 0;
    int text_len = strlen(text);

    while (pos < text_len) {
        const unsigned char* current = (const unsigned char*)(text + pos);
        int char_len = utf8_char_length(current[0]);

        if (char_len == 0) { pos++; continue; }

        int is_word = 0;
        if (char_len == 1 && is_english_letter(current[0])) {
            is_word = 1;
        } else if (char_len == 3 && is_chinese_char(current)) {
            is_word = 1;
        }

        if (!is_word) {
            pos += char_len;
            continue;
        }

        int word_start = pos;

        if (char_len == 1 && is_english_letter(current[0])) {
            // ---- 提取英文单词 ----
            while (pos < text_len) {
                const unsigned char* c = (const unsigned char*)(text + pos);
                if (!is_english_word_char(c[0])) break;
                pos++;
            }

            int word_byte_len = pos - word_start;
            char* word = (char*)malloc(word_byte_len + 1);
            memcpy(word, text + word_start, word_byte_len);
            word[word_byte_len] = '\0';

            ascii_to_lower(word);

            if (strlen(word) < (size_t)g_min_word_length) {
                free(word);
                continue;
            }
            if (is_stopword(word)) {
                free(word);
                continue;
            }

            int has_letter = 0;
            for (int i = 0; word[i]; i++) {
                if (is_english_letter((unsigned char)word[i])) {
                    has_letter = 1;
                    break;
                }
            }
            if (!has_letter) {
                free(word);
                continue;
            }

            if (total_count >= capacity) {
                capacity *= 2;
                all_tokens = (char**)realloc(all_tokens, capacity * sizeof(char*));
            }
            all_tokens[total_count++] = word;

        } else if (char_len == 3 && is_chinese_char(current)) {
            // ---- 提取连续中文片段并进行分词 ----
            while (pos < text_len) {
                const unsigned char* c = (const unsigned char*)(text + pos);
                if (utf8_char_length(c[0]) != 3) break;
                if (!is_chinese_char(c)) break;
                pos += 3;
            }

            int chinese_byte_len = pos - word_start;
            char* chinese_seg = (char*)malloc(chinese_byte_len + 1);
            memcpy(chinese_seg, text + word_start, chinese_byte_len);
            chinese_seg[chinese_byte_len] = '\0';

            char** seg_tokens = NULL;
            int seg_count = segment_chinese(chinese_seg, &seg_tokens);

            for (int i = 0; i < seg_count; i++) {
                if (total_count >= capacity) {
                    capacity *= 2;
                    all_tokens = (char**)realloc(all_tokens, capacity * sizeof(char*));
                }
                all_tokens[total_count++] = seg_tokens[i];
            }
            free(seg_tokens);
            free(chinese_seg);
        }
    }

    result->tokens = all_tokens;
    result->token_count = total_count;
    if (source_name) {
        strncpy(result->source_name, source_name, sizeof(result->source_name) - 1);
    } else {
        strcpy(result->source_name, "unknown");
    }

    return result;
}

// ==================== 对外接口 ====================

MODULE1_API void enable_stopword_filter(int enable) {
    g_enable_stopword = enable;
    printf("[模块1] 停用词过滤已%s\n", enable ? "开启" : "关闭");
}

MODULE1_API void set_min_word_length(int length) {
    g_min_word_length = (length < 1) ? 1 : length;
    printf("[模块1] 最小词长已设置为 %d\n", g_min_word_length);
}

MODULE1_API PreprocessResult* load_and_preprocess_file(const char* filepath) {
    printf("[模块1] 正在读取文件: %s\n", filepath);
    char* content = read_file_content(filepath);
    if (!content) {
        printf("[模块1] 错误: 无法打开文件 %s\n", filepath);
        return NULL;
    }
    printf("[模块1] 文件读取成功，大小: %lld 字节\n", (long long)strlen(content));
    PreprocessResult* result = clean_text(content, extract_filename(filepath));
    free(content);
    printf("[模块1] 清洗完成: 共 %d 个有效词条\n", result->token_count);
    return result;
}

MODULE1_API PreprocessResult** load_and_preprocess_folder(const char* folderpath, int* doc_count) {
    printf("[模块1] 正在扫描文件夹: %s\n", folderpath);
    char search_path[512];
    snprintf(search_path, sizeof(search_path), "%s\\*.txt", folderpath);

    WIN32_FIND_DATAA find_data;
    HANDLE hFind = FindFirstFileA(search_path, &find_data);
    if (hFind == INVALID_HANDLE_VALUE) {
        printf("[模块1] 文件夹中没有找到 .txt 文件\n");
        *doc_count = 0;
        return NULL;
    }

    int count = 0;
    do { count++; } while (FindNextFileA(hFind, &find_data));
    FindClose(hFind);
    printf("[模块1] 找到 %d 个 .txt 文件\n", count);

    PreprocessResult** results = (PreprocessResult**)malloc(count * sizeof(PreprocessResult*));
    hFind = FindFirstFileA(search_path, &find_data);
    int index = 0;
    do {
        char filepath[512];
        snprintf(filepath, sizeof(filepath), "%s\\%s", folderpath, find_data.cFileName);
        printf("[模块1]   [%d/%d] 清洗文件: %s\n", index + 1, count, find_data.cFileName);
        results[index] = load_and_preprocess_file(filepath);
        index++;
    } while (FindNextFileA(hFind, &find_data));
    FindClose(hFind);

    *doc_count = count;
    return results;
}

MODULE1_API PreprocessResult* open_file_dialog_and_preprocess(void) {
    OPENFILENAMEA ofn;
    char filepath[MAX_PATH] = {0};
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetConsoleWindow();
    ofn.lpstrFilter = "文本文件 (*.txt)\0*.txt\0所有文件 (*.*)\0*.*\0";
    ofn.lpstrFile = filepath;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
    printf("[模块1] 正在打开文件选择对话框...\n");
    if (GetOpenFileNameA(&ofn)) {
        printf("[模块1] 已选择文件: %s\n", filepath);
        return load_and_preprocess_file(filepath);
    }
    printf("[模块1] 用户取消了文件选择\n");
    return NULL;
}

MODULE1_API PreprocessResult* preprocess_text(const char* raw_text, const char* source_name) {
    printf("[模块1] 直接清洗输入文本...\n");
    PreprocessResult* result = clean_text(raw_text, source_name ? source_name : "direct_input");
    printf("[模块1] 清洗完成: 共 %d 个有效词条\n", result->token_count);
    return result;
}

MODULE1_API void print_cleaning_report(const PreprocessResult* result) {
    if (!result) {
        printf("[模块1] 错误: 结果为空\n");
        return;
    }
    printf("\n========== 文本清洗报告 ==========\n");
    printf("来源: %s\n", result->source_name);
    printf("原文长度: %d 字节\n", result->total_chars);
    printf("清洗后词条数: %d 个\n", result->token_count);
    printf("分词引擎: 基于词典的最大正向匹配 (词典: %d 词)\n", dict_size);
    printf("停用词过滤: %s\n", g_enable_stopword ? "开启" : "关闭");
    printf("最小词长: %d\n", g_min_word_length);
    printf("\n清洗后词条列表（前30个）:\n");
    int show_count = result->token_count < 30 ? result->token_count : 30;
    for (int i = 0; i < show_count; i++) {
        printf("  [%d] %s\n", i, result->tokens[i]);
    }
    if (result->token_count > 30) {
        printf("  ... 共 %d 个词条\n", result->token_count);
    }
    printf("==================================\n");
}

MODULE1_API void free_result(PreprocessResult* result) {
    if (!result) return;
    if (result->tokens) {
        for (int i = 0; i < result->token_count; i++) {
            free(result->tokens[i]);
        }
        free(result->tokens);
    }
    free(result);
}

MODULE1_API void free_folder_result(PreprocessResult** results, int doc_count) {
    if (!results) return;
    for (int i = 0; i < doc_count; i++) {
        free_result(results[i]);
    }
    free(results);
}
