#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "Commands.h"
#include "signals.h"

int main(int argc, char* argv[]) {
    if(signal(SIGTSTP , ctrlZHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-Z handler");
    }
    if(signal(SIGINT , ctrlCHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }

    // Set up the signal handler
    struct sigaction action;
    action.sa_handler = alarmHandler;
    if(sigemptyset(&action.sa_mask) == -1) {
        perror("smash error: sigemptyset failed");
    }
    action.sa_flags = SA_RESTART;
    if(sigaction(SIGALRM, &action, NULL) == -1) {
        perror("smash error: failed to set alarm handler");
    }

    SmallShell& smash = SmallShell::getInstance();
    while(true) {
        std::cout << smash.getPromptStr() << "> ";
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        smash.executeCommand(cmd_line.c_str());
    }
    return 0;
}