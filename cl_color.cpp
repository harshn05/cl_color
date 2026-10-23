// cl_color.cpp - compiler launcher: runs the given command and colorizes
// MSVC diagnostics (path, line/col, severity, code, message, caret) with ANSI codes.
// Build: cl /nologo /O2 /EHsc /std:c++17 cl_color.cpp
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <string>

// ---- palette (edit to taste) -------------------------------------------
#define RESET   "\x1b[0m"
#define C_PATH  "\x1b[94m"      // bright blue   : file path
#define C_LOC   "\x1b[95m"      // bright magenta: (line,col)
#define C_ERR   "\x1b[1;91m"    // bold bright red
#define C_WARN  "\x1b[1;93m"    // bold bright yellow
#define C_NOTE  "\x1b[1;96m"    // bold bright cyan
#define C_CODE  "\x1b[4m"       // underline     : C4018 etc. (combined with severity color)
#define C_MSG_E "\x1b[1;97m"    // bold white    : error message text
#define C_MSG_W "\x1b[97m"      // white         : warning/note message text
#define C_CARET "\x1b[1;92m"    // bold bright green: ^~~~ line
#define C_SRC   "\x1b[37m"      // light gray    : source snippet line
#define C_WITH  "\x1b[1;95m"    // bold magenta  : "with" keyword
#define C_BRK   "\x1b[37m"      // light gray    : [ ] brackets and '=' (90 was too dark on this theme)
#define C_GUT   "\x1b[38;5;245m" // gray         : "  64 | " gutter
#define C_CMT   "\x1b[38;5;108m" // muted green  : // comments in source snippet
#define C_QUOTE "\x1b[1;38;5;215m" // bold orange: 'quoted' names inside messages
#define C_TNAME "\x1b[96m"      // bright cyan   : template param name (T)
#define C_TVAL  "\x1b[93m"      // bright yellow : template argument (NoAdditionOperator)

enum Sev { NONE, ERR, WARN, NOTE };

static bool is_caret_line(const std::string& l) {
    bool caret = false;
    for (char c : l) {
        if (c == '^') caret = true;
        else if (c != ' ' && c != '~' && c != '\t' && c != '\r') return false;
    }
    return caret;
}

// Find ": error" / ": fatal error" / ": warning" / ": note" followed by ' ' or ':'.
static bool find_severity(const std::string& l, size_t& pos, size_t& wordStart,
                          size_t& wordEnd, Sev& sev) {
    struct Rule { const char* key; Sev sev; };
    static const Rule rules[] = {
        {": fatal error", ERR}, {": error", ERR},
        {": warning", WARN}, {": note", NOTE},
    };
    for (const Rule& r : rules) {
        size_t p = l.find(r.key);
        if (p == std::string::npos) continue;
        size_t e = p + std::string(r.key).size();
        if (e < l.size() && l[e] != ' ' && l[e] != ':' && l[e] != '\r') continue;
        pos = p; wordStart = p + 2; wordEnd = e; sev = r.sev;
        return true;
    }
    return false;
}

// Message text with 'quoted' parts highlighted (like clang does).
static std::string colorize_msg(const std::string& s, const char* base) {
    std::string out = base;
    size_t i = 0;
    while (i < s.size()) {
        if (s[i] == '\'') {
            size_t j = s.find('\'', i + 1);
            if (j != std::string::npos) {
                out += RESET;
                out += C_QUOTE + s.substr(i, j - i + 1) + RESET;
                out += base;
                i = j + 1;
                continue;
            }
        }
        out += s[i++];
    }
    out += RESET;
    return out;
}

// Source snippet line with a "  64 | " gutter; // comments in muted green.
static std::string colorize_snippet(const std::string& l, int lineNo) {
    std::string out;
    if (lineNo > 0) out += std::string(C_GUT) + " " + std::to_string(lineNo) + " | " + RESET;
    size_t c = l.find("//");
    if (c == std::string::npos) {
        out += C_SRC + l + RESET;
    } else {
        out += C_SRC + l.substr(0, c) + RESET;
        out += C_CMT + l.substr(c) + RESET;
    }
    return out;
}

// Caret line with a matching blank gutter: "     | ^~~~".
static std::string colorize_caret(const std::string& l, int lineNo) {
    std::string out;
    if (lineNo > 0)
        out += std::string(C_GUT) + " " + std::string(std::to_string(lineNo).size(), ' ') + " | " + RESET;
    out += C_CARET + l + RESET;
    return out;
}

static std::string colorize_diag(const std::string& l, size_t pos, size_t wordStart,
                                 size_t wordEnd, Sev sev, int& lineNo) {
    const char* sc = sev == ERR ? C_ERR : sev == WARN ? C_WARN : C_NOTE;
    const char* mc = sev == ERR ? C_MSG_E : C_MSG_W;

    std::string head = l.substr(0, pos);                  // path(line,col)
    std::string out;
    size_t par = head.rfind('(');
    lineNo = (par != std::string::npos) ? atoi(head.c_str() + par + 1) : 0;
    if (par != std::string::npos && head.back() == ')') {
        out += C_PATH + head.substr(0, par) + RESET;
        out += C_LOC + head.substr(par) + RESET;
    } else {
        out += C_PATH + head + RESET;
    }
    out += ": ";
    out += sc + l.substr(wordStart, wordEnd - wordStart) + RESET;

    std::string rest = l.substr(wordEnd);                 // " C4018: message"
    size_t colon = rest.find(':');
    bool hasCode = false;
    if (!rest.empty() && rest[0] == ' ' && colon != std::string::npos && colon > 1) {
        std::string code = rest.substr(1, colon - 1);
        hasCode = code.find(' ') == std::string::npos;
        if (hasCode) {
            out += " ";
            out += std::string(sc) + C_CODE + code + RESET;
            out += ":";
            out += colorize_msg(rest.substr(colon + 1), mc);
        }
    }
    if (!hasCode) out += colorize_msg(rest, mc);
    return out;
}

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

// Colors are on unless NO_COLOR is set (any non-empty value) or CL_COLOR=0.
static bool color_enabled() {
    wchar_t buf[8];
    if (GetEnvironmentVariableW(L"NO_COLOR", buf, 8) > 0) return false;
    DWORD n = GetEnvironmentVariableW(L"CL_COLOR", buf, 8);
    if (n > 0 && n < 8 && buf[0] == L'0') return false;
    return true;
}

int main() {
    // Strip our own executable name from the command line.
    std::wstring cmd = GetCommandLineW();
    size_t i;
    if (!cmd.empty() && cmd[0] == L'"') {
        i = cmd.find(L'"', 1);
        i = (i == std::wstring::npos) ? cmd.size() : i + 1;
    } else {
        i = cmd.find_first_of(L" \t");
        if (i == std::wstring::npos) i = cmd.size();
    }
    cmd = cmd.substr(i);

    size_t s = cmd.find_first_not_of(L" \t");
    if (s == std::wstring::npos) {
        fprintf(stderr,
                "cl_color - colorize MSVC diagnostics\n"
                "usage: cl_color <compiler> [args...]\n"
                "  e.g. cl_color cl /nologo /W4 /FC /diagnostics:caret main.cpp\n"
                "env:   NO_COLOR=1 or CL_COLOR=0 disables colors\n");
        return 2;
    }
    cmd = cmd.substr(s);

    SECURITY_ATTRIBUTES sa{sizeof(sa), nullptr, TRUE};
    HANDLE rd = nullptr, wr = nullptr;
    if (!CreatePipe(&rd, &wr, &sa, 0)) {
        fprintf(stderr, "cl_color: CreatePipe failed (%lu)\n", GetLastError());
        return 126;
    }
    SetHandleInformation(rd, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = wr;
    si.hStdError = wr;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    PROCESS_INFORMATION pi{};

    if (!CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, TRUE, 0,
                        nullptr, nullptr, &si, &pi)) {
        fprintf(stderr, "cl_color: CreateProcess failed (%lu)\n", GetLastError());
        return 127;
    }
    CloseHandle(wr);

    std::string out;
    char buf[8192];
    DWORD n;
    while (ReadFile(rd, buf, sizeof(buf), &n, nullptr) && n) out.append(buf, n);

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD code = 0;
    GetExitCodeProcess(pi.hProcess, &code);

    // Colorize line by line.
    std::string result;
    result.reserve(out.size() * 2 + 256);
    bool afterDiag = false;           // next line is likely the source snippet
    bool inWith = false;              // inside a "with [ T=... ]" block
    int curLine = 0;                  // line number of the last diagnostic (for the gutter)
    size_t start = 0;
    const bool color = color_enabled();
    while (color && start < out.size()) {
        size_t nl = out.find('\n', start);
        size_t end = (nl == std::string::npos) ? out.size() : nl;
        bool hasCR = end > start && out[end - 1] == '\r';
        std::string line = out.substr(start, (hasCR ? end - 1 : end) - start);

        size_t pos, ws, we; Sev sev;
        // Never touch /showIncludes lines: Ninja parses them.
        bool includeLine = line.compare(0, 23, "Note: including file:  ") == 0 ||
                           line.compare(0, 20, "Note: including file") == 0;

        // Trimmed copy for "with [ T=... ]" block detection.
        size_t t0 = line.find_first_not_of(" \t");
        std::string trimmed = (t0 == std::string::npos) ? "" : line.substr(t0);
        std::string indent = (t0 == std::string::npos) ? line : line.substr(0, t0);

        // A "with" block only continues over indented lines; anything else ends it.
        if (inWith && (line.empty() || (line[0] != ' ' && line[0] != '\t'))) inWith = false;

        if (!includeLine && trimmed == "with") {
            result += indent + C_WITH + trimmed + RESET;
            inWith = true;
            afterDiag = false;
        } else if (!includeLine && inWith && (trimmed == "[" || trimmed == "]")) {
            result += indent + C_BRK + trimmed + RESET;
            if (trimmed == "]") { inWith = false; afterDiag = true; }  // source snippet follows
        } else if (!includeLine && inWith) {
            // "T=NoAdditionOperator" -> name cyan, '=' gray, value yellow
            size_t eq = trimmed.find('=');
            if (eq != std::string::npos) {
                result += indent + C_TNAME + trimmed.substr(0, eq) + RESET +
                          C_BRK + "=" + RESET +
                          C_TVAL + trimmed.substr(eq + 1) + RESET;
            } else {
                result += indent + C_TVAL + trimmed + RESET;
            }
        } else if (!includeLine && find_severity(line, pos, ws, we, sev)) {
            result += colorize_diag(line, pos, ws, we, sev, curLine);
            afterDiag = true;
        } else if (!includeLine && is_caret_line(line)) {
            result += colorize_caret(line, curLine);
            afterDiag = false;
            curLine = 0;
        } else if (!includeLine && afterDiag && !line.empty()) {
            result += colorize_snippet(line, curLine);   // source snippet under the diagnostic
            afterDiag = false;
        } else {
            result += line;
            afterDiag = false;
        }
        if (hasCR) result += '\r';
        if (nl != std::string::npos) result += '\n';
        start = (nl == std::string::npos) ? out.size() : nl + 1;
    }

    if (!color) result = out;

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    // If we print straight to a console (command-line use), make sure it understands ANSI.
    DWORD oldMode = 0;
    bool modeChanged = false;
    if (color && GetConsoleMode(hOut, &oldMode))
        modeChanged = SetConsoleMode(hOut, oldMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
    size_t off = 0;
    while (off < result.size()) {
        DWORD w = 0;
        if (!WriteFile(hOut, result.data() + off, (DWORD)(result.size() - off), &w, nullptr) || !w) break;
        off += w;
    }
    if (modeChanged) SetConsoleMode(hOut, oldMode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(rd);
    return (int)code;
}
