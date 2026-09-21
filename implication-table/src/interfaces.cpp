#include "common.h"
#include "console_API.h"
#include "interfaces.h"
#include "tables.h"

map<Interface, function<Interface()>> interfaceMap;

static Interface welcomeInterface() {
    return WELCOME;
}

static Interface mainMenuInterface() {
    return MAIN_MENU;
}

static Interface fsmImplicationInterface() {    
    StateTable curTable;

    Button mealyButton = Button("Mealy", GREEN_BUTTON_PRESET_A);
    mealyButton.enable(1);
    Button mooreButton = Button("Moore", GREEN_BUTTON_PRESET_A);
    Button addButton = Button("Add Row", GREEN_BUTTON_PRESET_B);
    Button addInputButton = Button("+", GREEN_BUTTON_PRESET_B);
    Button delInputButton = Button("-", RED_BUTTON_PRESET_B);    
    vector<Button> deleteButtons(curTable.n_states, Button("Delete Row", RED_BUTTON_PRESET_B));
    Button generateButton = Button("Generate Implication Table", BLUE_BUTTON_PRESET_B);
    generateButton.setCLR(0b100, createColor(BLACK | BRIGHT, BLACK));
    generateButton.setCLR(0b101, createColor(BLACK, BLACK | BRIGHT));
    enum CursorLocation {
        onStateTable,
        onMealyButton,
        onMooreButton,
        onAddButton,
        onDeleteButton,
        onGenerateButton,
        onAddInputButton,
        onDelInputButton,
        Nothing
    };

    CursorLocation location = onStateTable;
    int c_row = 0, c_column = 0;
    
    auto processInput = [&]() -> Interface {
        int key = Input().getKeyStroke();
        if(location == onStateTable){
            if ((curTable.location = curTable.processInput()) != StateTable::Outside) return NULL_INTERFACE;
            if (key == VK_UP) location = onAddInputButton;
            if (key == VK_DOWN) location = onAddButton;
            if (key == VK_LEFT) location = onDeleteButton, c_row = 0;
            return NULL_INTERFACE;
        }

        if(location == onAddInputButton) {
            if (key == VK_DOWN) {
                if (curTable.n_states == 0) {
                    location = onAddButton;
                    return NULL_INTERFACE;
                }
                location = onStateTable;
                curTable.location = StateTable::onPS;
                curTable.c_row = curTable.c_column = 0;
            }
            if (key == VK_UP) location = onMooreButton;
            if (key == VK_LEFT) location = onDelInputButton;
            if (key == VK_RETURN) {
                curTable.n_inputs++;
                curTable.nextStates.assign(curTable.n_states, vector<string>(1 << curTable.n_inputs, ""));
                if(curTable.myType == MEALY)
                    curTable.stateOutput.assign(curTable.n_states, vector<string>(1 << curTable.n_inputs, ""));
            }
            return NULL_INTERFACE;
        }

        if(location == onDelInputButton) {
            if (key == VK_DOWN) {
                if (curTable.n_states == 0) {
                    location = onAddButton;
                    return NULL_INTERFACE;
                }
                location = onStateTable;
                curTable.location = StateTable::onPS;
                curTable.c_row = curTable.c_column = 0;
            }
            if (key == VK_UP) location = onMealyButton;
            if (key == VK_RIGHT) location = onAddInputButton;
            if (key == VK_RETURN && curTable.n_inputs > 0) {
                curTable.n_inputs--;
                curTable.nextStates.assign(curTable.n_states, vector<string>(1 << curTable.n_inputs, ""));
                if(curTable.myType == MEALY)
                    curTable.stateOutput.assign(curTable.n_states, vector<string>(1 << curTable.n_inputs, ""));
            }
            return NULL_INTERFACE;
        }

        if(location == onMealyButton){
            if (key == VK_RIGHT) location = onMooreButton;
            if (key == VK_DOWN) location = onDelInputButton;
            if (key == VK_RETURN){
                if (curTable.myType == MOORE) {
                    curTable.stateOutput.assign(curTable.n_states, vector<string>(1 << curTable.n_inputs));
                    curTable.myType = MEALY;
                    mealyButton.enable(1);
                    mooreButton.disable(1);
                }
            }
            return NULL_INTERFACE;
        }

        if(location == onMooreButton){
            if (key == VK_LEFT) location = onMealyButton;
            if (key == VK_DOWN) location = onAddInputButton;
            if (key == VK_RETURN){
                if (curTable.myType == MEALY) {
                    curTable.stateOutput.assign(curTable.n_states, vector<string>(1));
                    curTable.myType = MOORE;
                    mealyButton.disable(1);
                    mooreButton.enable(1);
                }
            }
            return NULL_INTERFACE;
        }

        if (location == onAddButton) {
            if (key == VK_UP) {
                if (curTable.n_states == 0) {
                    location = onAddInputButton;
                    return NULL_INTERFACE;
                }
                location = onStateTable;
                curTable.location = StateTable::onPS;
                curTable.c_row = curTable.n_states - 1, curTable.c_column = 0;
            }
            if (key == VK_RETURN) {
                curTable.states.push_back("");
                curTable.nextStates.push_back(vector<string>(1 << curTable.n_inputs));
                curTable.stateOutput.push_back(vector<string>(curTable.myType == MEALY ? (1 << curTable.n_inputs) : 1));
                deleteButtons.push_back(Button("Delete Row", RED_BUTTON_PRESET_B));
                curTable.n_states++;
            }
            if (key == VK_LEFT) {
                location = onDeleteButton;
                c_row = sz(deleteButtons) - 1;
            }
            if (key == VK_DOWN) {
                location = onGenerateButton;
            }
            return NULL_INTERFACE;
        }

        if (location == onDeleteButton) {
            if (key == VK_RIGHT) {
                location = onStateTable;
                curTable.location = StateTable::onPS;
                curTable.c_row = 0, curTable.c_column = 0;
            }
            if (key == VK_UP) {
                if(c_row == 0) location = onMealyButton;
                else c_row--;
            }
            if (key == VK_DOWN){
                if (c_row == sz(deleteButtons) - 1) location = onAddButton;
                else c_row++;
            }
            if (key == VK_RETURN){
                ERASE(curTable.states, c_row);
                ERASE(curTable.nextStates, c_row);
                ERASE(curTable.stateOutput, c_row);
                curTable.n_states--;
                ERASE(deleteButtons, c_row);

                if (curTable.n_states == 0) {
                     location = onAddButton;
                     return NULL_INTERFACE;
                }
                if (c_row == sz(deleteButtons)) c_row--;
            }
            return NULL_INTERFACE;
        }
        if (location == onGenerateButton) {
            if (key == VK_UP) location = onAddButton;
            if (key == VK_RETURN && !generateButton.activated(2)) generateImpTable(curTable); 
            return NULL_INTERFACE;
        }
        return NULL_INTERFACE;
    };

    auto updateInterface = [&]() -> void {
        curTable.updateSprite();

        mealyButton.disable(0);
        mooreButton.disable(0);
        addButton.disable(0);
        addInputButton.disable(0);
        delInputButton.disable(0);
        for (auto& i : deleteButtons) i.disable(0);
        generateButton.disable(0);
        
        if(location == onMealyButton) mealyButton.enable(0);
        if(location == onMooreButton) mooreButton.enable(0);
        if(location == onAddButton) addButton.enable(0);
        if(location == onDeleteButton) deleteButtons[c_row].enable(0);
        if(location == onGenerateButton) generateButton.enable(0);
        if(location == onAddInputButton) addInputButton.enable(0);
        if(location == onDelInputButton) delInputButton.enable(0);
        
        generateButton.disable(2);
        if(curTable.validation()) generateButton.enable(2);
    };

    auto displayInterface = [&]() -> void {
        int termW, termH;
        getWindowSize(termW, termH);

        int tableX = max((termW - curTable.sprite.width)/2, sz(string("Delete Row")) + 1);
        int tableY = 7;
        curTable.display(tableX, tableY);
        
        int middle = termW/2;
        int mealyX = max(middle - 7, 0);
        int boxX = max(middle - 3, 1);
        Output().drawSprite(addInputButton.sprite, 
                            boxX + 5, 
                            4);
        Output().drawSprite(delInputButton.sprite, 
                            boxX, 
                            4);
        Output().drawBox(boxX, 4, 6, 1);
        Output().drawBox(boxX + 2, 4, 2, 1);
        Output().renderBoxes();
        
        Output().drawText(boxX + 2, 4, to_string(curTable.n_inputs));
        Output().drawText(boxX, 3, "Inputs");

        Output().drawSprite(mealyButton.sprite, 
                            mealyX, 
                            0);

        Output().drawSprite(mooreButton.sprite, 
                            mealyX + 7, 
                            0);

        Output().drawSprite(addButton.sprite, 
                            middle - 1 - sz(string("Add Row"))/2, 
                            tableY + curTable.sprite.height + 1);

        for (int i = 0; i < sz(deleteButtons); i++)
            Output().drawSprite(deleteButtons[i].sprite, 
                                tableX - sz(string("Delete Row")) - 1, 
                                tableY + curTable.H2 + i);
        
        Output().drawSprite(generateButton.sprite, 
                            middle - 1 - generateButton.sprite.width/2, 
                            addButton.sprite.startY + addButton.sprite.height + 1);
        
        string text = "";
        if(location == onGenerateButton){
            int x = curTable.validation();
            if(x == 1){
                text = "You need to fill all the cells in the table";
            }
            if(x == 2){
                text = "Present state variables must be unique";
            }
            if(x == 3){
                text = "Next state elements must exist in the Present state variables";
            }
        }
        int tmp_x = max(0, middle - sz(text)/2), tmp_y = generateButton.sprite.startY + generateButton.sprite.height + 1;
        Output().drawText(tmp_x, tmp_y, text, RED, YELLOW);
    };

    auto adjustFocus = [&]() -> void {
        Output().focus(mealyButton.sprite);
        if(location == onMealyButton) Output().focus(mealyButton.sprite);
        if(location == onMooreButton) Output().focus(mooreButton.sprite);
        if(location == onAddButton) Output().focus(addButton.sprite);
        if(location == onDeleteButton) Output().focus(deleteButtons[c_row].sprite);
        if(location == onGenerateButton) Output().focus(generateButton.sprite);
        if(location == onAddInputButton) Output().focus(addInputButton.sprite);
        if(location == onDelInputButton) Output().focus(delInputButton.sprite);        
    };
    
    for (Input().clear(); ;tick()) {

        Interface nextInterface = processInput();
        if(nextInterface != NULL_INTERFACE) return nextInterface;
        
        updateInterface();
        displayInterface();
        adjustFocus();
    }
    return EXIT;
}

void generateImpTable(const StateTable& curTable) {
    ImplicationTable impTable(curTable);
    
    Button nextPageButton = Button("Next", YELLOW_BUTTON_PRESET_B);
    Button prevPageButton = Button("Back", YELLOW_BUTTON_PRESET_B);
    nextPageButton.setText(0b100, "Generate Minimized Table", BRIGHT | BLUE, BLACK);
    nextPageButton.setText(0b101, "Generate Minimized Table", BLACK, BRIGHT | BLUE);
    prevPageButton.setText(0b100, "", MAGENTA, BLACK);
    prevPageButton.setText(0b101, "", BLACK, MAGENTA);
    Button goBackButton = Button("Return back to state table");

    enum CursorLocation {
        onGoBackButton,
        onNextPageButton,
        onPrevPageButton,
        Nothing
    };

    CursorLocation location = onNextPageButton;
    int c_row = 0, c_column = 0;

    auto processInput = [&]() -> bool {
        int key = Input().getKeyStroke();
        if(location == onGoBackButton) {
            if(key == VK_RETURN) return 1;
            if(key == VK_UP) {
                location = onNextPageButton;
            }
            return 0;
        }

        if(location == onPrevPageButton) {
            if(key == VK_DOWN) {
                location = onGoBackButton;
            }
            if(key == VK_RIGHT) {
                location = onNextPageButton;
            }
            if(key == VK_RETURN) {
                impTable.page--;
                if(impTable.page == 0) location = onNextPageButton;
            }
            return 0;
        }
        
        if(location == onNextPageButton) {
            if(key == VK_DOWN) {
                location = onGoBackButton;
            }
            if(key == VK_LEFT) {
                if(!prevPageButton.activated(2)) location = onPrevPageButton;
            }
            if(key == VK_RETURN) {
                if(impTable.page != impTable.lastPage) impTable.page++;
                else {
                    generateMinimizedTable(curTable, impTable);
                }
            }
            return 0;
        }
        return 0;
    };

    auto updateInterface = [&]() -> void {
        impTable.updateSprite();
        
        prevPageButton.disable(0);
        nextPageButton.disable(0);
        goBackButton.disable(0);
        prevPageButton.disable(2);
        nextPageButton.disable(2);

        if(impTable.page == 0) prevPageButton.enable(2); 
        if(impTable.page == impTable.lastPage) nextPageButton.enable(2); 

        if(location == onNextPageButton) nextPageButton.enable(0);
        if(location == onPrevPageButton) prevPageButton.enable(0);
        if(location == onGoBackButton) goBackButton.enable(0);
    };

    auto displayInterface = [&]() -> void {
        int termW, termH;
        getWindowSize(termW, termH);

        int tableX = max((termW - impTable.sprite.width + impTable.shift)/2 - impTable.shift, 0), tableY = 1;
        Output().drawSprite(impTable.sprite, tableX, tableY);
        
        int middle = termW/2;
        int prevX = max(middle - 4 - 2, 0);
        
        if(impTable.page == impTable.lastPage){
            string text = "(Simplification finished, table can't be simplified further)";
            Output().drawText(middle - sz(text)/2, tableY + impTable.sprite.height, text);
        }

        Output().drawSprite(prevPageButton.sprite, 
                            prevX, 
                            tableY + impTable.sprite.height + 2 + 1);

        Output().drawSprite(nextPageButton.sprite, 
                            prevX + 2*2 + 4, 
                            tableY + impTable.sprite.height + 2 + 1);
        Output().drawSprite(goBackButton.sprite, 
                            max(middle - goBackButton.sprite.width/2, 0), 
                            tableY + impTable.sprite.height + 4 + 1);
    };

    auto adjustFocus = [&]() -> void {
        Output().focus(impTable.sprite);
        if(location == onNextPageButton) Output().focus(nextPageButton.sprite);
        if(location == onPrevPageButton) Output().focus(prevPageButton.sprite);
        if(location == onGoBackButton) Output().focus(goBackButton.sprite);
    };

    for (Input().update(); ;tick()) {
        bool goBack = processInput();
        if(goBack) return;
        
        updateInterface();
        displayInterface();
        adjustFocus();
    }
    return;
}

void generateMinimizedTable(const StateTable& oldTable, ImplicationTable& impTable) {
    StateTable newTable = oldTable;
    vector<int> equivalent_classes = impTable.getEquivalentClasses();
    newTable.minimize(equivalent_classes);
    
    Button goBackButton = Button("Return Back");
    
    newTable.updateSprite();

    int maxW = 1;

    for(int i = 1; i <= newTable.n_states; i++) {
        int L = sz(string("{} -> ")) + 1 + sz(base26(i));
        for(int j = 0; j < oldTable.n_states; j++){
            if(equivalent_classes[j] == i) L+= 1 + sz(oldTable.states[j]);
        }
        maxW = max(maxW, L);
    }

    Sprite EC_sprite = Sprite("Default", TEX(maxW, newTable.n_states));

    for(int i = 1; i <= newTable.n_states; i++) {
        EC_sprite.setCell(0, i - 1, L'⬤', i % 15 + 1);
        string text = "{";
        for(int j = 0; j < oldTable.n_states; j++){
            if(equivalent_classes[j] == i) text += oldTable.states[j] + ",";
        }
        text.pop_back();
        text += "} -> " + base26(i);
        EC_sprite.setText(2, i - 1, text);
    }

    enum CursorLocation {
        onGoBackButton,
        Nothing
    };

    CursorLocation location = onGoBackButton;
    int c_row = 0, c_column = 0;

    auto processInput = [&]() -> bool {
        int key = Input().getKeyStroke();
        if(key == VK_RETURN) return 1;
        return 0;  
    };

    auto updateInterface = [&]() -> void {
        goBackButton.disable(0);
        
        if(location == onGoBackButton) goBackButton.enable(0);
    };

    auto displayInterface = [&]() -> void {
        int termW, termH;
        getWindowSize(termW, termH);
        int middle = termW/2;
        int ECX = max(middle - EC_sprite.width/2, 0);
        int ECY = 1;
        int tableX = max((termW - newTable.sprite.width)/2, 0);
        int tableY = ECY + EC_sprite.height + 2;
        newTable.display(tableX, tableY);
        Output().drawSprite(EC_sprite, ECX, ECY);
        Output().drawSprite(goBackButton.sprite, 
                            middle - goBackButton.sprite.width/2, 
                            tableY + newTable.sprite.height + 2);
    };

    auto adjustFocus = [&]() -> void {
        Output().focus(EC_sprite);
        if(location == onGoBackButton) Output().focus(goBackButton.sprite);
    };

    for (Input().update(); ;tick()) {

        bool goBack = processInput();
        if(goBack) return;
        
        updateInterface();
        displayInterface();
        adjustFocus();

    }
}

void initInterface() {
    interfaceMap[WELCOME] = welcomeInterface;
    interfaceMap[MAIN_MENU] = mainMenuInterface;
    interfaceMap[FSM_IMPLICATION] = fsmImplicationInterface;
}

void launchInterface(Interface currentInterface) {
    while (currentInterface != EXIT){
        currentInterface = interfaceMap[currentInterface]();
    }
}