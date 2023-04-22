#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;


void ctrlZHandler(int sig_num) {
    cout << "smash: got ctrl-Z\n";
    SmallShell& smash = SmallShell::getInstance();
    int processToStop = smash.getCurrentFgPid();
    if(processToStop==-1)
        return;
    string cmd = smash.getCurrentFgCommand();
    if (cmd.empty())
        return;
    smash.getJobsList()->addJob(cmd, processToStop, true);
    if (kill(processToStop, SIGSTOP) == -1) {
        perror("smash error: kill failed");
    }
    cout << "smash: process " << processToStop << " was stopped\n";

    smash.setCurrentFgCommand("");
    smash.setCurrentFgPid(-1);
}

void ctrlCHandler(int sig_num) {
  // TODO: Add your implementation
}

void alarmHandler(int sig_num) {
  // TODO: Add your implementation
}

