#include <iostream>
#include <signal.h>
#include <unistd.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";


bool _isBackgroundCommand(const char* cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void ctrlZHandler(int sig_num) {
    cout << "smash: got ctrl-Z\n";
    SmallShell& smash = SmallShell::getInstance();
    int processToStop = smash.getCurrentFgPid();
    if(processToStop==-1)
        return;
    string cmd = smash.getCurrentFgCommand();
    if (cmd.empty())
        return;
    smash.getJobsList()->addExistingJob(cmd, processToStop, true);
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
    smash.getJobsList()->removeFinishedJobs();

    TimeoutCommand* topTimeout = smash.topTimeoutCommand();
//        int pid;

    int currentEnd = topTimeout->getExpectedEnd();

    while (!smash.isTimeoutQueueEmpty() && currentEnd == smash.topTimeoutCommand()->getExpectedEnd())
    {
        int isExist = kill(topTimeout->getProcessId(), 0);
        topTimeout = smash.topTimeoutCommand();
//            pid = topTimeout->getProcessId();
        // if(!_isBackgroundCommand(topTimeout->getCmdLine()) || smash.getJobsList()->getJobByPid(pid) != nullptr)
        if(isExist == 0)
        {
            if(kill(smash.topTimeoutCommand()->getProcessId(), SIGKILL) == -1) {
                perror("smash error: kill failed");
            }
            cout << "smash: " << topTimeout->getCmdLine() << " timed out!" << endl;
        }
        smash.popTimeoutCommand();
    }

    smash.scheduleTimeoutAlarm();
}

