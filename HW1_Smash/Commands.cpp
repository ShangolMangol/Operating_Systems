#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"

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

SmallShell::SmallShell() : promptStr("smash"), lastPwd(nullptr){

}

SmallShell::~SmallShell() {
// TODO: add your implementation
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
	// For example:

  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

  SmallShell& smash = SmallShell::getInstance();

  if (firstWord.compare("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("showpid") == 0) {
    return new ShowPidCommand(cmd_line);
  }
  else if (firstWord.compare("chprompt") == 0){
      return new ChangePromptCommand(cmd_s);
  }
  else if (firstWord.compare("cd") == 0){
//      string lastPwd = smash.getLastPwd();
//      std::vector<char> cstr(lastPwd.c_str(), lastPwd.c_str() + lastPwd.size() + 1);
      return new ChangeDirCommand(cmd_line, smash.getLastPwd());
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
   Command* cmd = CreateCommand(cmd_line);
   cmd->execute();
   delete cmd;
  // Please note that you must fork smash process for some commands (e.g., external commands....)
}

string SmallShell::getPromptStr(){
    return this->promptStr;
}

void SmallShell::setPromptStr(const std::string newPromptStr){
    this->promptStr = newPromptStr;
}

char** SmallShell::getLastPwd() const {
    return lastPwd;
}

void SmallShell::setLastPwd(char** lastPwd) {
    SmallShell::lastPwd = lastPwd;
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

ShowPidCommand::ShowPidCommand(const char* cmd_line) : BuiltInCommand(cmd_line){}

void ShowPidCommand::execute(){
    cout << "smash pid is " << getpid() << "\n";
}

GetCurrDirCommand::GetCurrDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {
}

void GetCurrDirCommand::execute() {
    char cwd[256];
    if(getcwd(cwd, sizeof(cwd)) == NULL)
        perror("smash error: getcwd failed");
    else
        cout << cwd << "\n";
}

ChangeDirCommand::ChangeDirCommand(const char *cmd_line, char **plastPwd)
    : BuiltInCommand(cmd_line), lastPwd(plastPwd)
{

}

void ChangeDirCommand::execute() {
    char *args[25];
    for (int i = 0; i < 25; i++) {
        args[i] = NULL;
    }
    int argNum = _parseCommandLine(this->getCmdLine(), args);
    if (argNum > 2 ){
        cerr << "smash error: cd: too many arguments";
    }
    else if(argNum < 2) {
        cerr << "smash error:> \"" << this->getCmdLine() << "\"\n";
    }
    else
    {
        string path = args[1];

        char* cwd = new char[256];
        if(getcwd(cwd, sizeof(char)*256) == NULL) {
            perror("smash error: getcwd failed");
            return;
        }
        if(path.compare("-") == 0)
        {
            if(lastPwd == nullptr)
                cerr << "smash error: cd: OLDPWD not set";
            else
                path = *lastPwd;
        }

        //executing cd
        if (chdir(path.c_str())!=0){
            perror("smash error: chdir failed");
        }
        delete[] this->lastPwd;
        *(this->lastPwd) = cwd;
    }

}