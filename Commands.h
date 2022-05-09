#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/time.h>
#include <utime.h>
#include <time.h>
#include <limits.h>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define READ_PIPE (0)
#define WRITE_PIPE (1)
#define STD_ERROR_CHANNEL (2)
#define STD_OUT_CHANNEL (1)
#define STD_IN_CHANNEL (0)
#define REDIRECTION_CHAR ('>')


class Command {
public:
    std::string cmd_line;

    explicit Command(const char* cmd_line): cmd_line(cmd_line){};
    virtual ~Command() = default;
    virtual void execute() = 0;
    //virtual void prepare();
    //virtual void cleanup();
    // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
public:
    BuiltInCommand(const char* cmd_line) : Command(cmd_line){};
    virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
public:
    ExternalCommand(const char* cmd_line) : Command(cmd_line){};
    virtual ~ExternalCommand() {}
    void execute() override;
};

class PipeCommand : public Command {
    // TODO: Add your data members
public:
    PipeCommand(const char* cmd_line) : Command(cmd_line){}; //
    virtual ~PipeCommand() {}
    void execute() override;
};

class RedirectionCommand : public Command {
public:
    RedirectionCommand(const char* cmd_line) : Command(cmd_line){};
    virtual ~RedirectionCommand() {}
    void execute() override;
    //void prepare() override;
    //void cleanup() override;
};

class ChangeDirCommand : public BuiltInCommand {
// TODO: Add your data members public:
public:
    ChangeDirCommand(const char* cmd_line, char** plastPwd) : BuiltInCommand(cmd_line){};
    virtual ~ChangeDirCommand() {}
    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    GetCurrDirCommand(const char* cmd_line) : BuiltInCommand(cmd_line){};
    virtual ~GetCurrDirCommand() {}
    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(const char* cmd_line) : BuiltInCommand(cmd_line){};
    virtual ~ShowPidCommand() {}
    void execute() override;
};

class ChangePromptCommand : public BuiltInCommand {
public:
    ChangePromptCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {};
    virtual ~ChangePromptCommand() {}
    void execute() override;
};

class JobsList;
class QuitCommand : public BuiltInCommand {
// TODO: Add your data members public:
public:
    QuitCommand(const char* cmd_line, JobsList* jobs): BuiltInCommand(cmd_line){}; // we need to implement quit execute
    virtual ~QuitCommand() {}
    void execute() override;//
};




class JobsList {
public:
    class JobEntry {
    public:
        pid_t pid;
        int job_id;
        time_t create_time;
        Command *command;
        bool is_stopped = false;
        JobEntry(Command *cmd, pid_t pid, bool is_stopped = false) //constructor for JobEntry class - must define the job_id later
        {
            this->command = cmd;
            this->create_time = time(nullptr);
            this->pid = pid; // maybe change
            this->job_id = -1;
        }
    };
    // TODO: Add your data members
    std::vector<JobEntry> jobs_list;

public:
    JobsList() = default;
    ~JobsList() = default;
    JobEntry* addJob(Command* cmd, pid_t pid, bool isStopped = false); //changed to void
    void insertJob(JobEntry *new_job);
    void printJobsList();
    void killAllJobs();
    void removeFinishedJobs();
    JobEntry * getJobById(int jobId);
    void removeJobById(int jobId);
    JobEntry * getLastJob(int* lastJobId);
    JobEntry *getLastStoppedJob(int *jobId);
    // TODO: Add extra methods or modify exisitng ones as needed
};

class JobsCommand : public BuiltInCommand {
    // TODO: Add your data members
    //TODO we can add job var and then we dont have to use getinstance
public:
    JobsCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line){};
    virtual ~JobsCommand() {}
    void execute() override;
};

class KillCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    KillCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line){};
    virtual ~KillCommand() {}
    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    ForegroundCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line){};
    virtual ~ForegroundCommand() {}
    void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    BackgroundCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line){};
    virtual ~BackgroundCommand() {}
    void execute() override;
};

class TailCommand : public BuiltInCommand {
public:
    TailCommand(const char* cmd_line) : BuiltInCommand(cmd_line){};
    virtual ~TailCommand() {}
    void execute() override;
};

class TouchCommand : public BuiltInCommand {
public:
    TouchCommand(const char* cmd_line) : BuiltInCommand(cmd_line){};
    virtual ~TouchCommand() {}
    void execute() override;
};


class SmallShell {
private:
    // TODO: Add your data members
    SmallShell();
public:
    std::string current_prompt = "smash";
    std::string prev_dir = "";
    bool dir_changed_flag = false;
    JobsList jobs;
    JobsList::JobEntry* foreground;
    bool quit = false;

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
    Command *decideCommand(const char* cmd_line);
    void executeCommand(const char* cmd_line);
    // TODO: add extra methods as needed
};

#endif //SMASH_COMMAND_H_