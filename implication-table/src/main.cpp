#include "common.h"
#include "console_API.h"
#include "interfaces.h"

int main() {
    atexit(shutdownTerminal);
    initTerminal();

    clearLog();

    initInterface();
    launchInterface(FSM_IMPLICATION);
    
    return 0;
}