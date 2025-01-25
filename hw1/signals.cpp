#include <iostream>
#include <signal.h>
#include <sys/wait.h>
#include "signals.h"
#include "Commands.h"

using namespace std;


void ctrlCHandler(int sig_num) {
    std::cout << "smash: got ctrl-C" << std::endl;
    JobsList* jobsList = SmallShell::getInstance().getJobsList();
    for (auto & job : *jobsList->getJobsList())
    {
        if(job.getStatus() == JobsList::JobEntry::Status::fg)
        {
            int status;
            pid_t result = waitpid(job.getJobPid(), &status, WNOHANG);
            if (result == 0) {
                // Child still alive
                if (kill(job.getJobPid(), sig_num) == -1)
                    perror("smash error: kill failed");
                std::cout << "smash: process " << job.getJobPid() << " was killed" << std::endl;
            }
        }
    }
    // If there is no process running in the foreground, then the smash should ignore them.
}


void alarmHandler(int sig_num) {

}


