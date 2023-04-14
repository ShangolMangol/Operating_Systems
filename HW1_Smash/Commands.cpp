#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <algorithm>

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

SmallShell::SmallShell() : promptStr("smash"), lastPwd(nullptr), jobsList(), smashPid(getpid()){

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
  string cmdLineCopy = cmd_line;
  cmdLineCopy.erase(remove(cmdLineCopy.begin(), cmdLineCopy.end(), '&'), cmdLineCopy.end());
  string firstWordBuiltIn = cmdLineCopy.substr(0, cmdLineCopy.find_first_of(" \n"));

  SmallShell& smash = SmallShell::getInstance();

  if (firstWordBuiltIn.compare("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWordBuiltIn.compare("showpid") == 0) {
    return new ShowPidCommand(cmd_line);
  }
  else if (firstWordBuiltIn.compare("chprompt") == 0){
      return new ChangePromptCommand(cmd_s);
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

//  .....
  else {
//    return new ExternalCommand(cmd_line);
  }
  return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
  // TODO: Add your implementation here
  // for example:
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

ChangePromptCommand::ChangePromptCommand(std::string cmd_s) : BuiltInCommand(cmd_s.c_str())
{
    string command = "chprompt";
    if(cmd_s.length() >= command.length()+1)
        this->secondWord = cmd_s.substr(command.length()+1, cmd_s.find_first_of(" \n"));
    else
        this->secondWord = "";
}

void ChangePromptCommand::execute()
{
    SmallShell& smash = SmallShell::getInstance();
    secondWord = _trim(secondWord);
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

ShowPidCommand::ShowPidCommand(const char* cmd_line) : BuiltInCommand(cmd_line){}

void ShowPidCommand::execute(){
    SmallShell& smash = SmallShell::getInstance();
    cout << "smash pid is " << smash.getSmashPid() << "\n";
}

GetCurrDirCommand::GetCurrDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {
}

void GetCurrDirCommand::execute() {
    char cwd[COMMAND_ARGS_MAX_LENGTH];
    if(getcwd(cwd, sizeof(cwd)) == NULL)
        perror("smash error: getcwd failed");
    else
        cout << cwd << "\n";
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
        cerr << "smash error: cd: too many arguments\n";
    } else if (argNum < 2) {
        cerr << "smash error:> \"" << this->getCmdLine() << "\"\n";
    } else {
        string path = args[1];
        releaseArgsArray(args);
        char *cwd = new char[COMMAND_ARGS_MAX_LENGTH];

        if (getcwd(cwd, sizeof(char) * COMMAND_ARGS_MAX_LENGTH) == NULL) {
            perror("smash error: getcwd failed");
            return;
        }
        if (path.compare("-") == 0) {
            if (dirLastPwd == nullptr) {
                cerr << "smash error: cd: OLDPWD not set\n";
                return;
            } else
                path = *dirLastPwd;
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
void JobsList::addJob(Command* cmd, bool isStopped){
    this->removeFinishedJobs();
    JobEntry* newJob = new JobEntry(this->maxJobIdAvailable,
                                   cmd->getCmdLine(), getpid(), isStopped);
    maxJobIdAvailable++;
    jobsVec.push_back(newJob);
}

bool compareJobs(JobsList::JobEntry *job1, JobsList::JobEntry *job2) {
    return job1->jobId < job2->jobId;
}

void JobsList::printJobsList() {
    this->removeFinishedJobs();
    std::sort(jobsVec.begin(), jobsVec.end(), compareJobs);
    for (JobEntry* job : jobsVec) {
        cout << "[" << job->jobId << "]" << job->command << " : " << job->processId
             << job->calculateTimeElapsed() << " secs";
        if(job->isStopped)
            cout << " (stopped)";
        cout << "\n";
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
    jobsVec.clear();
    maxJobIdAvailable = 1;
    removeFinishedJobs();
}

void JobsList::removeFinishedJobs()
{
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


    for(int finishedId : jobsToDelete)
    {
        removeJobById(finishedId);
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
            delete *job;
            jobsVec.erase(job);
        }
    }

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
        cout << job->processId << ": " << job->command << "\n";
    }
}

JobsList::JobEntry::JobEntry(int job, std::string command, int pid, bool isStopped)
    : jobId(job), command(command), processId(pid), startTime(time(NULL)), isStopped(isStopped) {

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
    int jobId=0;
    JobsList::JobEntry* pJobEntry;
    char *args[COMMAND_MAX_ARGS+1]= {NULL};
    int argNum = _parseCommandLine(this->getCmdLine(), args);
    if (argNum > 2 || argNum < 1) {
        cerr << "smash error: fg: invalid arguments\n";
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
            cerr << "smash error: fg: invalid arguments\n";
            return;
        }
        if(jobId <= 0)
        {
            cerr << "smash error: fg: job-id "<< jobId <<" does not exist\n";
            return;
        }
    }
    else
    {
        if(jobsPointer->isEmpty())
        {
            cerr << "smash error: fg: jobs list is empty\n";
            return;
        }
        else{
            pJobEntry = jobsPointer->getLastJob(&jobId);
        }
    }

    if(pJobEntry == nullptr) {
        cerr << "smash error: fg: job-id " << jobId << " does not exist\n";
        return;
    }
    if(pJobEntry->isStopped)
    {
        int kill_res = kill(pJobEntry->processId, SIGCONT);
        if (kill_res == -1)
        {
            perror("smash error: kill failed");
            return;
        }

        cout << pJobEntry->command << " : " << pJobEntry->processId;
        int jobPid = pJobEntry->processId;
        jobsPointer->removeJobById(jobId);
        int waitPid_res = waitpid(jobPid, NULL, 0);
        if (waitPid_res == -1)
        {
            perror("smash error: waitpid failed");
            return;
        }
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
        cerr << "smash error: bg: invalid arguments\n";
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
            cerr << "smash error: bg: invalid arguments\n";
            return;
        }
        if(jobId <= 0 || pJobEntry == nullptr)
        {
            cerr << "smash error: bg: job-id "<< jobId <<" does not exist\n";
            return;
        }
        if(!pJobEntry->isStopped)
        {
            cerr << "smash error: bg: job-id "<< jobId <<" is already running in the background\n";
        }
    }
    else
    {
        pJobEntry = jobsPointer->getLastStoppedJob(&jobId);
        if(pJobEntry == nullptr)
        {
            cerr << "smash error: bg: there is no stopped jobs to resume\n";
            return;
        }
    }

    cout << pJobEntry->command << " : " << pJobEntry->processId;
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
        cout << "smash: sending SIGKILL signal to "<< this->jobsPointer->getJobsAmount() << " jobs:\n";
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
        cerr << "smash error: kill: invalid arguments\n";
        releaseArgsArray(args);
        return;
    }

    if(killSignalArray == NULL || killSignalArray[0] != '-'){
        cerr << "smash error: kill: invalid arguments\n";
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
        cerr << "smash error: kill: invalid arguments\n";
        releaseArgsArray(args);
        return;
    }

    JobsList::JobEntry * pJobEntry= jobsPointer->getJobById(jobId);
    if(pJobEntry == nullptr) {
        cerr << "smash error: kill: job-id " << jobId << " does not exist\n";
        return;
    }
    int killResultCode = kill(pJobEntry->processId, sigNum);
    if(killResultCode == -1)
    {
        perror("smash error: kill failed");
        return;
    }

    cout << "signal number " << sigNum << "was sent to pid " << pJobEntry->processId;


}