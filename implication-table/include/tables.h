#pragma once

#include "console_API.h"

#include <vector>
#include <string>

#define ST_PS_MAX_LENGTH 6
#define ST_NS_MAX_LENGTH 6
#define ST_OUTPUT_MAX_LENGTH 6

namespace PopularColors{
    inline constexpr WORD DEFAULT_COLOR             = createColor(WHITE, BLACK);
    inline constexpr WORD INVERTED_DEFAULT_COLOR    = createColor(BLACK, WHITE); 
    inline constexpr WORD GREEN_BUTTON              = createColor(BLACK, GREEN);
    inline constexpr WORD HIGHLIGHTED_GREEN_BUTTON  = createColor(BLACK, GREEN | BRIGHT);
    inline constexpr WORD GREEN_TEXT                = createColor(GREEN, BLACK);
    inline constexpr WORD RED_BUTTON                = createColor(BLACK, RED);
    inline constexpr WORD HIGHLIGHTED_RED_BUTTON    = createColor(BLACK, RED | BRIGHT);
    inline constexpr WORD RED_TEXT                  = createColor(RED | BRIGHT, BLACK);
    inline constexpr WORD BLUE_BUTTON               = createColor(BLACK, BLUE);
    inline constexpr WORD HIGHLIGHTED_BLUE_BUTTON   = createColor(BLACK, BLUE | BRIGHT);
    inline constexpr WORD BLUE_TEXT                 = createColor(BLUE | BRIGHT, BLACK);
    inline constexpr WORD YELLOW_BUTTON             = createColor(BLACK, YELLOW);
    inline constexpr WORD HIGHLIGHTED_YELLOW_BUTTON = createColor(BLACK, YELLOW | BRIGHT);
    inline constexpr WORD YELLOW_TEXT               = createColor(YELLOW, BLACK);

    inline const std::vector<WORD> GREEN_BUTTON_PRESET_A = {DEFAULT_COLOR, INVERTED_DEFAULT_COLOR, GREEN_BUTTON, HIGHLIGHTED_GREEN_BUTTON};
    inline const std::vector<WORD> GREEN_BUTTON_PRESET_B = {GREEN_TEXT, GREEN_BUTTON};
    
    inline const std::vector<WORD> RED_BUTTON_PRESET_A = {DEFAULT_COLOR, INVERTED_DEFAULT_COLOR, RED_BUTTON, HIGHLIGHTED_RED_BUTTON};
    inline const std::vector<WORD> RED_BUTTON_PRESET_B = {RED_TEXT, HIGHLIGHTED_RED_BUTTON};
    
    inline const std::vector<WORD> BLUE_BUTTON_PRESET_A = {DEFAULT_COLOR, INVERTED_DEFAULT_COLOR, BLUE_BUTTON, HIGHLIGHTED_BLUE_BUTTON};
    inline const std::vector<WORD> BLUE_BUTTON_PRESET_B = {BLUE_TEXT, HIGHLIGHTED_BLUE_BUTTON};

    inline const std::vector<WORD> YELLOW_BUTTON_PRESET_A = {DEFAULT_COLOR, INVERTED_DEFAULT_COLOR, YELLOW_BUTTON, HIGHLIGHTED_YELLOW_BUTTON};
    inline const std::vector<WORD> YELLOW_BUTTON_PRESET_B = {YELLOW_TEXT, YELLOW_BUTTON};
}

using namespace PopularColors;

class Button{

public:
    Sprite sprite;
    int fullState = 0;

    Button(const std::string& content="", const std::vector<WORD>& colors = {DEFAULT_COLOR, INVERTED_DEFAULT_COLOR});
    
    bool activated(int st);
    
    void enable(int st);
    void disable(int st);
    void setText(int fst, const string& text, int fg = WHITE, int bg = BLACK);
    void setCLR(int fst, WORD clr);
};

enum StateTableType{
    MEALY,
    MOORE
};

class StateTable{
    int padding = 2, horizLineWidth = 1, vertiLineWidth = 1;
    int widthA = 7 + 2*padding, widthB = 9 + 2*padding, widthC = 6 + 2*padding;
    std::vector<int> widthNS, widthO;
    
public:
    int H1 = 0, H2 = 0, H3 = 0;
    int V1 = 0, V2 = 0, V3 = 0, V4 = 0;

    int n_states;
    int n_inputs;
    StateTableType myType;
    std::vector<std::string> states;
    std::vector<std::vector<std::string> > nextStates;
    std::vector<std::vector<std::string> > stateOutput;

    map<string, int>ID;
    enum CursorLocation{
        onPS,
        onNS,
        onOutput,
        Outside
    };
    Sprite sprite;
    CursorLocation location = onPS;
    int c_row = 0, c_column = 0;
    
    StateTable();
    void minimize(std::vector<int> equivalent_classes);
    int validation();
    void updateData();
    void updateSprite();
    void display(int startX = -1, int startY = -1);

    CursorLocation processInput();
};

class ImplicationTable{
    int padding = 1, horizLineWidth = 1, vertiLineWidth = 1;
    std::vector<int> boxWidth, boxHeight;
    struct Box{
        std::vector<std::array<int,3>> imps;
        int n_imps = 0, boxState = 1, width = 5, height = 2;
        Sprite sprite;
    };
    
public:
    int n_states, page = 1, lastPage = 1, shift = 1;
    std::vector<std::string> states;
    std::map<std::string, int> id;
    std::vector<std::vector<std::vector<Box>>> boxes;
    Sprite sprite;

    void create(int i, int j, const std::vector<string>& st1, const std::vector<string>& st2);
    int boxState(int i, int j);
    bool recheck(int i, int j);
    void recalcState(int i, int j);
    
    ImplicationTable(const StateTable& table);
    
    vector<int> getEquivalentClasses();

    void render(int i, int j);
    void updateSprite();
};

void processInputField(string& text, int maxL);