#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <algorithm>
#include <fcntl.h>
#include <sched.h>
#include <sys/stat.h>
#include <set>


using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

string _ltrim(const std::string& s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
  return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char* cmd_line, char** args) {
  FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for(std::string s; iss >> s; ) {
    args[i] = (char*)malloc(s.length()+1);
    memset(args[i], 0, s.length()+1);
    strcpy(args[i], s.c_str());
    args[++i] = NULL;
  }
  return i;

  FUNC_EXIT()
}

bool _isBackgroundComamnd(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
  const string str(cmd_line);
  // find last character other than spaces
  unsigned int idx = str.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return
  if (idx == string::npos) {
    return;
  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&') {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

// TODO: Add your implementation for classes in Commands.h

int SmallShell::smashPid = getpid();

SmallShell::SmallShell() : promptStr("smash"), lastPwd(nullptr), jobsList(),
    currentFgPid(-1), currentFgCommand(""), timeoutQueue() {

}

SmallShell::~SmallShell() {
    delete[] lastPwd;
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
	// For example:

  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
  string cmdLineCopy = string(_trim(cmd_line));
  // cmdLineCopy.erase(remove(cmdLineCopy.begin(), cmdLineCopy.end(), '&'), cmdLineCopy.end());
    string firstWordBuiltIn = firstWord;
  if(cmdLineCopy[cmdLineCopy.size() - 1] == '&'){
      cmdLineCopy = cmdLineCopy.substr(0, cmdLineCopy.size()-1);
      firstWordBuiltIn = cmdLineCopy.substr(0, cmdLineCopy.find_first_of(" \n"));
  }

    SmallShell& smash = SmallShell::getInstance();

  if (RedirectionCommand::isRedirection(cmd_line))
  {
      return new RedirectionCommand(cmd_line);
  }
  else if(PipeCommand::isPipeCommand(cmd_line) || PipeCommand::isErrorPipeType(cmd_line)){
      return new PipeCommand(cmd_line);
  }
  else if (firstWordBuiltIn.compare("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWordBuiltIn.compare("showpid") == 0) {
    return new ShowPidCommand(cmd_line);
  }
  else if (firstWordBuiltIn.compare("chprompt") == 0){
      return new ChangePromptCommand(cmd_line);
  }
  else if (firstWordBuiltIn.compare("cd") == 0){
      return new ChangeDirCommand(cmd_line, smash.getLastPwd());
  }
  else if (firstWordBuiltIn.compare("jobs") == 0)
  {
      return new JobsCommand(cmd_line, smash.getJobsList());
  }
  else if (firstWordBuiltIn.compare("fg") == 0)
  {
      return new ForegroundCommand(cmd_line, smash.getJobsList());
  }
  else if (firstWordBuiltIn.compare("bg") == 0)
  {
      return new BackgroundCommand(cmd_line, smash.getJobsList());
  }
  else if (firstWordBuiltIn.compare("quit") == 0)
  {
      return new QuitCommand(cmd_line, smash.getJobsList());
  }
  else if (firstWordBuiltIn.compare("kill") == 0)
  {
      return new KillCommand(cmd_line, smash.getJobsList());
  }
    else if (firstWordBuiltIn.compare("setcore") == 0)
    {
        return new SetcoreCommand(cmd_line, smash.getJobsList());
    }
    else if (firstWordBuiltIn.compare("getfiletype") == 0)
    {
        return new GetFileTypeCommand(cmd_line);
    }
    else if (firstWordBuiltIn.compare("chmod") == 0)
    {
        return new ChmodCommand(cmd_line);
    }
    else if (firstWordBuiltIn.compare("timeout") == 0)
    {
        return new TimeoutCommand(cmd_line);
    }


//  .....
  else
  {
      return new ExternalCommand(cmd_line);
  }
  return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
  // TODO: Add your implementation here
  // for example:
  if(string(cmd_line).empty())
  {
      return;
  }
  jobsList.removeFinishedJobs();
   Command* cmd = CreateCommand(cmd_line);
   cmd->execute();
   delete cmd;
  // Please note that you must fork smash process for some commands (e.g., external commands....)
}


void releaseArgsArray(char* args[])
{
    for (int i = 0; i <= COMMAND_MAX_ARGS; i++) {
        if(args[i] != NULL)
            free(args[i]);
    }
}

string SmallShell::getPromptStr(){
    return this->promptStr;
}

void SmallShell::setPromptStr(const std::string newPromptStr){
    this->promptStr = newPromptStr;
}

char** SmallShell::getLastPwd() {
    return &lastPwd;
}

void SmallShell::setLastPwd(char* lastPwd1) {
    SmallShell::lastPwd = lastPwd1;
}

JobsList *SmallShell::getJobsList() {
    return &jobsList;
}

int SmallShell::getSmashPid(){
    return this->smashPid;
}

int SmallShell::getCurrentFgPid() const {
    return currentFgPid;
}

void SmallShell::setCurrentFgPid(int currentFgPid) {
    this->currentFgPid = currentFgPid;
}

string SmallShell::getCurrentFgCommand() const{
    return this->currentFgCommand;
}

void SmallShell::setCurrentFgCommand(string newFgCommand){
    this->currentFgCommand = newFgCommand;
}

 priority_queue<TimeoutCommand *, std::vector<TimeoutCommand *>, CompareTimeout> & SmallShell::getTimeoutQueue() {
    return timeoutQueue;
}

void SmallShell::insertTimeoutCommand(TimeoutCommand *cmd){
    this->timeoutQueue.push(cmd);
}

bool SmallShell::isTimeoutQueueEmpty() const {
    return this->timeoutQueue.empty();
}

TimeoutCommand *SmallShell::topTimeoutCommand() {
    return this->timeoutQueue.top();
}

void SmallShell::popTimeoutCommand() {
    TimeoutCommand* toDelete = this->timeoutQueue.top();
    delete toDelete->getCmdLine();
    this->timeoutQueue.pop();
    delete toDelete;
}

void SmallShell::removeTimeoutByPid(int pid) {
    int queueSize = this->timeoutQueue.size();
    vector<TimeoutCommand*> tempVec;
    for(int i = 0; i< queueSize; i++)
    {
        if(timeoutQueue.top()->getProcessId() == pid) {
            this->popTimeoutCommand();
            break;
        }
        tempVec.push_back(timeoutQueue.top());
        this->timeoutQueue.pop();
    }

    for(TimeoutCommand* cmd: tempVec){
        this->timeoutQueue.push(cmd);
    }
}

void SmallShell::scheduleTimeoutAlarm() {
    if(isTimeoutQueueEmpty())
    {
        alarm(0);
        return;
    }
    int curTime = time(NULL);
    if(curTime == -1)
    {
        perror("smash error: time failed");
        return;
    }
    int closestTimeout = this->topTimeoutCommand()->getExpectedEnd() - curTime;
    alarm(closestTimeout);

}


ChangePromptCommand::ChangePromptCommand(std::string cmd_s) : BuiltInCommand(cmd_s.c_str())
{
    char *args[COMMAND_MAX_ARGS+1]= {NULL};
    int argNum = _parseCommandLine(this->getCmdLine(), args);
    if(argNum>1){
        this->secondWord = args[1];
    }
    else{
        this->secondWord = "";
    }
    releaseArgsArray(args);
}

void ChangePromptCommand::execute()
{
    SmallShell& smash = SmallShell::getInstance();
//    cout << "before: " << secondWord << endl;
    secondWord = _trim(secondWord);
//    cout << "after: " << secondWord << endl;

    if(secondWord.length() == 0){
        smash.setPromptStr("smash");
    }
    else{
        smash.setPromptStr(secondWord);
    }
}

BuiltInCommand::BuiltInCommand(const char *cmd_line) : Command(cmd_line) {

}

Command::Command(const char *cmd_line) : cmd_line(cmd_line){

}

const char *Command::getCmdLine() const {
    return cmd_line;
}

void Command::setCmdLine(const char *cmdLine) {
    cmd_line = cmdLine;
}

string Command::getCommandType() const {
    string cmd_s = _trim(string(this->cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
    return firstWord;
}

void Command::prepare() {

}

void Command::cleanup() {

}

ShowPidCommand::ShowPidCommand(const char* cmd_line) : BuiltInCommand(cmd_line){}

void ShowPidCommand::execute(){
    SmallShell& smash = SmallShell::getInstance();
    cout << "smash pid is " << smash.getSmashPid() << endl;
}

GetCurrDirCommand::GetCurrDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {
}

void GetCurrDirCommand::execute() {
    char cwd[COMMAND_ARGS_MAX_LENGTH];
    if(getcwd(cwd, sizeof(cwd)) == NULL)
        perror("smash error: getcwd failed");
    else
        cout << cwd << endl;
}

ChangeDirCommand::ChangeDirCommand(const char *cmd_line, char **plastPwd)
    : BuiltInCommand(cmd_line), dirLastPwd(plastPwd)
{

}

void ChangeDirCommand::execute() {
    char *args[COMMAND_MAX_ARGS+1];
    for (int i = 0; i <= COMMAND_MAX_ARGS; i++) {
        args[i] = NULL;
    }
    int argNum = _parseCommandLine(this->getCmdLine(), args);
    if (argNum > 2) {
        cerr << "smash error: cd: too many arguments" << endl;
    } else if (argNum < 2) {
        cerr << "smash error:> \"" << this->getCmdLine() << "\"" << endl;
    } else {
        string path = args[1];
        releaseArgsArray(args);
        char *cwd = new char[COMMAND_ARGS_MAX_LENGTH];

        if (getcwd(cwd, sizeof(char) * COMMAND_ARGS_MAX_LENGTH) == NULL) {
            perror("smash error: getcwd failed");
            return;
        }
        if (path.compare("-") == 0) {
            if (*dirLastPwd == nullptr) {
                cerr << "smash error: cd: OLDPWD not set" << endl;
                return;
            } else {
                path = *dirLastPwd;
            }
        }

        //executing cd
        if (chdir(path.c_str()) != 0) {
            perror("smash error: chdir failed");
            return;
        }
        if (*(this->dirLastPwd) != nullptr) {
            delete[] *(this->dirLastPwd);
        }
        *dirLastPwd = cwd;
    }

}

JobsList::JobsList(): maxJobIdAvailable(1) {}

JobsList::~JobsList(){
    killAllJobs();
}
void JobsList::addJob(Command* cmd, int pid, bool isStopped){
    this->removeFinishedJobs();
    char* copy_str = new char[strlen(cmd->getCmdLine())]; // Allocate memory for the copy
    strcpy(copy_str, cmd->getCmdLine());
    JobEntry* newJob = new JobEntry(this->maxJobIdAvailable,
                                   copy_str, pid, isStopped);
    maxJobIdAvailable++;
    jobsVec.push_back(newJob);
    allJobs.push_back(newJob);
}

void JobsList::addExistingJob(string cmd, int pid, bool isStopped){
    this->removeFinishedJobs();
    int jobId = -1;
    for(JobEntry* job : allJobs)
    {
        if(job->processId == pid)
            jobId = job->jobId;
    }
    bool newJobFlag = false;
    if(jobId == -1)
    {
        jobId = maxJobIdAvailable;
        maxJobIdAvailable++;
        newJobFlag = true;
    }
    JobEntry* newJob = new JobEntry(jobId,
                                    cmd, pid, isStopped);
    if(newJobFlag)
    {
        allJobs.push_back(newJob);
    }
    if(jobId >= maxJobIdAvailable)
        maxJobIdAvailable = jobId + 1;
    jobsVec.push_back(newJob);
}

bool compareJobs(JobsList::JobEntry *job1, JobsList::JobEntry *job2) {
    return job1->jobId < job2->jobId;
}

void JobsList::printJobsList() {
    this->removeFinishedJobs();
    std::sort(jobsVec.begin(), jobsVec.end(), compareJobs);
    for (JobEntry* job : jobsVec) {
        cout << "[" << job->jobId << "] " << job->command << " : " << job->processId<<" "
             << job->calculateTimeElapsed() << " secs";
        if(job->isStopped)
            cout << " (stopped)";
        cout << endl;
    }
}

void JobsList::killAllJobs() {
    for (JobEntry* jobEntry : jobsVec)
    {
        if(kill(jobEntry->processId, SIGKILL) == -1) {
            perror("smash error: kill failed");
            return;
        }
    }
    removeFinishedJobs();
    for(JobEntry* jobEntry : allJobs)
    {
        delete jobEntry;
    }
    allJobs.clear();
    jobsVec.clear();
    maxJobIdAvailable = 1;
}

void JobsList::removeFinishedJobs()
{
    SmallShell& smash = SmallShell::getInstance();
    if(getpid() != smash.getSmashPid())
        return;
    if(this->isEmpty())
        return;
    vector<int> jobsToDelete;
    int childPid = waitpid(-1, NULL, WNOHANG);
    while(childPid > 0)
    {
        jobsToDelete.push_back(childPid);
        childPid = waitpid(-1, NULL, WNOHANG);
    }
    if(childPid == -1 && jobsToDelete.size() != (unsigned int) this->getJobsAmount())
        perror("smash error: waitpid failed");

    for(int finishedPId : jobsToDelete)
    {
        removeJobByPID(finishedPId);
    }
    for(int finishedPid : jobsToDelete)
    {
        smash.removeTimeoutByPid(finishedPid);
        smash.scheduleTimeoutAlarm();
        for(auto job = allJobs.begin(); job< allJobs.end(); job++)
        {
            if(finishedPid == (*job)->processId)
            {
                delete *job;
                allJobs.erase(job);
            }
        }
    }
}

JobsList::JobEntry *JobsList::getJobById(int jobId) {
    for(JobEntry* job: this->jobsVec)
    {
        if(job->jobId == jobId){
            return job;
        }
    }
    return nullptr;
}

void JobsList::removeJobById(int jobId) {
    for(auto job = jobsVec.begin(); job< jobsVec.end(); job++)
    {
        if((*job)->jobId == jobId) {
            jobsVec.erase(job);
        }
    }
    updateMaxId();
}

bool JobsList::isEmpty() const {
    return jobsVec.size() == 0;
}

int JobsList::getMaxJobIdAvailable() const {
    return maxJobIdAvailable;
}

JobsList::JobEntry *JobsList::getLastJob(int *lastJobId) {
    JobEntry* job = getJobById(maxJobIdAvailable-1);
    if(job == nullptr)
    {
        *lastJobId = -1;
        return nullptr;
    }
    *lastJobId = maxJobIdAvailable - 1;
    return job;
}

JobsList::JobEntry *JobsList::getLastStoppedJob(int *jobId) {
    int maxJobId = 0;
    JobEntry* currentMaxJob = nullptr;
    for(JobEntry* jobEntry : jobsVec)
    {
        if(jobEntry->jobId >= maxJobId && jobEntry->isStopped)
        {
            maxJobId = jobEntry->jobId;
            currentMaxJob = jobEntry;
        }
    }
    *jobId = maxJobId;
    return currentMaxJob;
}

int JobsList::getJobsAmount() const {
    return jobsVec.size();
}

void JobsList::printOnQuit() {
    for(JobEntry* job: jobsVec)
    {
        cout << job->processId << ": " << job->command << endl;
    }
}

void JobsList::removeJobByPID(int pid)
{
    for(auto job = jobsVec.begin(); job< jobsVec.end(); job++)
    {
        if((*job)->processId == pid) {
            jobsVec.erase(job);
        }
    }
    updateMaxId();
}

void JobsList::updateMaxId() {
    int maxJobId = 1;
    for(JobEntry* jobEntry : jobsVec)
    {
        if(jobEntry->jobId >= maxJobId)
        {
            maxJobId = jobEntry->jobId + 1;
        }
    }
    this->maxJobIdAvailable = maxJobId;
}

JobsList::JobEntry::JobEntry(int job, std::string command, int pid, bool isStopped)
    : jobId(job), command(command), processId(pid), startTime(0), isStopped(isStopped) {
    this->startTime = time(NULL);
    if(this->startTime == -1) {
         perror("smash error: time failed");
         exit(-1);
    }
}

double JobsList::JobEntry::calculateTimeElapsed() const {
    return difftime(time(NULL), this->startTime);
}

JobsCommand::JobsCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), jobsList(jobs)
{}

void JobsCommand::execute() {
    jobsList->printJobsList();
}

ForegroundCommand::ForegroundCommand(const char *cmd_line, JobsList *jobs)
    : BuiltInCommand(cmd_line), jobsPointer(jobs) {}

void ForegroundCommand::execute() {
    int jobId = 0;
    JobsList::JobEntry *pJobEntry;
    char *args[COMMAND_MAX_ARGS + 1] = {NULL};
    int argNum = _parseCommandLine(this->getCmdLine(), args);
    if (argNum > 2 || argNum < 1) {
        cerr << "smash error: fg: invalid arguments" << endl;
        releaseArgsArray(args);
        return;
    } else if (argNum == 2) {
        try {
            jobId = stoi(args[1]);
            pJobEntry = jobsPointer->getJobById(jobId);
            releaseArgsArray(args);
        } catch (...) {
            cerr << "smash error: fg: invalid arguments" << endl;
            return;
        }
        if (jobId <= 0) {
            cerr << "smash error: fg: job-id " << jobId << " does not exist" << endl;
            return;
        }
    } else {
        if (jobsPointer->isEmpty()) {
            cerr << "smash error: fg: jobs list is empty" << endl;
            return;
        } else {
            pJobEntry = jobsPointer->getLastJob(&jobId);
        }
    }

    if (pJobEntry == nullptr) {
        cerr << "smash error: fg: job-id " << jobId << " does not exist" << endl;
        return;
    }
    if (pJobEntry->isStopped) {
        int kill_res = kill(pJobEntry->processId, SIGCONT);
        if (kill_res == -1) {
            perror("smash error: kill failed");
            return;
        }
    }

    cout << pJobEntry->command << " : " << pJobEntry->processId << endl;
    int jobPid = pJobEntry->processId;
    string jobCommand = pJobEntry->command;
    jobsPointer->removeJobById(jobId);

    SmallShell &smash = SmallShell::getInstance();
    smash.setCurrentFgPid(jobPid);
    smash.setCurrentFgCommand(jobCommand);
    int waitPid_res = waitpid(jobPid, NULL, WUNTRACED);
    smash.setCurrentFgPid(-1);
    smash.setCurrentFgCommand("");
    if (waitPid_res == -1) {
        perror("smash error: waitpid failed");
        return;
    }

}

BackgroundCommand::BackgroundCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line),
            jobsPointer(jobs) {}

void BackgroundCommand::execute(){
    int jobId=0;
    JobsList::JobEntry* pJobEntry;
    char *args[COMMAND_MAX_ARGS+1]= {NULL};
    int argNum = _parseCommandLine(this->getCmdLine(), args);
    if (argNum > 2 || argNum < 1) {
        cerr << "smash error: bg: invalid arguments" << endl;
        releaseArgsArray(args);
        return;
    }
    else if(argNum==2){
        try {
            jobId = stoi(args[1]);
            pJobEntry= jobsPointer->getJobById(jobId);
            releaseArgsArray(args);
        } catch (...)
        {
            cerr << "smash error: bg: invalid arguments" << endl;
            return;
        }
        if(jobId <= 0 || pJobEntry == nullptr)
        {
            cerr << "smash error: bg: job-id "<< jobId <<" does not exist" << endl;
            return;
        }
        if(!pJobEntry->isStopped)
        {
            cerr << "smash error: bg: job-id "<< jobId <<" is already running in the background" << endl;
            return;
        }
    }
    else
    {
        pJobEntry = jobsPointer->getLastStoppedJob(&jobId);
        if(pJobEntry == nullptr)
        {
            cerr << "smash error: bg: there is no stopped jobs to resume" << endl;
            return;
        }
    }

    cout << pJobEntry->command << " : " << pJobEntry->processId << endl;
    int kill_res = kill(pJobEntry->processId, SIGCONT);
    if (kill_res == -1)
    {
        perror("smash error: kill failed");
        return;
    }
    pJobEntry->isStopped = false;
}

QuitCommand::QuitCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line),
                                                                 jobsPointer(jobs) {}

void QuitCommand::execute()
{
    char *args[COMMAND_MAX_ARGS+1]= {NULL};
    int argNum = _parseCommandLine(this->getCmdLine(), args);
    if ((argNum >= 2) && strcmp(args[1], "kill") == 0)
    {
        cout << "smash: sending SIGKILL signal to "<< this->jobsPointer->getJobsAmount() << " jobs:" << endl;
        jobsPointer->printOnQuit();
        jobsPointer->killAllJobs();
    }
    releaseArgsArray(args);
    exit(0);
}

KillCommand::KillCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line),
                                                                 jobsPointer(jobs) {}

void KillCommand::execute()
{
    char *args[COMMAND_MAX_ARGS+1]= {NULL};
    int argNum = _parseCommandLine(this->getCmdLine(), args);
    char* killSignalArray = args[1];
    char* jobIdArray = args[2];

    if(argNum != 3){
        cerr << "smash error: kill: invalid arguments" << endl;
        releaseArgsArray(args);
        return;
    }

    if(killSignalArray == NULL || killSignalArray[0] != '-'){
        cerr << "smash error: kill: invalid arguments" << endl;
        releaseArgsArray(args);
        return;
    }

    int jobId = -1;
    int sigNum = -1;
    try{
        sigNum = stoi(string(killSignalArray).substr(1, string(killSignalArray).length()-1));
        jobId = stoi(jobIdArray);
        releaseArgsArray(args);
    }
    catch(...){
        cerr << "smash error: kill: invalid arguments" << endl;
        releaseArgsArray(args);
        return;
    }

    if(sigNum < 1 || sigNum > 31)
    {
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }

    JobsList::JobEntry * pJobEntry= jobsPointer->getJobById(jobId);
    if(pJobEntry == nullptr) {
        cerr << "smash error: kill: job-id " << jobId << " does not exist" << endl;
        return;
    }
    int killResultCode = kill(pJobEntry->processId, sigNum);
    if(killResultCode == -1)
    {
        perror("smash error: kill failed");
        return;
    }

    if(sigNum == SIGSTOP)
    {
        pJobEntry->isStopped = true;
    }
    else if(sigNum == SIGCONT)
    {
        pJobEntry->isStopped = false;
    }

    cout << "signal number " << sigNum << " was sent to pid " << pJobEntry->processId << endl;
}

ExternalCommand::ExternalCommand(const char *cmd_line) : Command(cmd_line),
        isComplexCommand(false), isBackgroundCommand(false)
{
    isComplexCommand = isComplex();
    string cmd_s = _trim(string(cmd_line));
    if(cmd_s[cmd_s.length() - 1] == '&')
        isBackgroundCommand = true;
}

bool ExternalCommand::isComplex() const {
    return ((string(this->getCmdLine()).find('*') != string::npos) ||
            (string(this->getCmdLine()).find('?') != string::npos));
}

void ExternalCommand::execute() {
    string strCmdline = string(getCmdLine());
    char* cmdline= new char[strCmdline.length() + 1];
    strcpy(cmdline, strCmdline.c_str());

    if(isBackgroundCommand){
        _removeBackgroundSign(cmdline);
    }
    char* argv[COMMAND_MAX_ARGS+1] = {NULL};
    int argNum = _parseCommandLine(cmdline, argv);
    if(argNum < 1)
    {
        cerr << "smash error:> \""<<getCmdLine()<<"\"" << endl;
        return;
    }

    int childPid = fork();
    if(childPid==-1)
    {
        perror("smash error: fork failed");
        return;
    }
    else if(childPid == 0) //child
    {
        if (setpgrp() == -1) {
            perror("smash error: setpgrp failed");
            exit(-1);
        }
//        int innerChildPid = getpid();
//        SmallShell &smash = SmallShell::getInstance();
//        if (isBackgroundCommand) {
//            smash.getJobsList()->addJob(this, innerChildPid, false);
//        }

        if (this->isComplexCommand) {
            char *cFlag = (char *) "-c";
            char *path = (char *) "/bin/bash";
            char *const argComplex[] = {path, cFlag, cmdline, NULL};
            int execv_res = execv("/bin/bash", argComplex);
            if (execv_res == -1) {
//                if (isBackgroundCommand) {
//                    smash.getJobsList()->removeJobByPID(innerChildPid);
//                }
                perror("smash error: execv failed");
                exit(-1);
            }
        } else
        {

            int execv_res = execvp(argv[0], argv);
            if(execv_res == -1){
//                if(isBackgroundCommand){
//                    SmallShell& smash = SmallShell::getInstance();
//                    smash.getJobsList()->removeJobByPID(innerChildPid);
//
//                }
                perror("smash error: execvp failed");
                exit(-1);
            }
        }

    }
    else //father
    {
        if(isBackgroundCommand)
        {
            SmallShell& smash = SmallShell::getInstance();
            smash.getJobsList()->addJob(this, childPid, false);
        }
        else
        {
            SmallShell& smash = SmallShell::getInstance();
            smash.setCurrentFgPid(childPid);
            smash.setCurrentFgCommand(this->getCmdLine());
            int resultChild = waitpid(childPid, NULL, WUNTRACED);

            smash.setCurrentFgPid(-1);
            smash.setCurrentFgCommand("");
            if(resultChild == -1){
                perror("smash error: waitpid failed");
                return;
            }
        }
    }
}

RedirectionCommand::RedirectionCommand(const char *cmd_line)
    : Command(cmd_line), fileName(), isAppend(false), subCommand(), stdoutTemp(-1), isFailed(false) {

    this->isAppend = RedirectionCommand::isAppendOperator(cmd_line);

    string cmdString = string(cmd_line);
    int placeIndex = cmdString.find('>');

    this->subCommand = cmdString.substr(0, placeIndex);
    this->subCommand = _trim(subCommand);
    if(subCommand[subCommand.size() - 1] == '&')
    {
        this->subCommand = cmdString.substr(0, subCommand.size() - 1);
        this->subCommand = _trim(subCommand);
    }

    if(isAppend){
        this->fileName = cmdString.substr(placeIndex + 2);
    }
    else{
        this->fileName = cmdString.substr(placeIndex + 1);
    }
    this->fileName = _trim(fileName);

}

bool RedirectionCommand::isRedirection(const char *cmd_line) {
    string cmdString = string(cmd_line);
    return isAppendOperator(cmd_line) || (cmdString.find('>') != string::npos);
}

bool RedirectionCommand::isAppendOperator(const char *cmd_line) {
    string cmdString = string(cmd_line);
    return (cmdString.find(">>") != string::npos);
}

void RedirectionCommand::prepare()
{
    if(fileName.empty())
    {
        isFailed = true;
        cerr << "smash error:> \"" << this->getCmdLine() << "\"" << endl;
        return;
    }

    stdoutTemp = dup(1);
    if(stdoutTemp == -1){
        isFailed = true;
        perror("smash error: dup failed");
        return;
    }

    int resultClose = close(1);
    if(resultClose < 0) {
        this->isFailed = true;
        perror("smash error: close failed");
        return;
    }

    int outputFd;
    if(this->isAppend){
        outputFd = open(this->fileName.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0655);
    }
    else{
        outputFd = open(this->fileName.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0655);
    }
    if(outputFd < 0)
    {
        this->isFailed = true;
        perror("smash error: open failed");
        return;
    }
}

void RedirectionCommand::execute()
{
    this->prepare();
    if(!this->isFailed)
    {
        SmallShell& smash = SmallShell::getInstance();
        smash.executeCommand(subCommand.c_str());
    }
    this->cleanup();


//    int pid = fork();
//    if(pid == -1)
//    {
//        perror("smash error: fork failed");
//        return;
//    }
//    else if(pid == 0) // child
//    {
//        if(setpgrp() == -1){
//            perror("smash error: setpgrp failed");
//            exit(-1);
//        }
//        SmallShell& smash = SmallShell::getInstance();
//        this->prepare();
//        smash.executeCommand(subCommand.c_str());
//        close(1);
//        exit(1);
//    }
//    else //father
//    {
//        SmallShell& smash = SmallShell::getInstance();
//        smash.setCurrentFgPid(pid);
//        smash.setCurrentFgCommand(this->getCmdLine());
//        int resultChild = waitpid(pid, NULL, WUNTRACED);
//        smash.setCurrentFgPid(-1);
//        smash.setCurrentFgCommand("");
//        if(resultChild == -1){
//            perror("smash error: waitpid failed");
//            return;
//        }
//    }
}

void RedirectionCommand::cleanup() {

    if(!isFailed){
        if(close(1) == -1)
        {
            perror("smash error: close failed");
            return;
        }
    }
    if(stdoutTemp == -1){
        return;
    }
    if(dup2(stdoutTemp, 1) == -1)
    {
        perror("smash error: dup2 failed");
        return;
    }
    if(close(stdoutTemp) == -1) {
        perror("smash error: close failed");
        return;
    }
}


PipeCommand::PipeCommand(const char *cmd_line) : Command(cmd_line)
{
    this->isErrorPipe = PipeCommand::isErrorPipeType(cmd_line);

    string cmdString = string(cmd_line);
    int placeIndex = cmdString.find('|');

    this->command1 = cmdString.substr(0, placeIndex);
    this->command1 = _trim(command1);
    if(command1[command1.size() - 1] == '&')
    {
        this->command1 = cmdString.substr(0, command1.size() - 1);
        this->command1 = _trim(command1);
    }
    if(isErrorPipe){
        this->command2 = cmdString.substr(placeIndex + 2);
    }
    else{
        this->command2 = cmdString.substr(placeIndex + 1);
    }
    this->command2 = _trim(command2);
    if(command2[command2.size() - 1] == '&')
    {
        this->command2 = cmdString.substr(0, command2.size() - 1);
        this->command2 = _trim(command2);
    }
}

void PipeCommand::execute()
{

    int fd[2];
    if(pipe(fd) != 0)
    {
        perror("smash error: pipe failed");
        exit(-1);
    }
    int pid1 = fork();
    if(pid1==-1){
        perror("smash error: fork failed");
        return;
    }
    else if(pid1==0){
        if(setpgrp() == -1){
            perror("smash error: setpgrp failed");
            exit(-1);
        }
        if (close(fd[1])==-1){
            perror("smash error: close failed");
            exit(-1);
        }

        int dupResult = dup2(fd[0], 0);
        if(dupResult == -1){
            perror("smash error: dup2 failed");
            exit(-1);
        }
        if (close(fd[0])==-1){
            perror("smash error: close failed");
            exit(-1);
        }
        SmallShell& smash = SmallShell::getInstance();
        smash.executeCommand(command2.c_str());
        exit(1);
    }
    int pid2 = fork();
    if(pid2==-1){
        perror("smash error: fork failed");
        exit(-1);
    }
    else if(pid2 == 0) //command1, writing
    {
        if(setpgrp() == -1){
            perror("smash error: setpgrp failed");
            exit(-1);
        }
        if(close(fd[0]) == -1)
        {
            perror("smash error: close failed");
            exit(-1);
        }

        int dupResult;
        if(this->isErrorPipe)
        {
            dupResult = dup2(fd[1], 2);
        }
        else
        {
            dupResult = dup2(fd[1], 1);
        }
        if(dupResult == -1)
        {
            perror("smash error: dup2 failed");
            exit(-1);
        }

        if(close(fd[1]) == -1)
        {
            perror("smash error: close failed");
            exit(-1);
        }
        SmallShell& smash = SmallShell::getInstance();
        smash.executeCommand(command1.c_str());
        exit(1);
    }
    else
    {
        if(close(fd[1]) == -1 || close(fd[0])== -1)
        {
            perror("smash error: close failed");
            return;
        }
        if(waitpid(pid1, NULL, 0) == -1 || waitpid(pid2, NULL, 0) == -1) {
            perror("smash error: waitpid failed");
            return;
        }
    }
}
//
//void PipeCommand::execute2()
//{
//    int pid1 = fork();
//    if(pid1==-1){
//        perror("smash error: fork failed");
//        return;
//    }
//    else if(pid1==0)
//    {
//        int fd[2];
//        if(pipe(fd) != 0)
//        {
//            perror("smash error: pipe failed");
//            exit(-1);
//        }
//        int pid2 = fork();
//        if(pid2==-1){
//            perror("smash error: fork failed");
//            exit(-1);
//        }
//
//        else if(pid2 == 0) //command1, writing
//        {
//            if(setpgrp() == -1){
//                perror("smash error: setpgrp failed");
//                exit(-1);
//            }
//            if(close(fd[0]) == -1)
//            {
//                perror("smash error: close failed");
//                exit(-1);
//            }
//
//            int dupResult;
//            if(this->isErrorPipe)
//            {
//                dupResult = dup2(fd[1], 2);
//            }
//            else
//            {
//                dupResult = dup2(fd[1], 1);
//            }
//            if(dupResult == -1)
//            {
//                perror("smash error: dup2 failed");
//                exit(-1);
//            }
//
//            if(close(fd[1]) == -1)
//            {
//                perror("smash error: close failed");
//                exit(-1);
//            }
//            SmallShell& smash = SmallShell::getInstance();
//            smash.executeCommand(command1.c_str());
//            exit(1);
//        }
//        else //command2, father of command1, reading
//        {
//            if(setpgrp() == -1){
//                perror("smash error: setpgrp failed");
//                exit(-1);
//            }
//            if (close(fd[1])==-1){
//                perror("smash error: close failed");
//                exit(-1);
//            }
//
//            int dupResult = dup2(fd[0], 0);
//            if(dupResult == -1){
//                perror("smash error: dup2 failed");
//                exit(-1);
//            }
//            if (close(fd[0])==-1){
//                perror("smash error: close failed");
//                exit(-1);
//            }
//            SmallShell& smash = SmallShell::getInstance();
//            smash.setCurrentFgPid(pid2);
//            int resultChild = waitpid(pid2, NULL, WUNTRACED);
//            smash.setCurrentFgPid(-1);
//            if(resultChild == -1){
//                perror("smash error: waitpid failed");
//                exit(-1);
//            }
//            smash.executeCommand(command2.c_str());
//            exit(1);
//        }
//    }
//    else //father
//    {
//
//        SmallShell &smash = SmallShell::getInstance();
//        smash.setCurrentFgPid(pid1);
//        int resultChild = waitpid(pid1, NULL, WUNTRACED);
//        smash.setCurrentFgPid(-1);
//        if(resultChild == -1){
//            perror("smash error: waitpid failed");
//            return;
//        }
//    }
//
//
//
//
//    int fd[2];
//    if(pipe(fd) != 0)
//    {
//        perror("smash error: pipe failed");
//        exit(-1);
//    }
//
//    int pid1 = fork();
//    if(pid1==-1){
//        perror("smash error: fork failed");
//        return;
//    }
//    else if(pid1 == 0)
//    {
//        if(setpgrp() == -1){
//            perror("smash error: setpgrp failed");
//            exit(-1);
//        }
//        if(close(fd[0]) == -1)
//        {
//            perror("smash error: close failed");
//            exit(-1);
//        }
//
//        int dupResult;
//        if(this->isErrorPipe)
//        {
//            dupResult = dup2(fd[1], 2);
//        }
//        else
//        {
//            dupResult = dup2(fd[1], 1);
//        }
//        if(dupResult == -1)
//        {
//            perror("smash error: dup2 failed");
//            exit(-1);
//        }
//
//        if(close(fd[1]) == -1)
//        {
//            perror("smash error: close failed");
//            exit(-1);
//        }
//        SmallShell& smash = SmallShell::getInstance();
//        smash.executeCommand(command1.c_str());
//        exit(1);
//    }
//    else
//    {
//        int pid2 = fork();
//        if(pid2==-1)
//        {
//            perror("smash error: fork failed");
//            exit(-1);
//        }
//        else if(pid2 == 0)
//        {
//
//        }
//        else
//        {
//            if()
//        }
//    }
//

//}

bool PipeCommand::isPipeCommand(const char* cmd_line) {
    return (string(cmd_line).find('|') != string::npos);
}

bool PipeCommand::isErrorPipeType(const char* cmd_line) {
    return (string(cmd_line).find("|&") != string::npos);
}

SetcoreCommand::SetcoreCommand(const char *cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line), jobsPointer(jobs) {

}

void SetcoreCommand::execute() {
    char *args[COMMAND_MAX_ARGS+1];
    for (int i = 0; i <= COMMAND_MAX_ARGS; i++) {
        args[i] = NULL;
    }
    int argNum = _parseCommandLine(this->getCmdLine(), args);
    if (argNum != 3)
    {
        cerr << "smash error: setcore: invalid arguments" << endl;
        return;
    }
    int jobId, coreId;
    try
    {
        jobId = stoi(args[1]);
        coreId = stoi(args[2]);
    }catch(...){
        cerr << "smash error: setcore: invalid arguments" << endl;
        return;
    }
    JobsList::JobEntry* currentJob = jobsPointer->getJobById(jobId);
    if(currentJob == nullptr)
    {
        cerr << "smash error: setcore: job-id "<< jobId << " does not exist" << endl;
        return;
    }

    cpu_set_t mask;
//    CPU_ZERO(&mask);
//    int coresNum = CPU_COUNT(&mask);
    //int num_cpus = std::thread::hardware_concurrency()
    long coresNum = sysconf(_SC_NPROCESSORS_ONLN);
    if(coresNum == -1)
    {
        perror("smash error: sched_setaffinity failed");
        return;
    }
    if(coreId >= coresNum){
        cerr << "smash error: setcore: invalid core number" << endl;
        return;
    }

    CPU_ZERO(&mask);
    CPU_SET(coreId, &mask);
    int result = sched_setaffinity(currentJob->processId, sizeof(mask), &mask);
    if(result == -1){
        perror("smash error: sched_setaffinity failed");
        return;
    }
}

GetFileTypeCommand::GetFileTypeCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}

void GetFileTypeCommand::execute()
{
    char *args[COMMAND_MAX_ARGS+1] = {NULL};
    int argNum = _parseCommandLine(this->getCmdLine(), args);
    if(argNum!=2) {
        cerr << "smash error: gettype: invalid arguments" << endl;
        releaseArgsArray(args);
        return;
    }
    else
    {
        string filename = string(args[1]);
        struct stat fileStat;

        if (stat(filename.c_str(), &fileStat) == -1) {
            perror("smash error: stat failed");
            return;
        }
        string fileType="";
         if (S_ISREG(fileStat.st_mode))
             fileType="regular file";
        else if (S_ISDIR(fileStat.st_mode))
             fileType="directory";
        else if (S_ISCHR(fileStat.st_mode))
             fileType="character device";
        else if (S_ISBLK(fileStat.st_mode))
             fileType = "block device";
        else if (S_ISFIFO(fileStat.st_mode))
            fileType = "FIFO";
        else if (S_ISLNK(fileStat.st_mode))
            fileType = "symbolic link";
        else if (S_ISSOCK(fileStat.st_mode))
            fileType = "socket";

        long fileSize = fileStat.st_size;

        cout << filename << "\'s type is \""<< fileType << "\" and takes up " << fileSize << " bytes" << endl;

    releaseArgsArray(args);

    }
}

ChmodCommand::ChmodCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {

}

void ChmodCommand::execute() {
    char *args[COMMAND_MAX_ARGS+1] = {NULL};
    int argNum = _parseCommandLine(this->getCmdLine(), args);
    if (argNum != 3) {
        cerr << "smash error: gettype: invalid arguments" << endl;
        releaseArgsArray(args);
        return;
    }
    string newModeStr = args[1];
    string pathToFile = args[2];
    releaseArgsArray(args);

    const char* modeChar = newModeStr.c_str();
    char* endptr;
    long int newMode = strtol(modeChar, &endptr, 8);

    if (endptr == modeChar || errno == EINVAL || *endptr != '\0')
    {
        cerr << "smash error: chmod: invalid arguments" << endl;
        return;
    }

    if(chmod(pathToFile.c_str(), newMode) == -1)
    {
        perror("smash error: chmod failed");
        return;
    }
}

TimeoutCommand::TimeoutCommand(const char* cmd_line) : BuiltInCommand(cmd_line){}

void TimeoutCommand::execute() {
    char *args[COMMAND_MAX_ARGS+1] = {NULL};
    int argNum = _parseCommandLine(this->getCmdLine(), args);
    if(argNum < 3)
    {
        cerr << "smash error: timeout: invalid arguments" << endl;
        return;
    }
    string durationStr = args[1];
    string command = args[2];

    for(int i=3; i< argNum; i++)
    {
        string tempString = args[i];
        command += " " + tempString;
    }
    releaseArgsArray(args);
    try{
        this->duration = stoi(durationStr);
        if(duration <= 0){
            cerr << "smash error: timeout: invalid arguments" << endl;
            return;
        }
    } catch(...)
    {
        return;
    }
    this->startTime = time(NULL);
    this->expectedEnd = this->startTime + this->duration;

    SmallShell& smash = SmallShell::getInstance();
    int currentAlarm = alarm(this->duration);
    if(currentAlarm != 0 && currentAlarm < this->duration)
    {
        alarm(currentAlarm);
    }
    int childPid = fork();
    if(childPid == -1){
        perror("smash error: fork failed");
        return;
    }
    if(childPid == 0){
        if(setpgrp() == -1){
            perror("smash error: setpgrp failed");
            exit(-1);
        }
        this->processId = getpid();
        if(command[command.size()-1] == '&')
            command = command.substr(0, command.size()-1);
        smash.executeCommand(command.c_str());
        exit(1);
    }
    else //father
    {
        int currentAlarm = alarm(this->duration);
        this->processId = childPid;
        char* copy_str = new char[strlen(this->getCmdLine())]; // Allocate memory for the copy
        strcpy(copy_str, this->getCmdLine());

        smash.insertTimeoutCommand(
                new TimeoutCommand(copy_str, childPid, this->startTime,
                                   this->expectedEnd, this->duration));
        if(command[command.size()-1] != '&')
        {
            smash.setCurrentFgPid(childPid);
            smash.setCurrentFgCommand(this->getCmdLine());
            int waitPid_res = waitpid(childPid, NULL, WUNTRACED);
            smash.setCurrentFgPid(-1);
            smash.setCurrentFgCommand("");
            smash.removeTimeoutByPid(childPid);
            smash.scheduleTimeoutAlarm();
            if (waitPid_res == -1) {
                perror("smash error: waitpid failed");
                return;
            }
        }
        else
        {
            smash.getJobsList()->addJob(this, childPid, false);
        }
    }
}

int TimeoutCommand::getExpectedEnd() const{
    return expectedEnd;
}

int TimeoutCommand::getProcessId() const{
    return this->processId;
}
void TimeoutCommand::setExpectedEnd(int expectedEnd) {
    this->expectedEnd = expectedEnd;
}

TimeoutCommand::TimeoutCommand(const char* cmd_line, int pid, int start, int end, int duration)
    : BuiltInCommand(cmd_line), processId(pid), startTime(start), expectedEnd(end), duration(duration) {}



bool CompareTimeout::operator()(TimeoutCommand* t1, TimeoutCommand* t2)
{
    return t1->getExpectedEnd() > t2->getExpectedEnd();
}