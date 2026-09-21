#pragma once

#include <chrono>
#include <thread>
#include <map>
#include <vector>
#include <string>
#include <algorithm>
#include <functional>
#include <fstream>
#include <windows.h>
#include <mmsystem.h>

#define sz(x) (int)x.size()
#define ERASE(v, i) v.erase(v.begin() + i)
#define BOX_CHAR L'#'

using std::string;
using std::vector;
using std::max;
using std::min;
using std::fill;
using std::sort;
using std::swap;
using std::map;
using std::pair;
using std::array;
using std::to_string;
using std::function;
using std::this_thread::sleep_for;
using std::chrono::milliseconds;
using std::chrono::steady_clock;
using std::chrono::duration_cast;
using std::ofstream;
using std::ios;
using std::stoi;
using std::reverse;

string binaryFormat(uint32_t value, int n);
string base26(int x);
void log(const std::string& msg);
void clearLog();

inline constexpr wchar_t boxCharString[] = L"###│#┘┐┤#└┌├─┴┬┼";
// box characters: ─ │ ┌ ┐ └ ┘ ┬ ┴ ┼ ├ ┤
constexpr bool isABoxCharacter(wchar_t c) {
    if (c == L'#') return 1;
    return 0;
}


constexpr WORD createColor(int fg, int bg) {
    return fg | (bg << 4);
}

constexpr int getFG(const WORD& color) {
    return color & 0x0F;
}

constexpr int getBG(const WORD& color) {
    return (color >> 4) & 0x0F;
}

constexpr void setFG(WORD& color, int fg) {
    color = (color & 0xF0) | fg;
}

constexpr void setBG(WORD& color, int bg) {
    color = (color & 0x0F) | (bg << 4);
}