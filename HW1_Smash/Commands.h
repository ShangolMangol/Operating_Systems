#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <string>
#include <queue>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

class Command {
private:
    const char* cmd_line;
public:
    Command(const char* cmd_line);
    virtual ~Command() = default;
    virtual void execute() = 0;
  virtual void prepare();
  virtual void cleanup();
  // TODO: Add your extra methods if needed
    const char *getCmdLine() const;
    std::string getCommandType() const;

    void setCmdLine(const char *cmdLine);
};

class BuiltInCommand : public Command {
 public:
  BuiltInCommand(const char* cmd_line);
  virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
private:
    bool isComplexCommand;
    bool isBackgroundCommand;
public:
  ExternalCommand(const char* cmd_line);
  virtual ~ExternalCommand() {}
  void execute() override;
  bool isComplex() const;
  bool isBeckGround() const;
};

class PipeCommand : public Command {
  std::string command1;
  std::string command2;
  bool isErrorPipe;
 public:
  PipeCommand(const char* cmd_line);
  virtual ~PipeCommand() {}
  void execute() override;

    static bool isPipeCommand(const char* cmd_line);
    static bool isErrorPipeType(const char* cmd_line);
};

class RedirectionCommand : public Command {
    std::string fileName;
    bool isAppend;
    std::string subCommand;
    int stdoutTemp;
    bool isFailed;
 public:
  explicit RedirectionCommand(const char* cmd_line);
  virtual ~RedirectionCommand() {}
  void execute() override;
  void prepare() override;
  void cleanup() override;
  static bool isRedirection(const char* cmd_line);
  static bool isAppendOperator(const char* cmd_line);
};

class ChangePromptCommand : public BuiltInCommand {
private:
    std::string secondWord;
public:
    ChangePromptCommand(std::string cmd_s);
    virtual ~ChangePromptCommand() = default;
    void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
// TODO: Add your data members public:
public:
    char** dirLastPwd;
    ChangeDirCommand(const char* cmd_line, char** plastPwd);
    virtual ~ChangeDirCommand() {}
    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
 public:
  GetCurrDirCommand(const char* cmd_line);
  virtual ~GetCurrDirCommand() {}
  void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
 public:
  ShowPidCommand(const char* cmd_line);
  virtual ~ShowPidCommand() {}
  void execute() override;
};

class JobsList;

class JobsList {
public:
    class JobEntry {
    public:
        int jobId;
        std::string command;
        int processId;
        time_t startTime;
        bool isStopped;

        JobEntry(int job, std::string command, int pid, bool isStopped);
        double calculateTimeElapsed() const;
    };

    int maxJobIdAvailable;

    int getMaxJobIdAvailable() const;

    std::vector<JobEntry*> jobsVec;
    std::vector<JobEntry*> allJobs;
public:
    JobsList();
    ~JobsList();
    void addJob(Command* cmd, int pid, bool isStopped = false);
    void addExistingJob(std::string cmd, int pid, bool isStopped);
    void printJobsList();
    void killAllJobs();
    void removeFinishedJobs();
    JobEntry * getJobById(int jobId);
    void removeJobById(int jobId);
    JobEntry * getLastJob(int* lastJobId);
    JobEntry *getLastStoppedJob(int *jobId);
    // TODO: Add extra methods or modify exisitng ones as needed
    bool isEmpty() const;
    int getJobsAmount() const;
    void printOnQuit();
    void removeJobByPID(int pid);
    void updateMaxId();
};

class JobsCommand : public BuiltInCommand {
 // TODO: Add your data members
 JobsList* jobsList;
 public:
  JobsCommand(const char* cmd_line, JobsList* jobs);
  virtual ~JobsCommand() {}
  void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
private:
    JobsList* jobsPointer;
public:
  ForegroundCommand(const char* cmd_line, JobsList* jobs);
  virtual ~ForegroundCommand() {}
  void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
private:
    JobsList* jobsPointer;
 public:
  BackgroundCommand(const char* cmd_line, JobsList* jobs);
  virtual ~BackgroundCommand() {}
  void execute() override;
};

class QuitCommand : public BuiltInCommand {
private:
    JobsList* jobsPointer;
public:
    QuitCommand(const char* cmd_line, JobsList* jobs);
    virtual ~QuitCommand() {}
    void execute() override;
};

class KillCommand : public BuiltInCommand {
private:
    JobsList* jobsPointer;
public:
    KillCommand(const char* cmd_line, JobsList* jobs);
    virtual ~KillCommand() {}
    void execute() override;
};



class TimeoutCommand : public BuiltInCommand {
/* Bonus */
private:
    int processId;
    int startTime;
    int expectedEnd;
    int duration;
public:
    int getExpectedEnd() const;
    int getProcessId() const;
    void setExpectedEnd(int expectedEnd);

public:
  explicit TimeoutCommand(const char* cmd_line);
    explicit TimeoutCommand(const char* cmd_line, int pid, int start, int end, int duration);
    virtual ~TimeoutCommand() {}
  void execute() override;
};

class CompareTimeout {
public:
    bool operator() (TimeoutCommand* t1, TimeoutCommand* t2);
};

class ChmodCommand : public BuiltInCommand {
public:
  explicit ChmodCommand(const char* cmd_line);
  virtual ~ChmodCommand() {}
  void execute() override;
};

class GetFileTypeCommand : public BuiltInCommand {
public:
  GetFileTypeCommand(const char* cmd_line);
  virtual ~GetFileTypeCommand() {}
  void execute() override;
};

class SetcoreCommand : public BuiltInCommand {
    JobsList* jobsPointer;

public:
  SetcoreCommand(const char* cmd_line, JobsList* jobs);
  virtual ~SetcoreCommand() {}
  void execute() override;
};


class SmallShell {
 private:
  // TODO: Add your data members
  SmallShell();

  std::string promptStr;
  char* lastPwd;
  JobsList jobsList;
  static int smashPid;
  int currentFgPid;
  std::string currentFgCommand;
  std::priority_queue<TimeoutCommand*, std::vector<TimeoutCommand*>, CompareTimeout> timeoutQueue;
public:
    std::priority_queue<TimeoutCommand *, std::vector<TimeoutCommand *>, CompareTimeout> &getTimeoutQueue();
    void insertTimeoutCommand(TimeoutCommand* cmd);
    bool isTimeoutQueueEmpty() const;
    TimeoutCommand* topTimeoutCommand();
    void popTimeoutCommand();
    void removeTimeoutByPid(int pid);
    void scheduleTimeoutAlarm();
    int getCurrentFgPid() const;
    void setCurrentFgPid(int currentFgPid);
    Command *CreateCommand(const char* cmd_line);
    SmallShell(SmallShell const&)      = delete; // disable copy ctor
    void operator=(SmallShell const&)  = delete; // disable = operator
    static SmallShell& getInstance() // make SmallShell singleton
  {
    static SmallShell instance; // Guaranteed to be destroyed.

    return instance;
  }

  ~SmallShell();
  void executeCommand(const char* cmd_line);

  std::string getPromptStr();
  void setPromptStr(const std::string newPromptStr);

  char** getLastPwd();
  void setLastPwd(char* lastPwd);

  JobsList* getJobsList();
  // TODO: add extra methods as needed
  int getSmashPid();

  std::string getCurrentFgCommand() const;
  void setCurrentFgCommand(std::string newFgCommand);
};

#endif //SMASH_COMMAND_H_
