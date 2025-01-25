#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <algorithm>
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

SmallShell::SmallShell() : m_name("smash"), m_pid(getpid()), m_jobs_list(new JobsList){
// TODO: add your implementation
    updateCurrentPath();
}

SmallShell::~SmallShell() {
// TODO: add your implementation
    delete m_jobs_list;
}

void SmallShell::setName(const string& new_name){
  m_name = new_name;
}

std::list<JobsList::JobEntry>* JobsList::getJobsList() {
    return m_job_entry_list;
};

int JobsList::getSize() {
    return m_job_entry_list->size();
}

int JobsList::getMaxJobID (){
    return m_max_job_id;
}

JobsList::JobEntry& SmallShell::getJob(int jobID) {
    // must be called after checking if exists!!
    for (JobsList::JobEntry& job : *SmallShell::getInstance().m_jobs_list->getJobsList()) {
        if (job.getJobId() == jobID) {
            return job;
        }
    }
    throw std::out_of_range("Job ID not found in the job list");
}

JobsList::JobEntry& SmallShell::getJob() {
    SmallShell::getInstance().getJobsList()->removeFinishedJobs();
    int max_job_id = SmallShell::getInstance().m_jobs_list->getMaxJobID();
    return getJob(max_job_id);
}

JobsList* SmallShell::getJobsList() {
    return m_jobs_list;
}

ChangeDirCommand::ChangeDirCommand(const char *cmd_line, char** plastPwd) : BuiltInCommand(cmd_line){}

JobsCommand::JobsCommand(const char *cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line){}

KillCommand::KillCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line) {}

void KillCommand::execute() {
    if(numOfArgs< 3){
        std::cerr << "smash error: kill: invalid arguments" << std::endl;
        return;
    }

    SmallShell::getInstance().getJobsList()->removeFinishedJobs();
    int signum;
    int jobId;

    try{
        jobId = stoi(args[2]);
    }
    catch(std::invalid_argument const& ex) {
        std::cerr << "smash error: kill: invalid arguments" << std::endl;
        return;
    }

    if (!SmallShell::getInstance().jobIdExist(jobId)) {
        std::cerr << "smash error: kill: job-id " << jobId << " does not exist" << std::endl;
        return;
    }

    if(numOfArgs != 3){
        std::cerr << "smash error: kill: invalid arguments" << std::endl;
        return;
    }

    if (args[1][0] != '-' ) {
        std::cerr << "smash error: kill: invalid arguments" << std::endl;
        return;
    }
    else {
        try {
            signum = stoi(&args[1][1]);
        }
        catch (std::invalid_argument const &ex) {
            std::cerr << "smash error: kill: invalid arguments" << std::endl;
            return;
        }
    }


    try {
        JobsList::JobEntry &targetJob = SmallShell::getInstance().getJob(jobId);
        int targetPid = targetJob.getJobPid();
        // no need to check for sigstop
        // https://piazza.com/class/lrc4hab2gwm1z/post/52
        if (kill(targetPid, signum) == 0) {
            std::cout << "signal number " << signum << " was sent to pid " << targetPid << std::endl;
        } else {
            perror("smash error: kill failed");

        }
    }
    catch(std::out_of_range const& ex){
        std::cerr << "smash error: kill: job-id " << jobId << " does not exist" << std::endl;
    }


}

ShowPidCommand::ShowPidCommand(const char *cmd_line) : BuiltInCommand(cmd_line){}

QuitCommand::QuitCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line) {}

std::list<JobsList::JobEntry>::iterator JobsList::begin() {
    return m_job_entry_list->begin();
}

std::list<JobsList::JobEntry>::iterator JobsList::end() {
    return m_job_entry_list->end();
}

void QuitCommand::execute() {
    if (numOfArgs > 2) {
        std::cerr << "smash error: quit: invalid arguments" << std::endl;
    }
    else {
        if (numOfArgs == 2 && strcmp(args[1], "kill") == 0) {
            SmallShell::getInstance().getJobsList()->removeFinishedJobs();
            std::cout << "smash: sending SIGKILL signal to " << SmallShell::getInstance().getJobsList()->getSize() << " jobs:" << std::endl;
            for (auto &job : *SmallShell::getInstance().getJobsList()) {
                std::cout << job.getJobPid() << ": " << job.getCmdLine() << std::endl;
                if(kill(job.getJobPid(), SIGKILL) != 0){
                    perror("smash error: kill failed");
                }
            }
        }
        exit(EXIT_SUCCESS);
    }
}

GetCurrDirCommand::GetCurrDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line){}

ChpromptCommand::ChpromptCommand(const char *cmd_line) : BuiltInCommand(cmd_line){}

ChmodCommand::ChmodCommand(const char *cmd_line) : BuiltInCommand(cmd_line){}

ExternalCommand::ExternalCommand(const char *cmd_line)
    : Command(cmd_line), m_is_bg(_isBackgroundComamnd(cmd_line)), m_cmd_line(cmd_line),
    m_no_back_sign(const_cast<char *>(cmd_line))
    {
    if(m_is_bg){
        _removeBackgroundSign(m_no_back_sign);
        _parseCommandLine(cmd_line, args);
    }
    }

void ExternalCommand::execute() {
    int stat;
    int pid = fork();
    if( pid < 0 ) {
        perror("smash error: fork failed");
    }
    else if (pid== 0) { // child
        setpgrp();
        if(m_cmd_line.find('*') != std::string::npos || m_cmd_line.find('?') != std::string::npos){
            // complex external command
            string bash = "bash";
            string c = "-c";
            char* arg_arr[] ={const_cast<char *>(bash.c_str()), const_cast<char *>(c.c_str()),
                              const_cast<char *>(m_no_back_sign), NULL};
            if(execvp("bash", arg_arr) == -1){
                exit(EXIT_FAILURE);
            }
            exit(EXIT_SUCCESS);

        }
        // simple external command
        else if(execvp(args[0], args) == -1){
            perror("smash error: execvp failed");
            exit(EXIT_FAILURE);
        }

        exit(EXIT_SUCCESS);
    }
    else { // parent
        this->pid = pid;

        if(m_is_bg){
            // this is a background command
            SmallShell::getInstance().getJobsList()->addJob(this, true);
        }
        else{
            SmallShell::getInstance().getJobsList()->addJob(this, false);
        }
        if(!m_is_bg && wait(&stat) < 0 )
            perror("smash error : wait failed");
        if (!m_is_bg) {
            waitpid(pid, &stat, WUNTRACED);
        }
    }
}


void ChmodCommand::execute() {
    if(numOfArgs !=3){
        std::cerr << "smash error: chmod: invalid arguments" << std::endl;
    }
    else{
        string mode_str = args[1];
        string path = args[2];
        mode_t mode;
        if(!isdigit(*mode_str.c_str())){
            std::cerr << "smash error: chmod: invalid arguments" << std::endl;
            return;
        }
        try{
            mode = stoi(mode_str,nullptr,8);
        }
        catch(std::invalid_argument const& ex){
            std::cerr << "smash error: chmod: invalid arguments" << std::endl;
        }
        if(chmod(path.c_str(), mode) == -1){
            perror("smash error: chmod failed");
        }


    }

}


ForegroundCommand::ForegroundCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line) {

}

void ForegroundCommand::execute() {
    SmallShell::getInstance().getJobsList()->removeFinishedJobs();

    if (numOfArgs == 1 && SmallShell::getInstance().getJobsList()->getSize() == 0) {
        std::cerr << "smash error: fg: jobs list is empty" << std::endl;
        return;
    }


    SmallShell::getInstance().getJobsList()->removeFinishedJobs();
    int theJobID;

    if(numOfArgs >= 2) {
        string jobID_str = args[1];
        try {
            theJobID = stoi(jobID_str);
        }
        catch (std::invalid_argument const &ex) {
            std::cerr << "smash error: fg: invalid arguments" << std::endl;
            return;
        }
        if (!SmallShell::getInstance().jobIdExist(theJobID)) {
            std::cerr << "smash error: fg: job-id " << theJobID << " does not exist" << std::endl;
            return;
        }
    }

    if (numOfArgs > 2) { // more than 2 arguments
        std::cerr << "smash error: fg: invalid arguments" << std::endl;
        return;
    }

    if(numOfArgs==2 && SmallShell::getInstance().jobIdExist(theJobID)) {
        JobsList::JobEntry &theJob = SmallShell::getInstance().getJob(theJobID);
        theJob.setStatus(JobsList::JobEntry::fg);
        std::cout << theJob.getCmdLine() << " " << theJob.getJobPid() << std::endl;
        waitpid(theJob.getJobPid(), nullptr, WUNTRACED);
        SmallShell::getInstance().getJobsList()->removeJobById(theJobID);
    }


    if (numOfArgs == 1) {
        try {
            JobsList::JobEntry &theJob = SmallShell::getInstance().getJob();
            theJob.setStatus(JobsList::JobEntry::fg);
            std::cout << theJob.getCmdLine() << " " << theJob.getJobPid() << std::endl;
            waitpid(theJob.getJobPid(), nullptr, WUNTRACED);
            SmallShell::getInstance().getJobsList()->removeJobById(theJobID);
        }
        catch (std::out_of_range const &ex) {
            std::cerr << "smash error: fg: job-id " << theJobID << " does not exist" << std::endl;
        }
    }


}

void JobsCommand::execute() {
    SmallShell::getInstance().getJobsList()->removeFinishedJobs();
    SmallShell::getInstance().getJobsList()->printJobsList();
}


void ChpromptCommand::execute()
{
    if (args[1])
    {
        SmallShell::getInstance().setName(args[1]);
    }
    else
    {
        SmallShell::getInstance().setName("smash");
    }
}

void ShowPidCommand::execute() {
    std::cout << "smash pid is " << SmallShell::getInstance().getPid() << std::endl;
}

void GetCurrDirCommand::execute() {
    if(SmallShell::getInstance().getCurrentPath().c_str()) {
        std::cout << SmallShell::getInstance().getCurrentPath() << std::endl;
    }
    else{
        std::cerr << "smash error: could not get the current path" << std::endl;
    }

}

void ChangeDirCommand::execute() {
    if(numOfArgs >2){
        std::cerr << "smash error: cd: too many arguments" << std::endl;
    }
    else if(numOfArgs == 1){
        std::cerr << "smash error: cd: not enough arguments" << std::endl;
    }
    else {
        SmallShell::getInstance().setCurrentPath(args[1]);
    }
}

Command::~Command(){
    for(auto & arg : args){
        delete arg;
    }
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
	// For example:

  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));


    if(cmd_s.find('>') != std::string::npos){
        return new RedirectionCommand(cmd_line);
    }

    else if(cmd_s.find('|') != std::string::npos){
        return new PipeCommand(cmd_line);
    }

    else if(firstWord.compare("chprompt") == 0){
    return new ChpromptCommand(cmd_line);
  }

  else if (firstWord.compare("showpid") == 0) {
      return new ShowPidCommand(cmd_line);
  }

   else if (firstWord.compare("pwd") == 0) {
     return new GetCurrDirCommand(cmd_line);
   }

  else if (firstWord.compare("cd") == 0) {
      return new ChangeDirCommand(cmd_line, nullptr);
  }
    else if (firstWord.compare("jobs") == 0) {
        return new JobsCommand(cmd_line, m_jobs_list);
    }

  else if(firstWord.compare("chmod") == 0){
        return new ChmodCommand(cmd_line);
    }
    else if (firstWord.compare("quit") == 0) {
        return new QuitCommand(cmd_line, m_jobs_list);
    }
    else if (firstWord.compare("kill") == 0) {
        return new KillCommand(cmd_line, m_jobs_list);
    }
    else if (firstWord.compare("fg") == 0) {
        return new ForegroundCommand(cmd_line, m_jobs_list);
    }
    else{
        return new ExternalCommand(cmd_line);
    }




    /*
    else if ...
    .....
    else {
      return new ExternalCommand(cmd_line);
    }
    */
  return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
  // TODO: Add your implementation here
  // for example:

    Command* cmd = CreateCommand(cmd_line);
    cmd->execute();
  // Please note that you must fork smash process for some commands (e.g., external commands....)
}

std::string SmallShell::getName() {
    return m_name;
}

int SmallShell::getPid() const {
    return m_pid;
}

void SmallShell::setCurrentPath(const string &new_path) {
    if(new_path == "-"){
        if(!m_previous_path.empty()){
            std::string temp = m_current_path;
            m_current_path = m_previous_path;
            m_previous_path = temp;
            if(chdir(m_current_path.c_str())!=0){
                perror("smash error: chdir failed");
            }
        }
        else {
            std::cerr << "smash error: cd: OLDPWD not set" << std::endl;
        }
        return;
    }
    else if(chdir(new_path.c_str())==0){
           m_previous_path = m_current_path;
           updateCurrentPath();
           if(SmallShell::getInstance().getCurrentPath().empty()) {
                std::cerr << "smash error: could not get the current path" << std::endl;
            }
    }
    else
    {
        perror("smash error: chdir failed");
    }
}

std::string SmallShell::getCurrentPath() {
    return m_current_path;
}

void SmallShell::setPreviousPath(const string &new_path) {
    m_previous_path = new_path;

}

std::string SmallShell::getPreviousPath() {
    return m_previous_path;
}

void SmallShell::updateCurrentPath() {
    char buff[PATH_MAX];
    if(getcwd( buff, PATH_MAX)) {
        m_current_path = buff;
    }
    else{
        m_current_path = "";
    }

}

bool SmallShell::jobIdExist(int jobId) {
    for(auto job : *m_jobs_list){
        if(job.getJobId() == jobId){
            return true;
        }
    }
    return false;
}

Command::Command(const char *cmd_line) : numOfArgs(_parseCommandLine(cmd_line,args)), cmd_line(cmd_line), pid(0)
{}

BuiltInCommand::BuiltInCommand(const char* cmd_line): Command(cmd_line){}

JobsList::JobEntry::JobEntry(int job_id, int pid, const char *cmd_line, Status status) : m_job_id(job_id),
m_pid(pid), m_cmd_line(cmd_line), m_status(status){}

int JobsList::JobEntry::getJobId() const {
    return m_job_id;
}

int JobsList::JobEntry::getJobPid() const {
    return m_pid;
}

const char *JobsList::JobEntry::getCmdLine() const {
    return m_cmd_line;
}

JobsList::JobEntry::Status JobsList::JobEntry::getStatus() const {
    return m_status;
}

void JobsList::JobEntry::setStatus(Status new_status) {
    m_status = new_status;
}

void JobsList::printJobsList() {
    // not sure if sort will be needed for final solution
    //m_jobs_list.sort(compareById);

    for ( auto job : *m_job_entry_list) {
        if (job.getStatus() == JobEntry::Status::bg) {
            std::cout << "[" << job.getJobId() << "] " << job.getCmdLine() << std::endl;
        }
    }
}

void JobsList::removeJobById(int jobId) {
//    auto it = std::remove_if(m_job_entry_list->begin(), m_job_entry_list->end(),
//                             [jobId](const JobEntry& job) { return job.getJobId() == jobId; });
//    m_job_entry_list->erase(it, m_job_entry_list->end());
    for(auto job : *m_job_entry_list){
        if(job.getJobPid() == jobId){
            job.setStatus(JobEntry::Status::finished);
        }
    }
}

void JobsList::removeFinishedJobs() {
    //m_job_entry_list->remove_if([](const JobEntry& job) { return job.getStatus() == JobEntry::Status::finished; });
    for(auto job_iter = m_job_entry_list->begin(); job_iter != m_job_entry_list->end(); job_iter++)
    {
        JobEntry job = *job_iter;
        int status;
        pid_t result = waitpid(job.getJobPid(), &status, WNOHANG);
        if(result!=0){
            job_iter = m_job_entry_list->erase(job_iter);
            job_iter--;
        }
    }
    if(m_job_entry_list->empty()){
        m_max_job_id = 0;
    }
    else{
        auto job_iter = m_job_entry_list->end();
        job_iter--;
        m_max_job_id =  job_iter->getJobId();
    }

}


JobsList::JobsList() : m_job_entry_list(new std::list<JobEntry>), m_max_job_id(0) {}

void JobsList::addJob(Command *cmd, bool isBg) {
    removeFinishedJobs();

    JobEntry::Status status = JobEntry::Status::fg;
    if(isBg){
        status = JobEntry::Status::bg;
    }
    JobEntry job(m_max_job_id+1, cmd->pid, cmd->cmd_line.c_str(), status);
    m_job_entry_list->push_back(job);
    m_max_job_id++;

}

JobsList::~JobsList() {
    delete m_job_entry_list;
}

RedirectionCommand::RedirectionCommand(const char *cmd_line) : Command(cmd_line), m_cmd_line(cmd_line) {}

PipeCommand::PipeCommand(const char *cmd_line) : Command(cmd_line), m_cmd_line(cmd_line) {}

void PipeCommand::execute() {
    int diff = 1;
    string op = "|";
    int type = 1;

    if(m_cmd_line.find("|&") != std::string::npos){
        diff = 2;
        op = "|&";
        type = 2;
    }

    size_t end_command = m_cmd_line.find(op);
    string command1 = m_cmd_line.substr(0, end_command);
    command1 = _trim(command1);
    string command2 = m_cmd_line.substr(end_command + diff, m_cmd_line.length());
    command2 = _trim(command2);

    int fd[2];
    pipe(fd);
    int pid = fork();
    if (pid == 0) { // first child
        setpgrp();
        dup2(fd[1],type); // change stdout
        close(fd[0]);
        close(fd[1]);
        SmallShell::getInstance().executeCommand(command1.c_str());
        exit(0);

    }
    else if(pid == -1){
        perror("smash error: fork failed");
    }
    pid = fork();
    if (pid == 0) { // second child
        setpgrp();
        dup2(fd[0],0); // change stdin
        close(fd[0]);
        close(fd[1]);
        SmallShell::getInstance().executeCommand(command2.c_str());
        exit(0);
    }
    else if(pid == -1){
        perror("smash error: fork failed");
    }
    close(fd[0]);
    close(fd[1]);
    wait(nullptr);
    wait(nullptr);



}

void RedirectionCommand::execute() {
    int file_mode = O_WRONLY|O_CREAT|O_TRUNC;
    string op = ">";
    int diff = 1;

    if(m_cmd_line.find(">>") != std::string::npos){
        file_mode = O_WRONLY|O_CREAT|O_APPEND;
        op = ">>";
        diff = 2;
    }
    size_t end_command = m_cmd_line.find(op);
    string command = m_cmd_line.substr(0, end_command);
    command = _trim(command);
    string file_name = m_cmd_line.substr(end_command + diff, m_cmd_line.length());
    file_name = _trim(file_name);

    int fd = open(file_name.c_str(), file_mode, 0666);  //open file
    if(fd == -1){
        std::cerr  << "smash error:> could not open file" << std::endl;
        return;
    }

    int oldOut = dup(1); //save screen FD
    dup2(fd,1); //change output stream from screen to file
    SmallShell::getInstance().executeCommand(command.c_str());
    dup2(oldOut,1); //change output stream from file back screen
    int close_check = close(fd);
    if(close_check == -1){
        std::cerr  << "smash error:> could not close file" << std::endl;
    }
}