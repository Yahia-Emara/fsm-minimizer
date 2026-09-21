#pragma once

#include "tables.h"

enum Interface{
    NULL_INTERFACE,
    WELCOME,
    MAIN_MENU,
    FSM_IMPLICATION,
    EXIT
};

void generateImpTable(const StateTable& curTable);
void generateMinimizedTable(const StateTable& oldTable, ImplicationTable& impTable);

void initInterface();
void launchInterface(Interface currentInterface);