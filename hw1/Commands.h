#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <list>
#include <iostream>
#include <cctype>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

class Command {
// TODO: Add your data members
protected:
    int numOfArgs;
    char* args[COMMAND_MAX_ARGS];

 public:
    std::string cmd_line;
    int pid;
    Command(const char* cmd_line);
  virtual ~Command();
  virtual void execute() = 0;
  //virtual void prepare();
  //virtual void cleanup();
  // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
 public:
  BuiltInCommand(const char* cmd_line);
  virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
private:
    const bool m_is_bg;
    const std::string m_cmd_line;
    char *m_no_back_sign;
    /*
    ----------timeout----------
    int timeout_duration;
    */
 public:
  ExternalCommand(const char* cmd_line);
  virtual ~ExternalCommand() {}
  void execute() override;
};

class PipeCommand : public Command {
  // TODO: Add your data members
  std::string m_cmd_line;
public:
  PipeCommand(const char* cmd_line);
  virtual ~PipeCommand() {}
  void execute() override;
};

class RedirectionCommand : public Command {
 // TODO: Add your data members
 std::string m_cmd_line;
 public:
  explicit RedirectionCommand(const char* cmd_line);
  virtual ~RedirectionCommand() {}
  void execute() override;
  //void prepare() override;
  //void cleanup() override;
};

class ChangeDirCommand : public BuiltInCommand {
// TODO: Add your data members public:
public:
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
class QuitCommand : public BuiltInCommand {
// TODO: Add your data members
public:
  QuitCommand(const char* cmd_line, JobsList* jobs);
  virtual ~QuitCommand() {}
  void execute() override;
};


class JobsList {
 public:
  class JobEntry {
   // TODO: Add your data members
  public:
      enum Status {finished, fg, bg};

  private:
       const int m_job_id;
       const int m_pid;
       const char* m_cmd_line;
       Status m_status;

  public:
      int getJobId() const;
      int getJobPid() const;
      const char *getCmdLine() const;
      Status getStatus() const;
      void setStatus(Status new_status);
      JobEntry(int job_id, int pid, const char* cmd_line, Status status);

  };
 // TODO: Add your data members
 private:
    std::list<JobEntry>* m_job_entry_list;
    int m_max_job_id;
    //static bool compareById(const JobEntry& job1, const JobEntry& job2);
 public:
  JobsList();
  ~JobsList();
  void addJob(Command* cmd, bool isStopped = false);
  void printJobsList();
  void removeFinishedJobs();
  void removeJobById(int jobId);
  int getMaxJobID ();
  std::list<JobEntry>* getJobsList();
  std::list<JobEntry>::iterator begin();
  std::list<JobEntry>::iterator end();
  int getSize();
  // TODO: Add extra methods or modify exisitng ones as needed
};

class JobsCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  JobsCommand(const char* cmd_line, JobsList* jobs);
  virtual ~JobsCommand() {}
  void execute() override;
};

class KillCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  KillCommand(const char* cmd_line, JobsList* jobs);
  virtual ~KillCommand() {}
  void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  ForegroundCommand(const char* cmd_line, JobsList* jobs);
  virtual ~ForegroundCommand() {}
  void execute() override;
};

class ChmodCommand : public BuiltInCommand {
 public:
  ChmodCommand(const char* cmd_line);
  virtual ~ChmodCommand() {}
  void execute() override;
};

class ChpromptCommand : public BuiltInCommand {
  private:
  std::string name;
 public:
  ChpromptCommand(const char* cmd_line);
  virtual ~ChpromptCommand() {}
  void execute() override;
};

class SmallShell {
 private:
  // TODO: Add your data members
  std::string m_name;
  const int m_pid;
  std::string  m_previous_path;
  std::string m_current_path;
  JobsList *m_jobs_list;

  SmallShell();
  void updateCurrentPath();
 public:
  Command *CreateCommand(const char* cmd_line);
  SmallShell(SmallShell const&)      = delete; // disable copy ctor
  void operator=(SmallShell const&)  = delete; // disable = operator
  static SmallShell& getInstance() // make SmallShell singleton
  {
    static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
  }
  ~SmallShell();
  void executeCommand(const char* cmd_line);
  // TODO: add extra methods as needed
  void setName(const std::string& new_name);
  std::string  getName();
  int getPid() const;
  void setCurrentPath(const std:: string& new_path);
  std::string getCurrentPath();
  void setPreviousPath(const std:: string& new_path);
  std::string getPreviousPath();
  JobsList::JobEntry& getJob ();
  JobsList::JobEntry& getJob (int jobID);
  JobsList* getJobsList();
  bool jobIdExist(int jobId);

};

#endif //SMASH_COMMAND_H_
