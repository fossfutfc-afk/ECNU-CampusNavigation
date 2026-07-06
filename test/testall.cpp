//
// CampusNavigation 测试框架
// testall.cpp — 批量测试驱动程序
//
// 用法：
//   testall.exe                             一键全测试
//   testall.exe --small                      仅小用例
//   testall.exe --medium                     仅中等用例
//   testall.exe --large                      仅大用例
//   testall.exe --extend                     仅拓展用例
//   testall.exe --ecnu                       仅 ECNU 样例
//   testall.exe --case <name>                仅指定用例（如 case_01）
//   testall.exe --help                       显示帮助
//
// 编译：
//   g++ -std=c++17 -O2 -o test/testall.exe test/testall.cpp
//

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <conio.h>

#include <windows.h>

// ==================== 平台常量 ====================

#define PATHSEP '\\'

// ==================== ANSI 颜色 ====================

void EnableVT() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
}

#define CLR_RESET   "\033[0m"
#define CLR_GREEN   "\033[0;32m"
#define CLR_RED     "\033[0;31m"
#define CLR_YELLOW  "\033[1;33m"
#define CLR_CYAN    "\033[0;36m"
#define CLR_BOLD    "\033[1m"
#define CLR_GRAY    "\033[0;90m"

// ==================== 工具函数 ====================

// 去除字符串末尾的 \r 和 \n
static void TrimCRLF(std::string &s) {
    while (!s.empty() && (s.back() == '\r' || s.back() == '\n'))
        s.pop_back();
}

// 去除字符串首尾空白
static void Trim(std::string &s) {
    size_t start = 0;
    while (start < s.size() && (s[start] == ' ' || s[start] == '\t'))
        ++start;
    size_t end = s.size();
    while (end > start && (s[end-1] == ' ' || s[end-1] == '\t' || s[end-1] == '\r' || s[end-1] == '\n'))
        --end;
    s = s.substr(start, end - start);
}

// 按空格分割字符串
static std::vector<std::string> Split(const std::string &s) {
    std::vector<std::string> tokens;
    std::istringstream iss(s);
    std::string token;
    while (iss >> token) tokens.push_back(token);
    return tokens;
}

// 检查文件是否存在
static bool FileExists(const std::string &path) {
    struct _stat buffer;
    return (_stat(path.c_str(), &buffer) == 0);
}

// ==================== 递归目录遍历 ====================

static void CollectTestDirsRecursive(const std::string &base,
                                      std::vector<std::string> &dirs) {
    std::string pattern = base + "\\*";
    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(pattern.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        std::string name = fd.cFileName;
        if (name == "." || name == "..") continue;

        std::string fullPath = base + "\\" + name;

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            CollectTestDirsRecursive(fullPath, dirs);
        } else if (name == "command.txt") {
            // 只要有 command.txt 就算一个测试用例（answer.txt 可选，运行后自动生成）
            dirs.push_back(base);
        }
    } while (FindNextFileA(hFind, &fd));
    FindClose(hFind);
}

// 收集 data 目录下所有包含 answer.txt 的测试用例目录
static void CollectTestDirs(const std::string &base,
                            std::vector<std::string> &dirs) {
    CollectTestDirsRecursive(base, dirs);
    std::sort(dirs.begin(), dirs.end());
}

// 获取路径的最后一部分（目录名或文件名）
static std::string BaseName(const std::string &path) {
    size_t pos = path.find_last_of("/\\");
    if (pos == std::string::npos) return path;
    return path.substr(pos + 1);
}

// 获取路径的倒数第二部分
static std::string ParentName(const std::string &path) {
    std::string parent = path;
    while (!parent.empty() && (parent.back() == '/' || parent.back() == '\\'))
        parent.pop_back();
    size_t pos = parent.find_last_of("/\\");
    if (pos == std::string::npos) return "";
    parent = parent.substr(0, pos);
    pos = parent.find_last_of("/\\");
    if (pos == std::string::npos) return parent;
    return parent.substr(pos + 1);
}

// ==================== 语义等价判断 ====================

//
// 判断两行输出是否语义等价（而非字符级一致）
// 用于处理 Dijkstra 等权边 tie-breaking、MST 等权边不同选择等情况
//
static bool IsSemanticMatch(const std::string &expect,
                            const std::string &actual) {
    // PATH <DIST|TIME> <cost> NODES <nodes...>
    if (expect.compare(0, 5, "PATH ") == 0 &&
        actual.compare(0, 5, "PATH ") == 0) {
        auto et = Split(expect);
        auto at = Split(actual);
        if (et.size() < 5 || at.size() < 5) return false;
        if (et[0] != "PATH" || at[0] != "PATH") return false;

        std::string eMode = et[1], aMode = at[1];
        std::string eCost = et[2], aCost = at[2];

        auto findNodes = [](const std::vector<std::string> &t) -> int {
            for (size_t i = 0; i < t.size(); ++i)
                if (t[i] == "NODES") return static_cast<int>(i);
            return -1;
        };
        int eNi = findNodes(et), aNi = findNodes(at);
        if (eNi < 0 || aNi < 0) return false;

        std::string eStart = (eNi + 1 < (int)et.size()) ? et[eNi + 1] : "";
        std::string eEnd   = et.back();
        std::string aStart = (aNi + 1 < (int)at.size()) ? at[aNi + 1] : "";
        std::string aEnd   = at.back();

        return (eMode == aMode) && (eCost == aCost) &&
               (eStart == aStart) && (eEnd == aEnd);
    }

    // MST <total> EDGES <edges...>
    if (expect.compare(0, 4, "MST ") == 0 &&
        actual.compare(0, 4, "MST ") == 0) {
        auto et = Split(expect);
        auto at = Split(actual);
        if (et.size() < 3 || at.size() < 3) return false;
        std::string eTotal = et[1], aTotal = at[1];
        int eEdgeCount = static_cast<int>(et.size()) - 3;
        int aEdgeCount = static_cast<int>(at.size()) - 3;
        return (eTotal == aTotal) && (eEdgeCount == aEdgeCount);
    }

    return false;
}

// ==================== 测试结果类型 ====================

enum class Verdict {
    AC,      // Accepted — 与 answer.txt 完全一致
    SEMANTIC,// 语义一致 — 输出不同但语义等价
    WA,      // Wrong Answer — 真实差异
    NEW,     // 新用例 — 首次运行，已自动生成 answer.txt
    CRASH,   // 程序崩溃或运行失败
    SKIP     // 跳过（缺少 command.txt）
};

struct CaseResult {
    std::string case_name;
    std::string group;
    Verdict verdict;
    double elapsed_ms;
    std::vector<std::string> diffs;
};

// ==================== 单个用例测试 ====================

static CaseResult RunOneCase(const std::string &test_dir,
                              const std::string &exe_path) {
    CaseResult res;
    res.case_name = BaseName(test_dir);
    res.group = ParentName(test_dir);
    res.verdict = Verdict::SKIP;
    res.elapsed_ms = 0;

    std::string cmd_file  = test_dir + PATHSEP + "command.txt";
    std::string ans_file  = test_dir + PATHSEP + "answer.txt";

    // command.txt 是必须的
    if (!FileExists(cmd_file)) {
        return res;
    }

    bool has_answer = FileExists(ans_file);

    std::string tmp_out = test_dir + PATHSEP + "_testall_tmp.txt";

    std::string runCmd = "cd /d \"" + test_dir + "\" && \"" + exe_path +
                         "\" < \"" + cmd_file + "\" > \"" + tmp_out + "\" 2>nul";

    auto t1 = std::chrono::high_resolution_clock::now();
    int rc = system(runCmd.c_str());
    auto t2 = std::chrono::high_resolution_clock::now();
    res.elapsed_ms = std::chrono::duration<double, std::milli>(t2 - t1).count();

    // 读取程序输出
    std::vector<std::string> output;
    {
        std::ifstream fin(tmp_out);
        if (fin.is_open()) {
            std::string line;
            while (std::getline(fin, line)) {
                TrimCRLF(line);
                output.push_back(line);
            }
        }
    }
    std::remove(tmp_out.c_str());

    // 若 answer.txt 不存在 → 自动生成并标记为 NEW
    if (!has_answer) {
        std::ofstream fout(ans_file);
        if (fout.is_open()) {
            for (size_t i = 0; i < output.size(); ++i) {
                fout << output[i];
                if (i + 1 < output.size()) fout << "\n";
            }
        }
        res.verdict = Verdict::NEW;
        return res;
    }

    // 读取参考输出
    std::vector<std::string> answer;
    {
        std::ifstream fin(ans_file);
        std::string line;
        while (std::getline(fin, line)) {
            TrimCRLF(line);
            answer.push_back(line);
        }
    }

    // 逐行对比
    bool exact_match = true;
    bool all_semantic = true;
    size_t maxLines = (std::max)(answer.size(), output.size());

    for (size_t i = 0; i < maxLines; ++i) {
        std::string aLine = (i < answer.size()) ? answer[i] : "";
        std::string oLine = (i < output.size()) ? output[i] : "";

        while (!aLine.empty() && aLine.back() == '\r') aLine.pop_back();
        while (!oLine.empty() && oLine.back() == '\r') oLine.pop_back();

        if (aLine == oLine) continue;

        if (aLine.empty() && oLine.empty()) continue;
        if (aLine.empty() && i >= output.size()) continue;
        if (oLine.empty() && i >= answer.size()) continue;

        exact_match = false;

        if (!IsSemanticMatch(aLine, oLine)) {
            all_semantic = false;
        }

        if (res.diffs.size() < 6) {
            res.diffs.push_back("  expect: " + aLine);
            res.diffs.push_back("  actual: " + oLine);
        }
    }

    if (exact_match) {
        res.verdict = Verdict::AC;
    } else if (all_semantic) {
        res.verdict = Verdict::SEMANTIC;
    } else {
        res.verdict = Verdict::WA;
    }

    return res;
}

// ==================== 输出函数 ====================

static void PrintHeader() {
    std::cout << CLR_BOLD
              << "========================================\n"
              << "  CampusNavigation 批量测试\n"
              << "========================================\n"
              << CLR_RESET;
}

static const char* VerdictTag(Verdict v) {
    switch (v) {
        case Verdict::AC:       return " AC";
        case Verdict::SEMANTIC: return "SEM";
        case Verdict::WA:       return " WA";
        case Verdict::NEW:      return "NEW";
        case Verdict::CRASH:    return "ERR";
        case Verdict::SKIP:     return "SKIP";
        default:                return " ??";
    }
}

static const char* VerdictColor(Verdict v) {
    switch (v) {
        case Verdict::AC:       return CLR_GREEN;
        case Verdict::SEMANTIC: return CLR_YELLOW;
        case Verdict::WA:       return CLR_RED;
        case Verdict::NEW:      return CLR_CYAN;
        case Verdict::CRASH:    return CLR_RED;
        case Verdict::SKIP:     return CLR_GRAY;
        default:                return CLR_RESET;
    }
}

static void PrintSummary(const std::vector<CaseResult> &results) {
    int ac = 0, sem = 0, wa = 0, nnew = 0, crash = 0, skip = 0;

    for (const auto &r : results) {
        switch (r.verdict) {
            case Verdict::AC:       ++ac; break;
            case Verdict::SEMANTIC: ++sem; break;
            case Verdict::WA:       ++wa; break;
            case Verdict::CRASH:    ++crash; break;
            case Verdict::NEW:      ++nnew; break;
            case Verdict::SKIP:     ++skip; break;
        }
    }

    int total = ac + sem + wa;
    std::cout << "\n" << CLR_BOLD
              << "========================================\n" << CLR_RESET;
    if (wa == 0 && crash == 0) {
        std::cout << CLR_GREEN << CLR_BOLD
                  << "  结果: " << ac << "/" << total << " 全部通过"
                  << CLR_RESET << "\n";
    } else {
        std::cout << CLR_RED << CLR_BOLD
                  << "  结果: " << wa << "/" << total << " 存在差异"
                  << CLR_RESET << "\n";
    }
    if (sem > 0) {
        std::cout << CLR_YELLOW
                  << "  语义等价: " << sem << "  (输出不同但语义一致)"
                  << CLR_RESET << "\n";
    }
    if (nnew > 0) {
        std::cout << CLR_CYAN
                  << "  新用例: " << nnew << "  (已自动生成 answer.txt)"
                  << CLR_RESET << "\n";
    }
    if (crash > 0) {
        std::cout << CLR_RED
                  << "  崩溃: " << crash
                  << CLR_RESET << "\n";
    }
    if (skip > 0) {
        std::cout << CLR_GRAY
                  << "  跳过: " << skip << " (缺少文件)"
                  << CLR_RESET << "\n";
    }
    std::cout << "  总计: " << results.size() << " cases\n";
    std::cout << CLR_BOLD
              << "========================================\n"
              << CLR_RESET;
}

// 最后输出所有用例的 AC 情况与运行时间
static void PrintAllResults(const std::vector<CaseResult> &results) {
    std::cout << "\n" << CLR_BOLD
              << "============ 全部用例运行时间 ============\n"
              << CLR_RESET;

    int nameW = 22;
    for (const auto &r : results) {
        int len = (int)r.case_name.size();
        if (len > nameW) nameW = len;
    }
    nameW += 2;

    for (const auto &r : results) {
        // 时间格式化
        std::string timeStr;
        if (r.elapsed_ms < 1000.0)
            timeStr = std::to_string((int)r.elapsed_ms) + "ms";
        else {
            char buf[32];
            snprintf(buf, sizeof(buf), "%.2fs", r.elapsed_ms / 1000.0);
            timeStr = buf;
        }

        std::cout << "  " << VerdictColor(r.verdict) << "[ " << VerdictTag(r.verdict)
                  << " ]" << CLR_RESET
                  << "  " << r.case_name;
        // 对齐
        int pad = nameW - (int)r.case_name.size();
        for (int i = 0; i < pad; ++i) std::cout << ' ';
        std::cout << timeStr << "\n";
    }

    std::cout << CLR_BOLD
              << "============================================\n"
              << CLR_RESET;
}

static void PrintHelp() {
    std::cout << R"(CampusNavigation 测试框架

用法:
  testall.exe [选项]

选项:
  --small         仅测试小用例 (data/Must/small_cases/)
  --medium        仅测试中等用例 (data/Must/medium_cases/)
  --large         仅测试大用例 (data/Must/large_cases/)
  --extend        仅测试拓展用例 (data/Addition/)
  --ecnu          仅测试 ECNU 样例 (data/Must/sample_ecnu/)
  --case <name>   仅测试指定用例（如 case_01）
  --help          显示此帮助

无参数时运行所有测试用例。

状态说明:
  " AC "  绿色 — 输出与参考答案完全一致
  " SEM"  黄色 — 输出不同但语义等价（如最短路径等权边选择了不同合法路径）
  " WA "  红色 — 真实差异，需排查
  "CRASH" 红色 — 程序运行异常
)";
}

// ==================== 主函数 ====================

int main(int argc, char *argv[]) {
    EnableVT();
    SetConsoleOutputCP(CP_UTF8);

    // -------------------- 参数解析 --------------------
    std::string filter;
    std::string case_name;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--small")   filter = "small_cases";
        else if (arg == "--medium")  filter = "medium_cases";
        else if (arg == "--large")   filter = "large_cases";
        else if (arg == "--extend")  filter = "Addition";
        else if (arg == "--ecnu")    filter = "sample_ecnu";
        else if (arg == "--case") {
            if (i + 1 < argc) case_name = argv[++i];
            else { std::cerr << "错误: --case 需要参数\n"; return 1; }
        }
        else if (arg == "--help" || arg == "-h") {
            PrintHelp();
            std::cout << "\n按任意键退出...";
            _getch();
            return 0;
        }
        else {
            std::cerr << "未知参数: " << arg << "\n使用 --help 查看帮助\n";
            return 1;
        }
    }

    // -------------------- 定位可执行文件和数据目录 --------------------
    std::string harness_dir;
    {
        std::string argv0 = argv[0];
        size_t pos = argv0.find_last_of("/\\");
        if (pos != std::string::npos)
            harness_dir = argv0.substr(0, pos);
        else
            harness_dir = ".";
    }

    // CampusNavigation.exe 在 ../build/ 下
    std::string exe_path = harness_dir + "\\..\\build\\CampusNavigation.exe";
    {
        char full[MAX_PATH];
        if (GetFullPathNameA(exe_path.c_str(), sizeof(full), full, nullptr))
            exe_path = full;
    }

    if (!FileExists(exe_path)) {
        std::cerr << CLR_RED << "[ERROR] 找不到 " << exe_path << CLR_RESET << "\n";
        std::cerr << "请先编译:\n";
        std::cerr << "  g++ -std=c++17 -O2 -o build/CampusNavigation.exe src/*.cpp -I include\n";
        std::cout << "\n按任意键退出...";
        _getch();
        return 1;
    }

    // 数据目录在 ../data/ 下
    std::string data_root = harness_dir + "\\..\\data";
    {
        char full[MAX_PATH];
        if (GetFullPathNameA(data_root.c_str(), sizeof(full), full, nullptr))
            data_root = full;
    }

    PrintHeader();
    std::cout << "可执行文件: " << CLR_CYAN << exe_path << CLR_RESET << "\n";
    std::cout << "测试根目录: " << CLR_CYAN << data_root << CLR_RESET << "\n\n";

    // -------------------- 收集测试用例 --------------------
    std::vector<std::string> test_dirs;
    CollectTestDirs(data_root, test_dirs);

    std::vector<std::string> filtered;
    for (const auto &d : test_dirs) {
        if (!filter.empty() && d.find(filter) == std::string::npos)
            continue;
        if (!case_name.empty() && d.find(case_name) == std::string::npos)
            continue;
        filtered.push_back(d);
    }
    test_dirs = std::move(filtered);

    if (test_dirs.empty()) {
        std::cout << CLR_YELLOW << "没有找到匹配的测试用例。" << CLR_RESET << "\n";
        std::cout << "\n按任意键退出...";
        _getch();
        return 0;
    }

    // -------------------- 逐用例测试 --------------------
    std::vector<CaseResult> results;
    std::string current_group;
    int nameColW = 28;

    for (const auto &d : test_dirs) {
        int len = (int)BaseName(d).size();
        if (len > nameColW) nameColW = len;
    }
    nameColW += 2;

    for (const auto &test_dir : test_dirs) {
        std::string group = ParentName(test_dir);

        if (group != current_group) {
            std::cout << "\n" << CLR_BOLD << "--- " << group << " ---"
                      << CLR_RESET << "\n";
            current_group = group;
        }

        CaseResult res = RunOneCase(test_dir, exe_path);
        results.push_back(res);

        const char *tag = VerdictTag(res.verdict);
        const char *clr = VerdictColor(res.verdict);

        std::string timeStr;
        if (res.elapsed_ms < 1000.0)
            timeStr = std::to_string((int)res.elapsed_ms) + "ms";
        else {
            char buf[32];
            snprintf(buf, sizeof(buf), "%.1fs", res.elapsed_ms / 1000.0);
            timeStr = buf;
        }

        char line[256];
        snprintf(line, sizeof(line), "[%s%4s" CLR_RESET "] %-*s %8s",
                 clr, tag, nameColW, res.case_name.c_str(), timeStr.c_str());
        std::cout << line;

        if (res.verdict == Verdict::SEMANTIC) {
            std::cout << "  " CLR_YELLOW "(semantic match)" CLR_RESET;
        }
        std::cout << "\n";

        // 显示差异
        if (res.verdict == Verdict::SEMANTIC || res.verdict == Verdict::WA) {
            for (const auto &d : res.diffs) {
                if (d.compare(0, 10, "  expect: ") == 0)
                    std::cout << CLR_GRAY << d << CLR_RESET << "\n";
                else
                    std::cout << CLR_RED << d << CLR_RESET << "\n";
            }
        }
    }

    // -------------------- 汇总 --------------------
    PrintSummary(results);

    // -------------------- 全部用例 AC 情况与运行时间 --------------------
    PrintAllResults(results);

    // -------------------- 暂停以便在终端观测数据 --------------------
    std::cout << "\n按任意键退出...";
    _getch();

    for (const auto &r : results)
        if (r.verdict == Verdict::WA || r.verdict == Verdict::CRASH)
            return 1;
    return 0;
}
