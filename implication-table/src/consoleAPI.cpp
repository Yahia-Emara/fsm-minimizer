#include "console_API.h"
#include "common.h"

#include <windows.h>
#include <mmsystem.h>


#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

// ============================================================
//  INTERNAL STATE
// ============================================================
static HANDLE              s_hOut          = INVALID_HANDLE_VALUE;
static HANDLE              s_hIn           = INVALID_HANDLE_VALUE;
static HANDLE              s_hOrigOut      = INVALID_HANDLE_VALUE;
static DWORD               s_origOutMode   = 0;
static DWORD               s_origInMode    = 0;
static CONSOLE_CURSOR_INFO s_origCursor    = {};
static WORD                s_origAttrib    = 7;

static InputManager s_Input;
static OutputManager s_Output;

// ============================================================
//  PUBLIC API
// ============================================================
void initTerminal() {
    // Save original state so shutdownTerminal() can fully restore it
    s_hOrigOut = GetStdHandle(STD_OUTPUT_HANDLE);
    s_hIn  = GetStdHandle(STD_INPUT_HANDLE);

    GetConsoleMode(s_hOrigOut, &s_origOutMode);
    GetConsoleMode(s_hIn, &s_origInMode);
    GetConsoleCursorInfo(s_hOrigOut, &s_origCursor);

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(s_hOrigOut, &csbi))
        s_origAttrib = csbi.wAttributes;
    
    // Create a new, private screen buffer
    s_hOut = CreateConsoleScreenBuffer(
        GENERIC_READ | GENERIC_WRITE, 
        FILE_SHARE_READ | FILE_SHARE_WRITE, 
        NULL, 
        CONSOLE_TEXTMODE_BUFFER, 
        NULL
    );
    
    // Make our private buffer the one the user actually sees
    SetConsoleActiveScreenBuffer(s_hOut);

    // Output mode:
    //   ENABLE_PROCESSED_OUTPUT            - interprets \b \t etc.
    //   ENABLE_VIRTUAL_TERMINAL_PROCESSING - ANSI/VT sequences
    //   ENABLE_WRAP_AT_EOL_OUTPUT absent   - stops bottom-right scroll bug
    DWORD outMode = ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(s_hOut, outMode);

    // Input mode:
    //   ENABLE_PROCESSED_INPUT - Ctrl+C works
    //   ENABLE_EXTENDED_FLAGS  - required to turn off Quick Edit
    //   Quick Edit absent      - no click-to-pause freezing the loop
    //   Echo/Line input absent - we use GetAsyncKeyState for input
    DWORD inMode = ENABLE_PROCESSED_INPUT | ENABLE_EXTENDED_FLAGS;
    SetConsoleMode(s_hIn, inMode);

    // UTF-8 so Unicode characters render correctly
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

    // Hide the system text cursor — we draw our own via the CURSOR sprite
    CONSOLE_CURSOR_INFO ci = {1, FALSE};
    SetConsoleCursorInfo(s_hOut, &ci);

    // Park the real cursor at (0,0) so it's out of the way
    COORD origin = {0, 0};
    SetConsoleCursorPosition(s_hOut, origin);

    // 1ms timer resolution for accurate frame pacing via Sleep()
    timeBeginPeriod(1);

    // Initialize the logical output buffer.
    // 200x60 is a safe default — change to suit your layout.
    // This does NOT need to match the terminal window size.
    setFont(16);
    s_Output = OutputManager(500, 500);
}

void setFont(int cellHeight) {
    CONSOLE_FONT_INFOEX fi = {};
    fi.cbSize       = sizeof(fi);
    fi.dwFontSize.X = 0;
    fi.dwFontSize.Y = (SHORT)cellHeight;
    fi.FontFamily   = FF_DONTCARE;
    fi.FontWeight   = FW_NORMAL;
    wcscpy_s(fi.FaceName, L"Consolas");
    SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &fi);
}

void shutdownTerminal() {
    if (s_hOut == INVALID_HANDLE_VALUE) return;

    timeEndPeriod(1);

    SetConsoleActiveScreenBuffer(s_hOrigOut);

    SetConsoleMode(s_hOrigOut, s_origOutMode);
    SetConsoleMode(s_hIn, s_origInMode);
    SetConsoleCursorInfo(s_hOrigOut, &s_origCursor);
    SetConsoleTextAttribute(s_hOrigOut, s_origAttrib);

    CloseHandle(s_hOut);

    s_hOut = INVALID_HANDLE_VALUE;
}

void getWindowSize(int& w, int& h) {
    if (s_hOut == INVALID_HANDLE_VALUE) return;

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(s_hOut, &csbi)) return;

    w = csbi.dwSize.X; 
    h = csbi.dwSize.Y;
}

void InputManager::clear() {
    for(int i = 0; i < 256; i++) currentBuffer[i] = countBuffer[i] = 0;
}

void InputManager::update() {
    for (int i = 0; i < 256; i++) {
        bool held = (GetAsyncKeyState(i) & 0x8000) != 0;
        if (held == currentBuffer[i]) countBuffer[i]++;
        else countBuffer[i] = 1;
        currentBuffer[i] = held;
    }
}

bool InputManager::isKeyDown(int vKey)const{
    return currentBuffer[vKey];
}

bool InputManager::isPressed(int vKey)const{
    return currentBuffer[vKey] && countBuffer[vKey] == 1;
}

bool InputManager::isReleased(int vKey)const{
    return !currentBuffer[vKey] && countBuffer[vKey] == 1;
}

int InputManager::getKeyStroke(int repeatThreshold)const{
    int minCount = INT_MAX, strokeKey = -1;

    for (int vk : strokeKeys) {
        if (!currentBuffer[vk]) continue;
        int count = countBuffer[vk];
        if (count < minCount) {
            minCount = count;
            strokeKey = vk;
        }
    }
    if (minCount > 1 && minCount < repeatThreshold) strokeKey = -1; 

    return strokeKey;
}

TEX::TEX(int w, int h, bool tr) : width(w), height(h) {
    cells.resize(w * h);
    for (auto& c : cells) {
        c.Char.UnicodeChar = (tr ? L'\0' : L' ');
        c.Attributes = 7;
    }
}

void TEX::setCell(int x, int y, wchar_t ch, int fg, int bg) {
    if (x < 0 || x >= width || y < 0 || y >= height) return;
    int i = y*width + x;
    cells[i].Char.UnicodeChar = ch;
    cells[i].Attributes  = createColor(fg, bg);
}

void TEX::setText(int x, int y, const string& text, int fg, int bg) {
    for (int i = 0; i < sz(text); i++) {
        int tx = x + i;
        if (tx < 0 || tx >= width || y < 0 || y >= height) continue;
        int idx = y*width + tx;
        cells[idx].Char.UnicodeChar = (wchar_t)(unsigned char)text[i];
        cells[idx].Attributes  = createColor(fg, bg);
    }
}

void TEX::setLine(int x, int y, int l, wchar_t c, bool verti, int fg, int bg) {
    for (int i = 0; i < l; i++) {
        int tx = x + i*(verti ^ 1);
        int ty = y + i*verti;
        if (tx < 0 || tx >= width || ty < 0 || ty >= height) continue;
        
        int idx = ty*width + tx;
        cells[idx].Char.UnicodeChar = c;
        cells[idx].Attributes  = createColor(fg, bg);
    }
}

void TEX::setBox(int x, int y, int w, int h, int horizLineWidth, int vertiLineWidth, int fg, int bg) {
    w += 2*(vertiLineWidth), h += 2*(horizLineWidth);
    for(int t = 1; t <= horizLineWidth; t++) {
        setLine(x - vertiLineWidth, y - t, w, BOX_CHAR, 0);
        setLine(x - vertiLineWidth, y + h - vertiLineWidth - t, w, BOX_CHAR, 0);
    }
    for(int t = 1; t <= vertiLineWidth; t++) {
        setLine(x - t, y - horizLineWidth, h, BOX_CHAR, 1);
        setLine(x + w - horizLineWidth - t, y - horizLineWidth, h, BOX_CHAR, 1);
    }
}

void TEX::setTex(int x, int y, const TEX& tex){
    for (int j = 0; j < tex.height; j++) {
        for (int i = 0; i < tex.width; i++) {
            int tx = x + i, ty = y + j;
            if (tx < 0 || tx >= width || ty < 0 || ty >= height) continue;
            const CHAR_INFO& src = tex.cells[j*tex.width + i];
            if (src.Char.UnicodeChar == L'\0') continue;
            cells[ty*width + tx] = src;
        }
    }
}

void TEX::setTextureBG(int bg) {
    for (int y = 0; y < height; y++) {
        for(int x = 0; x < width; x++) {
            int idx = y*width + x;
            setBG(cells[idx].Attributes, bg);
        }
    }
}

void TEX::setTextureFG(int fg) {
    for (int y = 0; y < height; y++) {
        for(int x = 0; x < width; x++) {
            int idx = y*width + x;
            setFG(cells[idx].Attributes, fg);
        }
    }
}

void TEX::setTextureCLR(WORD clr) {
    for (int y = 0; y < height; y++) {
        for(int x = 0; x < width; x++) {
            int idx = y*width + x;
            cells[idx].Attributes = clr;
        }
    }
}

Sprite::Sprite(const string& name, TEX Tex) {
    startX = startY = 0;
    
    currentCostume = name;
    costumes.clear();
    costumes[currentCostume] = Tex;

    width = costumes[currentCostume].width;
    height = costumes[currentCostume].height;
    cells = costumes[currentCostume].cells;
    
}

void Sprite::resize(int w, int h){
    costumes[currentCostume] = TEX(w, h);
}

void Sprite::setCell(int x, int y, wchar_t ch, int fg, int bg, const string&name) {
    costumes[name == "" ? currentCostume : name].setCell(x, y, ch, fg, bg);
}

void Sprite::setText(int x, int y, const string& text, int fg, int bg, const string&name) {
    costumes[name == "" ? currentCostume : name].setText(x, y, text, fg, bg);
}

void Sprite::setLine(int x, int y, int l, wchar_t c, bool verti, int fg, int bg, const string&name) {
    costumes[name == "" ? currentCostume : name].setLine(x, y, l, c, verti, fg, bg);
}

void Sprite::setBox(int x, int y, int w, int h, int horizLineWidth, int vertiLineWidth, int fg, int bg, const string& name) {
    costumes[name == "" ? currentCostume : name].setBox(x, y, w, h, horizLineWidth, vertiLineWidth, fg, bg);
}

void Sprite::setTex(int x, int y, const TEX& tex, const string& name) {
    costumes[name == "" ? currentCostume : name].setTex(x, y, tex);
}

void Sprite::setSprite(Sprite& sprite, int startX, int startY, const string&name) {
    sprite.render();
    if(startX == -1) startX = sprite.startX;
    if(startY == -1) startY = sprite.startY;
    costumes[name == "" ? currentCostume : name].setTex(startX, startY, sprite.getCurrentCostume());
}

void Sprite::setTextureBG(int bg, const string&name) {
    costumes[name == "" ? currentCostume : name].setTextureBG(bg);
}

void Sprite::setTextureFG(int fg, const string&name) {
    costumes[name == "" ? currentCostume : name].setTextureFG(fg);
}

void Sprite::setTextureCLR(WORD clr, const string&name) {
    costumes[name == "" ? currentCostume : name].setTextureCLR(clr);
}

void Sprite::reposition(int x, int y) {
    if (x != -1) startX = x;
    if (y != -1) startY = y;   
}

void Sprite::addCostume(const string& name, const TEX& newTex) {
    costumes[name] = newTex;
}

void Sprite::setCostume(const string& name) {
    currentCostume = name;
}

void Sprite::render() {
    width = costumes[currentCostume].width;
    height = costumes[currentCostume].height;
    cells = costumes[currentCostume].cells;
}

void Sprite::changeCurrentCostumeName(const string& newName) {
    if(newName == currentCostume)return;

    costumes[newName] = costumes[currentCostume];
    costumes.erase(costumes.find(currentCostume));
}

TEX& Sprite::getCurrentCostume() {
    return costumes[currentCostume];
}

OutputManager::OutputManager(int w, int h) : width(w), height(h) {
    viewportX = 0, viewportY = 0;
    buffer.assign(w*h, {{L' '}, 7});
}

void OutputManager::clear() {
    CHAR_INFO blank;
    blank.Char.UnicodeChar = L' ';
    blank.Attributes = 7;
    std::fill(buffer.begin(), buffer.end(), blank);
    f_lx = f_rx = f_ly = f_ry = 0;
}

void OutputManager::draw(int x, int y, wchar_t ch, int fg, int bg) {
    if (x < 0 || x >= width || y < 0 || y >= height) return;
    int i = y*width + x;
    buffer[i].Char.UnicodeChar = ch;
    buffer[i].Attributes = (WORD)(fg | (bg << 4));
}

void OutputManager::drawSprite(Sprite& sprite, int startX, int startY) {
    sprite.render();

    if (startX == -1) startX = sprite.startX;
    if (startY == -1) startY = sprite.startY;

    sprite.startX = startX;
    sprite.startY = startY;

    for (int y = 0; y < sprite.height; y++) {
        for (int x = 0; x < sprite.width; x++) {
            int tx = startX + x, ty = startY + y;
            if (tx < 0 || tx >= width || ty < 0 || ty >= height) continue;
            const CHAR_INFO& src = sprite.cells[y*sprite.width + x];
            if (src.Char.UnicodeChar == L'\0') continue;
            buffer[ty*width + tx] = src;
        }
    }
}

void OutputManager::drawText(int x, int y, const string& text, int fg, int bg) {
    for (int i = 0; i < sz(text); i++) {
        int tx = x + i;
        if (tx < 0 || tx >= width || y < 0 || y >= height) continue;
        
        int idx = y*width + tx;
        buffer[idx].Char.UnicodeChar = (wchar_t)(unsigned char)text[i];
        buffer[idx].Attributes  = createColor(fg, bg);
    }
}

void OutputManager::drawLine(int x, int y, int l, wchar_t c, bool verti, int fg, int bg) {
    for (int i = 0; i < l; i++) {
        int tx = x + i*(verti ^ 1);
        int ty = y + i*verti;
        if (tx < 0 || tx >= width || ty < 0 || ty >= height) continue;
        
        int idx = ty*width + tx;
        buffer[idx].Char.UnicodeChar = c;
        buffer[idx].Attributes  = createColor(fg, bg);
    }
}

void OutputManager::drawBox(int x, int y, int w, int h, int horizLineWidth, int vertiLineWidth, int fg, int bg) {
    w += 2*(horizLineWidth), h += 2*(vertiLineWidth);
    for(int t = 1; t <= horizLineWidth; t++) {
        drawLine(x - vertiLineWidth, y - t, w, BOX_CHAR, 0);
        drawLine(x - vertiLineWidth, y + h - vertiLineWidth - t, w, BOX_CHAR, 0);
    }
    for(int t = 1; t <= vertiLineWidth; t++) {
        drawLine(x - t, y - horizLineWidth, h, BOX_CHAR, 1);
        drawLine(x + w - horizLineWidth - t, y - horizLineWidth, h, BOX_CHAR, 1);
    }
}

void OutputManager::focus(int lx, int rx, int ly, int ry) {
    f_lx = lx, f_rx = rx, f_ly = ly, f_ry = ry;
    int termW, termH; 
    getWindowSize(termW, termH);
    
    if(viewportX + termW - 1 < f_rx)viewportX = f_rx - termW + 1;
    if(viewportX > f_lx)viewportX = f_lx;
    if(viewportY + termH - 1 < f_ry)viewportY = f_ry - termH + 1;
    if(viewportY > f_ly)viewportY = f_ly;
}

void OutputManager::focus(const Sprite& sprite) {
    f_lx = sprite.startX, f_ly = sprite.startY;
    f_rx = f_lx + sprite.width, f_ry = f_ly + sprite.height;
    int termW, termH; 
    getWindowSize(termW, termH);
    
    if(viewportX + termW - 1 < f_rx)viewportX = f_rx - termW + 1;
    if(viewportX > f_lx)viewportX = f_lx;
    if(viewportY + termH - 1 < f_ry)viewportY = f_ry - termH + 1;
    if(viewportY > f_ly)viewportY = f_ly;
}

void OutputManager::renderBoxes() {
    auto cpy = buffer;
    auto getBoxNeighborsMask = [&](int x, int y) -> int {
        int mask = 0, idx = y*width + x;
        if (y > 0 && isABoxCharacter(cpy[idx - width].Char.UnicodeChar)) mask |= 1;
        if (y + 1 < height && isABoxCharacter(cpy[idx + width].Char.UnicodeChar)) mask |= 2;
        if (x > 0 && isABoxCharacter(cpy[idx - 1].Char.UnicodeChar)) mask |= 4;
        if (x + 1 < width && isABoxCharacter(cpy[idx + 1].Char.UnicodeChar)) mask |= 8;
        return mask;
    };


    for(int x = 0; x < width; x++) {
        for(int y = 0; y < height; y++) {
            int idx = y*width + x;
            if (isABoxCharacter(buffer[idx].Char.UnicodeChar))
                buffer[idx].Char.UnicodeChar = boxCharString[getBoxNeighborsMask(x, y)];
        }
    }
}

void OutputManager::display() {
    if (s_hOut == INVALID_HANDLE_VALUE) return;
    
    int termW, termH; 
    getWindowSize(termW, termH);
    
    if(viewportX + termW - 1 < f_rx)viewportX = f_rx - termW + 1;
    if(viewportX > f_lx)viewportX = f_lx;
    if(viewportY + termH - 1 < f_ry)viewportY = f_ry - termH + 1;
    if(viewportY > f_ly)viewportY = f_ly;
    
    int blitW = min(termW,  width - viewportX);
    int blitH = min(termH, height - viewportY);
    if (blitW <= 0 || blitH <= 0) return;

    COORD srcSize = {(SHORT)width, (SHORT)height};
    COORD srcOrigin = {(SHORT)viewportX, (SHORT)viewportY};
    SMALL_RECT dstRect = {
        (SHORT)0,
        (SHORT)0,
        (SHORT)(blitW - 1),
        (SHORT)(blitH - 1)
    };
    
    renderBoxes();
    WriteConsoleOutputW(s_hOut, buffer.data(), srcSize, srcOrigin, &dstRect);
}

const milliseconds frameDuration(1000/TARGET_FPS); //Target 60 FPS, 1000ms/60 ~= 16
int frameCount = 0;
double currentFPS = 0.0;
auto lastUpdate = steady_clock::now();

void tick() {
    auto frameStart = steady_clock::now();
    
    s_Output.display();
    s_Input.update();
    s_Output.clear();

    frameCount++;
    auto now = steady_clock::now();
    auto durationSinceUpdate = duration_cast<milliseconds>(now - lastUpdate);

    // Update the FPS counter once every second (1000ms)
    if (durationSinceUpdate >= milliseconds(1000)) {
        currentFPS = frameCount / (durationSinceUpdate.count() / 1000.0);
        frameCount = 0;
        lastUpdate = now;
        
        // Optional: Print FPS to the title bar so it doesn't mess up your UI
        string title = "FPS: " + to_string((int)currentFPS);
        SetConsoleTitleA(title.c_str());
    }

    // Precise Sleeping
    auto frameEnd = steady_clock::now();
    auto elapsed = duration_cast<milliseconds>(frameEnd - frameStart);

    if (elapsed < frameDuration) {
        sleep_for(frameDuration - elapsed);
    }
}

InputManager& Input(){return s_Input;}
OutputManager& Output(){return s_Output;}

char getTypedCharacter() {
    int key = Input().getKeyStroke();
    if(key >= 'A' && key <= 'Z'){
        if (!Input().isKeyDown(VK_SHIFT)) return key + 'a' - 'A';
        return key;
    }
    if(key >= '0' && key <= '9') return key;
    if(key >= VK_NUMPAD0 && key <= VK_NUMPAD9) return '0'+key - VK_NUMPAD0;
    if(key == VK_BACK) return '\b';
    return 0;
}

void displayCursor(int x, int y, wchar_t textureA, wchar_t textureB, int X_ms, int Y_ms) {
    auto     now    = steady_clock::now().time_since_epoch();
    uint64_t ms     = (uint64_t)duration_cast<milliseconds>(now).count();
    uint64_t period = (uint64_t)(X_ms + Y_ms);
    wchar_t  tex    = (ms % period < (uint64_t)X_ms) ? textureA : textureB;
    Output().focus(x, x, y, y);
    Output().draw(x, y, textureA, BRIGHT | WHITE, BLACK);
}