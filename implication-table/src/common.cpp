#include "common.h"

string binaryFormat(uint32_t value, int n) {
    if (n == 0) return "";
    string s(n, '0');
    for (int i = n - 1; i >= 0; --i) {
        s[i] = (value & 1) ? '1' : '0';
        value >>= 1;
    }
    return s;
}

string base26(int x){
    if(x == 1) return "A";
    if(x < 1) return "";
    x--;
    string s = "";
    while(x > 0) s += char('A' + x%26), x/=26;
    reverse(s.begin(), s.end());
    return s;

}

void log(const string& msg) {
    ofstream file("debug.log", ios::app);
    file << msg << "\n";
}

void clearLog() {
    ofstream file("debug.log", ios::trunc);
}
