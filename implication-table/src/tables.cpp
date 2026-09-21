#include "common.h"
#include "console_API.h"
#include "tables.h"

#define BUTTON_N_STATES 3

Button::Button(const string& content, const vector<WORD>& colors) {
    int length = sz(content);
    fullState = 0;
    sprite = Sprite("0", TEX(length, 1));
    for(int i = 1; i < (1 << BUTTON_N_STATES); i++) sprite.addCostume(to_string(i), TEX(length, 1));
    for(int i = 0; i < (1 << BUTTON_N_STATES); i++) sprite.setText(0, 0, content, WHITE, BLACK, to_string(i));
    for(int i = 0; i < sz(colors); i++) sprite.setText(0, 0, content, getFG(colors[i]), getBG(colors[i]), to_string(i));
}

bool Button::activated(int st) {
    return (fullState >> st) & 1;
}

void Button::enable(int st) {
    fullState |= (1 << st);
    sprite.setCostume(to_string(fullState));
}

void Button::disable(int st) {
    fullState &= ~(1 << st);
    sprite.setCostume(to_string(fullState));
}

void Button::setText(int fst, const string& text, int fg, int bg) {
    string name = to_string(fst);
    sprite.costumes[name] = TEX(sz(text), 1);
    sprite.setText(0, 0, text, fg, bg, name);
}

void Button::setCLR(int fst, WORD clr) {
    sprite.setTextureCLR(clr, to_string(fst));
}

StateTable::StateTable() {
    n_inputs = 1;
    n_states = 12;
    myType = MEALY;
    states = {"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l"};

    nextStates = {{"b", "c"},
                  {"d", "e"},
                  {"f", "g"},
                  {"h", "i"},
                  {"j", "k"},
                  {"d", "l"},
                  {"j", "l"},
                  {"h", "a"},
                  {"j", "a"},
                  {"d", "a"},
                  {"b", "a"},
                  {"b", "a"}};

    stateOutput = {{"0", "0"},
                   {"0", "0"},
                   {"0", "0"},
                   {"0", "0"},
                   {"0", "0"},
                   {"0", "0"},
                   {"0", "0"},
                   {"0", "0"},
                   {"1", "0"},
                   {"0", "0"},
                   {"0", "0"},
                   {"0", "0"}};
}

void StateTable::minimize(vector<int> equivalent_classes) {
    vector<vector<string>> nextStatesCPY = nextStates;
    vector<vector<string>> outputCPY = stateOutput;
    n_states = 0;
    
    for(int i = 0; i < sz(equivalent_classes); i++) {
        bool flag = 0;
        for(int j = 0; j < i && !flag; j++){
            if(equivalent_classes[i] == equivalent_classes[j]) flag = 1;
        }
        if(flag) continue;
        states[n_states] = base26(equivalent_classes[i]);
        for(int j = 0; j < (1 << n_inputs); j++){
            nextStates[n_states][j] = base26(equivalent_classes[ID[nextStatesCPY[i][j]]]);
        }
        stateOutput[n_states] = outputCPY[i];
        n_states++;
    }
    states.resize(n_states);
    nextStates.resize(n_states);
    stateOutput.resize(n_states);
    
}

int StateTable::validation() {
    updateData();

    for(auto& i : states) {
        if(i == "") return 1;
    }
    for(auto& v : nextStates) {
        for(auto& j : v){
            if(j == "") return 1;
        }
    }
    for(auto& v : stateOutput) {
        for(auto& j : v){
            if(j == "") return 1;
        }
    }
    
    if(sz(ID) != n_states) return 2;

    for(auto& v : nextStates) {
        for(auto& j : v){
            if(!ID.count(j)) return 3;
        }
    }
    
    return 0;
}

void StateTable::updateData() {
    ID.clear();
    for(int i = 0; i < n_states; i++) ID[states[i]] = i;

    widthA = 7 + 2*padding, widthB = 9 + 2*padding, widthC = 6 + 2*padding;

    widthNS.assign((1 << n_inputs), n_inputs);
    
    if (myType == MEALY) widthO.assign((1 << n_inputs), n_inputs);
    else widthO.assign(1, widthC - 2*padding);
    
    for (int i = 0; i < n_states; i++) {
        widthA = max(widthA, padding + sz(states[i]) + padding);
        for (int j = 0; j < (1 << n_inputs); j++) {
            widthNS[j] = max(widthNS[j], sz(nextStates[i][j]));
            if (myType == MEALY) widthO[j] = max(widthO[j], sz(stateOutput[i][j]));
        }
    }
    if(myType == MOORE) {
        for (int i = 0; i < n_states; i++) widthO[0] = max(widthO[0], sz(stateOutput[i][0]));
    }

    int temp_width = 0;
    for (int j = 0; j < (1 << n_inputs); j++) {
        temp_width += padding + widthNS[j] + padding;
    }
    widthB = max(widthB, temp_width);
    temp_width = 0;
    if(myType == MEALY) {
        for (int j = 0; j < (1 << n_inputs); j++) {
            temp_width += padding + widthO[j] + padding;
        }
    }
    else temp_width = padding + widthO[0] + padding;
    widthC = max(widthC, temp_width);

    H1 = horizLineWidth, H2 = H1 + 2 + horizLineWidth, H3 = H2 + n_states + horizLineWidth;
    V1 = vertiLineWidth, V2 = V1 + widthA + vertiLineWidth, V3 = V2 + widthB + vertiLineWidth, V4 = V3 + widthC + vertiLineWidth;
}

void StateTable::updateSprite() {
    updateData();

    sprite = Sprite("Default", TEX(V4, H3));
        
    for (int i = 1; i <= horizLineWidth; i++) {
        sprite.setLine(0, H1 - i, sprite.width, BOX_CHAR, 0);
        sprite.setLine(0, H2 - i, sprite.width, BOX_CHAR, 0);
        sprite.setLine(0, H3 - i, sprite.width, BOX_CHAR, 0);
    }
    for (int i = 1; i <= vertiLineWidth; i++) {
        sprite.setLine(V1 - i, 0, sprite.width, BOX_CHAR, 1);
        sprite.setLine(V2 - i, 0, sprite.width, BOX_CHAR, 1);
        sprite.setLine(V3 - i, 0, sprite.width, BOX_CHAR, 1);
        sprite.setLine(V4 - i, 0, sprite.width, BOX_CHAR, 1);
    }

    sprite.setText(V1 + (widthA - 7)/2, H1, "Present");
    sprite.setText(V1 + (widthA - 5)/2, H1 + 1, "State");
    sprite.setText(V2 + (widthB - 10)/2, H1, "Next State");
    sprite.setText(V3 + (widthC - 6)/2, H1, "Output");
    
    for (int j = 0, _x = V2 + padding; j < (1 << n_inputs); j++) {
        sprite.setText(_x + (widthNS[j] - n_inputs)/2, H1 + 1, binaryFormat(j, n_inputs));
        _x += widthNS[j] + padding + padding;
    }
    if(myType == MEALY){
        for (int j = 0, _x = V3 + padding; j < (1 << n_inputs); j++) {
            sprite.setText(_x + (widthO[j] - n_inputs)/2, H1 + 1, binaryFormat(j, n_inputs));
            _x += widthO[j] + padding + padding;
        }
    }

    for (int i = 0; i < n_states; i++) {
        sprite.setText(V1 + (widthA - sz(states[i]))/2, H2 + i, states[i]);
        
        for (int j = 0, _x = V2 + padding; j < (1 << n_inputs); j++) {
            sprite.setText(_x + (widthNS[j] - sz(nextStates[i][j]))/2, H2 + i, nextStates[i][j]);
            _x += widthNS[j] + padding + padding;
        }
        
        if (myType == MEALY) {
            for (int j = 0, _x = V3 + padding; j < (1 << n_inputs); j++) {
                sprite.setText(_x + (widthO[j] - sz(stateOutput[i][j]))/2, H2 + i, stateOutput[i][j]);
                _x += widthO[j] + padding + padding;
            }   
        }
        else sprite.setText(V3 + padding + (widthO[0] - sz(stateOutput[i][0]))/2, H2 + i, stateOutput[i][0]);
    }
}

void StateTable::display(int startX, int startY) {
    if (startX == -1) startX = sprite.startX;
    if (startY == -1) startY = sprite.startY;
    
    int c_x = 0, c_y = 0;
    if (location == onPS) {
        c_x = V1 + sz(states[c_row]) + (widthA - sz(states[c_row]))/2;
        c_y = H2 + c_row;
    }
    if (location == onNS) {
        c_x = V2 + padding;
        for (int i = 0; i < c_column; i++) c_x += widthNS[i] + padding + padding;
        c_x += sz(nextStates[c_row][c_column]) + (widthNS[c_column] - sz(nextStates[c_row][c_column]))/2;
        c_y = H2 + c_row;
    }
    if (location == onOutput) {
        c_x = V3 + padding;
        for (int i = 0; i < c_column; i++) c_x += widthO[i] + padding + padding;
        c_x += sz(stateOutput[c_row][c_column]) + (widthO[c_column] - sz(stateOutput[c_row][c_column]))/2;
        c_y = H2 + c_row;
    }

    Output().drawSprite(sprite, startX, startY);
    if (location != Outside) displayCursor(c_x + startX, c_y + startY);
}

StateTable::CursorLocation StateTable::processInput(){
    int key = Input().getKeyStroke();

    if (key == VK_ESCAPE) return Outside;

    if (location == onPS){
        if (key == VK_UP){
            if(c_row == 0){
                return Outside;
            }
            c_row--;
            return onPS;
        }
        if (key == VK_DOWN){
            if(c_row == n_states - 1){
                return Outside;
            }
            c_row++;
            return onPS;
        }
        if (key == VK_LEFT){
            return Outside;
        }
        if (key == VK_RIGHT){
            c_column = 0;
            return onNS;
        }
        processInputField(states[c_row], ST_PS_MAX_LENGTH);
        return onPS;
    }

    if(location == onNS){
        if (key == VK_UP){
            if (c_row == 0){
                return Outside;
            }
            c_row--;
            return onNS;
        }
        if (key == VK_DOWN){
            if (c_row == n_states - 1){
                return Outside;
            }
            c_row++;
            return onNS;
        }
        if (key == VK_LEFT){
            if (c_column == 0){
                return onPS;
            }
            c_column--;
            return onNS;
        }
        if (key == VK_RIGHT){
            if (c_column == (1 << n_inputs) - 1){
                c_column = 0;
                return onOutput;
            }
            c_column++;
            return onNS;
        }
        processInputField(nextStates[c_row][c_column], ST_NS_MAX_LENGTH);
        return onNS;
    }

    if (location == onOutput){
        if (key == VK_UP){
            if (c_row == 0){
                return Outside;
            }
            c_row--;
            return onOutput;
        }
        if (key == VK_DOWN){
            if (c_row == n_states - 1){
                return Outside;
            }
            c_row++;
            return onOutput;
        }
        if (key == VK_LEFT){
            if (c_column == 0){
                c_column = (1 << n_inputs) - 1;
                return onNS;
            }
            c_column--;
            return onOutput;
        }
        if (key == VK_RIGHT){
            if (myType == MOORE || c_column == (1 << n_inputs) - 1){
                return onOutput;
            }
            c_column++;
            return onOutput;
        }
        processInputField(stateOutput[c_row][c_column], ST_OUTPUT_MAX_LENGTH);
        return onOutput;
    }

    return Outside;
}

void ImplicationTable::create(int i, int j, const vector<string>& st1, const vector<string>& st2) {
    Box& box = boxes[page][i - 1][j];
    box.imps.clear();
    box.width = 5, box.height = 3;
    box.boxState = 0;
    for(int i = 0; i < sz(st1); i++) {
        int x1 = id[st1[i]], x2 = id[st2[i]];
        if(x1 == x2) continue;
        box.imps.push_back({x1, x2, 0});
        box.width = max(box.width, padding*2 + sz(states[x1]) + sz(states[x2]) + 1);
    }
    box.n_imps = sz(box.imps);
    box.height = box.n_imps + 1;
    boxWidth[j] = max(boxWidth[j], box.width);
    boxHeight[i - 1] = max(boxHeight[i - 1], box.height);
}

int ImplicationTable::boxState(int i, int j) {
    if(i == j) return 1;
    if(i < j) swap(i, j);
    return boxes[page][i - 1][j].boxState;
}

bool ImplicationTable::recheck(int i, int j) {
    Box& box = boxes[page][i - 1][j];
    if(box.boxState != 0) return 0;

    bool change = 0;
    for(auto& [i_i, i_j, i_state] : box.imps) {
        int imp_state = boxState(i_i, i_j);
        if(imp_state != i_state) change = 1, i_state = imp_state;
    }
    return change;
}

void ImplicationTable::recalcState(int i, int j) {
    Box& box = boxes[page][i - 1][j];
    if(box.boxState) return;

    box.boxState = 1;
    for(auto& [i_i, i_j, i_state] : box.imps) 
        box.boxState = min(box.boxState, i_state);
}

ImplicationTable::ImplicationTable(const StateTable& curTable) {
    n_states = curTable.n_states;
    boxWidth.assign(n_states - 1, 0), boxHeight.assign(n_states - 1, 0);
    states = curTable.states;
    id.clear();
    for(int i = 0; i < n_states; i++) id[states[i]] = i;
    page = lastPage = 0;
    boxes.resize(1);
    boxes[0].resize(n_states - 1);
    for(int i = 1; i < n_states; i++) {
        boxes[page][i - 1].resize(i);
        for(int j = 0; j < i; j++) {
            if(curTable.stateOutput[i] != curTable.stateOutput[j]) {
                //log(to_string(i) + " " + to_string(j) + "\n");
                Box& box = boxes[page][i - 1][j];
                box.imps.clear();
                box.width = 5, box.height = 2;
                box.boxState = -1;
                box.n_imps = 0;
                boxWidth[j] = max(boxWidth[j], box.width);
                boxHeight[i - 1] = max(boxHeight[i - 1], box.height);
                continue;
            }
            create(i, j, curTable.nextStates[i], curTable.nextStates[j]);
        }
    }
    for(bool flag = 1; flag; ) {
        flag = 0;
        boxes.push_back(boxes.back());
        page = sz(boxes) - 1;

        for(int i = 1; i < n_states; i++) {
            for(int j = 0; j < i; j++){
                recalcState(i, j);
            }
        }

        for(int i = 1; i < n_states; i++) {
            for(int j = 0; j < i; j++) {
                flag |= recheck(i, j);
            }
        }
    }
    lastPage = page, page = 0;
}

vector<int> ImplicationTable::getEquivalentClasses() {
    vector<int> equivalent_classes(n_states);
    int tp = 1;
    for(int i = 0; i < n_states; i++) {
        equivalent_classes[i] = tp;
        for(int j = 0; j < i; j++){
            if(boxState(i, j) != -1) equivalent_classes[i] = equivalent_classes[j];
        }
        tp = max(tp, equivalent_classes[i] + 1);
    }
    return equivalent_classes;
}

void ImplicationTable::render(int i, int j){
    Box& box = boxes[page][i - 1][j];
    box.sprite = Sprite("Default", TEX(boxWidth[j] + 2*vertiLineWidth, boxHeight[i - 1] + 2*horizLineWidth, 0));
    WORD clr;
    int clrFG, clrBG;
    for(int k = 0; k < box.n_imps; k++) {
        int st1 = box.imps[k][0], st2 = box.imps[k][1], stst = box.imps[k][2];
        clr = (stst == 0 ? DEFAULT_COLOR : stst == 1 ? GREEN_BUTTON : RED_BUTTON);
        clrFG = getFG(clr), clrBG = getBG(clr);
 
        string text = states[st1] + "/" + states[st2];
        box.sprite.setText(vertiLineWidth + (boxWidth[j] - sz(text))/2, horizLineWidth + k, text, clrFG, clrBG);
    }
    clr = (box.boxState == 0 ? DEFAULT_COLOR : box.boxState == 1 ? GREEN_BUTTON : RED_BUTTON);
    clrFG = getFG(clr), clrBG = getBG(clr);
    wchar_t text = (box.boxState == 0 ? L'?' : box.boxState == 1 ? L'✓' : L'X');
 
    box.sprite.setCell(vertiLineWidth + (boxWidth[j] - 1)/2, horizLineWidth + boxHeight[i - 1] - 1, text, clrFG, clrBG);
    if(box.boxState != 0) box.sprite.setTextureFG(clrFG), box.sprite.setTextureBG(clrBG);

    box.sprite.setBox(vertiLineWidth, horizLineWidth, boxWidth[j], boxHeight[i - 1], horizLineWidth, vertiLineWidth);
}

void ImplicationTable::updateSprite(){
    shift = 0;
    for(auto& i : states) shift = max(shift, sz(i));
    shift += 1;
    sprite.width = shift + vertiLineWidth,
    sprite.height = 1 + horizLineWidth;
    for(int i = 0; i < n_states - 1; i++) {
        sprite.width += boxWidth[i] + vertiLineWidth,
        sprite.height += boxHeight[i] + horizLineWidth;
    }
    sprite = Sprite("Default", TEX(sprite.width, sprite.height));

    for(int i = 1, h = horizLineWidth; i < n_states; h += boxHeight[i - 1] + horizLineWidth, i++){
        string text = states[i];
        sprite.setText(shift - sz(text) - 1, h + (boxHeight[i - 1] - 1)/2, text);
    }
    for(int i = 0, w = vertiLineWidth; i < n_states - 1; i++, w += boxWidth[i] + vertiLineWidth){
        string text = states[i];
        sprite.setText(shift + w + (boxWidth[i] - sz(text))/2, sprite.height - 1, text);
    }

    for(int i = 1; i < n_states; i++){
        for(int j = 0; j < i; j++){
            render(i, j);
            Box& box = boxes[page][i - 1][j];
            box.sprite.startX = shift, box.sprite.startY = 0;
            if(j > 0){
                Box& tmp_box = boxes[page][i - 1][j - 1];
                box.sprite.startX = tmp_box.sprite.startX + tmp_box.sprite.width - vertiLineWidth, 
                box.sprite.startY = tmp_box.sprite.startY;
            }
            else if(i > 1){
                Box& tmp_box = boxes[page][i - 2][j];
                box.sprite.startX = tmp_box.sprite.startX, 
                box.sprite.startY = tmp_box.sprite.startY + tmp_box.sprite.height - horizLineWidth;
            }
            sprite.setSprite(box.sprite);
        }
    }
}

void processInputField(string& text, int maxL) {
    char c = getTypedCharacter();
    if (c == '\0') return;
    if (c == '\b') {
        if (sz(text)) text.pop_back();
        return;
    }
    if (sz(text) == maxL) return;
    text += c;
}