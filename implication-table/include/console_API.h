#pragma once

#include "common.h"

#define TARGET_FPS 60

enum ConsoleColor {
    BLACK     = 0,
    BLUE      = FOREGROUND_BLUE,
    GREEN     = FOREGROUND_GREEN,
    RED       = FOREGROUND_RED,
    YELLOW    = FOREGROUND_RED   | FOREGROUND_GREEN,
    CYAN      = FOREGROUND_GREEN | FOREGROUND_BLUE,
    MAGENTA   = FOREGROUND_RED   | FOREGROUND_BLUE,
    WHITE     = FOREGROUND_RED   | FOREGROUND_GREEN | FOREGROUND_BLUE,
    BRIGHT    = FOREGROUND_INTENSITY
};

class InputManager {
    bool currentBuffer[256] = {};
    int  countBuffer[256] = {};
    const std::vector<int> strokeKeys = {
        // Letters
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
        'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',

        // Numbers
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',

        // Special Symbols
        VK_OEM_3,      // ` ~
        VK_OEM_MINUS,  // - _
        VK_OEM_PLUS,   // = +
        VK_OEM_4,      // [ {
        VK_OEM_6,      // ] }
        VK_OEM_5,      // \ |
        VK_OEM_1,      // ; :
        VK_OEM_7,      // ' "
        VK_OEM_COMMA,  // , 
        VK_OEM_PERIOD, // . >
        VK_OEM_2,      // / ?

        //Numpad
        VK_NUMPAD0,
        VK_NUMPAD1, VK_NUMPAD2, VK_NUMPAD3,
        VK_NUMPAD4, VK_NUMPAD5, VK_NUMPAD6,
        VK_NUMPAD7, VK_NUMPAD8, VK_NUMPAD9,
        VK_MULTIPLY,  // *
        VK_ADD,       // +
        VK_SUBTRACT,  // -
        VK_DECIMAL,   // .
        VK_DIVIDE,    // /

        // Arrows
        VK_LEFT, VK_UP, VK_RIGHT, VK_DOWN,

        // Special
        VK_SPACE, VK_RETURN, VK_ESCAPE, VK_BACK
    };

public:
    void clear();
    void update();
    bool isKeyDown(int vKey)const;
    bool isPressed(int vKey)const;
    bool isReleased(int vKey)const;
    int getKeyStroke(int repeatThreshold = TARGET_FPS * 0.6)const;
};

class TEX {
public:
    int width, height;
    std::vector<CHAR_INFO> cells;
    
    TEX(int w = 1, int h = 1, bool tr = 1);

    void setCell(int x, int y, wchar_t ch, int fg = WHITE, int bg = BLACK);
    void setText(int x, int y, const std::string& text, int fg = WHITE, int bg = BLACK);
    void setLine(int x, int y, int l, const wchar_t c, bool verti = 0, int fg = WHITE, int bg = BLACK);    
    void setBox(int x, int y, int w, int h, int horizLineWidth = 1, int vertiLineWidth = 1, int fg = WHITE, int bg = BLACK);
    void setTex(int x, int y, const TEX& tex);
    void setTextureBG(int bg);
    void setTextureFG(int fg);
    void setTextureCLR(WORD clr);
};

class Sprite {
public:
    int width, height, startX, startY;
    std::map<std::string, TEX> costumes;
    std::vector<CHAR_INFO> cells;
    std::string currentCostume;

    Sprite(const std::string& name = "Default", TEX Tex = TEX());
    void resize(int w, int h);

    void setCell(int x, int y, wchar_t ch, int fg = WHITE, int bg = BLACK, const std::string&name = "");
    void setText(int x, int y, const std::string& text, int fg = WHITE, int bg = BLACK, const std::string&name = "");
    void setLine(int x, int y, int l, const wchar_t c, bool verti = 0, int fg = WHITE, int bg = BLACK, const std::string&name = "");
    void setBox(int x, int y, int w, int h, int horizLineWidth = 1, int vertiLineWidth = 1, int fg = WHITE, int bg = BLACK, const std::string&name = "");
    void setTex(int x, int y, const TEX& tex, const std::string&name = "");
    void setSprite(Sprite& sprite, int startX = -1, int startY = -1, const std::string&name = "");
    void setTextureBG(int bg, const std::string&name = "");
    void setTextureFG(int fg, const std::string&name = "");
    void setTextureCLR(WORD clr, const std::string&name = "");

    void reposition(int x, int y);
    
    void addCostume(const std::string& name, const TEX& newTex = TEX());
    void setCostume(const std::string& name);
    void render();
    void changeCurrentCostumeName(const std::string& newName);
    TEX& getCurrentCostume();
};


class OutputManager {
    std::vector<CHAR_INFO> buffer;

public:
    int width, height;
    int f_lx, f_rx, f_ly, f_ry;
    int viewportX = 0, viewportY = 0;

    OutputManager(int w = 1, int h = 1);

    void clear();
    void draw(int x, int y, wchar_t ch, int fg = WHITE, int bg = BLACK);
    void drawSprite(Sprite& sprite, int startX = -1, int startY = -1);
    void drawText(int x, int y, const std::string& text, int fg = WHITE, int bg = BLACK);
    void drawLine(int x, int y, int l, const wchar_t c, bool verti = 0, int fg = WHITE, int bg = BLACK);
    void drawBox(int x, int y, int w, int h, int horizLineWidth = 1, int vertiLineWidth = 1, int fg = WHITE, int bg = BLACK);
    void focus(int lx, int rx, int ly, int ry);
    void focus(const Sprite& sprite);
    void renderBoxes();
    void display();
};

void initTerminal();
void shutdownTerminal();
void setFont(int cellHeight);
void getWindowSize(int& w, int& h);

void tick();

InputManager& Input();
OutputManager& Output();

char getTypedCharacter();

void displayCursor(int x, int y, wchar_t textureA = L'\x2588' /*█*/, wchar_t textureB = L' ', int X_ms = 530, int Y_ms = 500);