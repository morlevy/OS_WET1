#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {
    SmallShell &smash = SmallShell::getInstance();
    cout << "smash: got ctrl-Z\n";
    //cout << "smash.foreground->is_stopped: " << smash.foreground->is_stopped << endl;
    //cout << "smash.foreground->pid: " << smash.foreground->pid << endl;
    if (!smash.foreground || smash.foreground->pid == -1){
      //  cout << "returned without kill" << endl;
        return;
    }
    JobsList::JobEntry *job = smash.foreground;
    int job_pid = job->pid;
    int res = kill(job_pid, SIGSTOP);
    if (res == -1) //check error handle requirement
    {
        perror("smash error: kill failed\n");
        return;
    }
    smash.foreground->is_stopped = true;
    smash.jobs.insertJob(job);
    cout << "smash: process " << job_pid << " was stopped\n";
    smash.foreground = nullptr;
}

void ctrlCHandler(int sig_num) {
    SmallShell &smash = SmallShell::getInstance();
    cout << "smash: got ctrl-C\n";
    if (!smash.foreground || smash.foreground->pid == -1) return;
    int res = kill(smash.foreground->pid, SIGKILL);
    if (res == -1) //check error handle requirement
    {
        perror("smash error: kill failed\n");
    }
    cout << "smash: process " << smash.foreground->pid << " was killed\n";
    smash.foreground = nullptr;
}

void alarmHandler(int sig_num) {
    SmallShell &smash = SmallShell::getInstance();

    cout <<"smash: got an alarm\n";
}

