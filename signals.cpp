#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {
    SmallShell &smash = SmallShell::getInstance();
    cout << "smash: got ctrl-Z\n";
    if (!smash.foreground || smash.foreground->pid == -1) return;
    int res = kill(smash.foreground->pid, SIGSTOP);
    if (res == -1) //check error handle requirement
    {
        perror("smash error: kill failed");
        return;
    }
    smash.foreground->is_stopped = true;
    smash.jobs.insertJob(smash.foreground);
    cout << "smash: process " << smash.foreground->pid << " was stopped";
}

void ctrlCHandler(int sig_num) {
    SmallShell &smash = SmallShell::getInstance();
    cout << "smash: got ctrl-C\n";
    if (!smash.foreground || smash.foreground->pid == -1) return;
    int res = kill(smash.foreground->pid, SIGKILL);
    if (res == -1) //check error handle requirement
    {
        perror("smash error: kill failed");
    }
    cout << "smash: process " << smash.foreground->pid << " was killed";
}

void alarmHandler(int sig_num) {
    SmallShell &smash = SmallShell::getInstance();

    cout <<"smash: got an alarm\n";
}

