#include <iostream>
#include <signal.h>
#include <unistd.h>
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
    cout << "smash: got ctrl-C\n";
    SmallShell& smash = SmallShell::getInstance();
    int processToKill = smash.getCurrentFgPid();
    if(processToKill==-1){
        return;
    }
    if(kill(processToKill, SIGKILL) == -1){
        perror("smash error: kill failed");
    }
    cout << "smash: process " << processToKill << " was killed\n";
    smash.setCurrentFgCommand("");
    smash.setCurrentFgPid(-1);
}

void alarmHandler(int sig_num) {
    cout << "smash: got an alarm\n";
    SmallShell& smash = SmallShell::getInstance();

    TimeoutCommand* topTimeout = smash.topTimeoutCommand();
    int currentEnd = topTimeout->getExpectedEnd();

    while (!smash.isTimeoutQueueEmpty() && currentEnd == smash.topTimeoutCommand()->getExpectedEnd())
    {
        if(kill(smash.topTimeoutCommand()->getProcessId(), SIGKILL) == -1){
            perror("smash error: kill failed");
        }
        cout << "smash: " << smash.topTimeoutCommand()->getCmdLine() << " timed out!\n";
        smash.popTimeoutCommand();
    }

    if(smash.isTimeoutQueueEmpty())
        return;

    int curTime = time(NULL);
    if(curTime == -1)
    {
        perror("smash error: time failed");
        return;
    }
    int closestTimeout = smash.topTimeoutCommand()->getExpectedEnd() - curTime;
    alarm(closestTimeout);
}

