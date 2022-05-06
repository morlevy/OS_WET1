#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <time.h>
#include <utime.h>
#include <algorithm>
#include <signal.h>

using namespace std;

#if 0
#define FUNC_ENTRY() \
    cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT() \
    cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

const std::string WHITESPACE = " \n\r\t\f\v";

bool isNumber(string line)
{
    char *p;
    strtol(line.c_str(), &p, 10);
    return *p == 0;
}

string _ltrim(const std::string &s)
{
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string &s)
{
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string &s)
{
    return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char *cmd_line, char **args)
{
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for (std::string s; iss >> s;)
    {
        args[i] = (char *)malloc(s.length() + 1);
        memset(args[i], 0, s.length() + 1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;

    FUNC_EXIT()
}

bool _isBackgroundComamnd(const char *cmd_line)
{
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char *cmd_line)
{
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos)
    {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&')
    {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

// TODO: Add your implementation for classes in Commands.h

vector<string> splitString(string s)
{
    vector<string> v;
    string temp = "";
    for (int i = 0; i < s.length(); ++i)
    {

        if (s[i] == ' ')
        {
            v.push_back(temp);
            temp = "";
        }
        else
        {
            temp.push_back(s[i]);
        }
    }
    v.push_back(temp);
    return v;
}

void ChangePromptCommand::execute()
{
    SmallShell &smash = SmallShell::getInstance();
    vector<string> params = splitString(cmd_line);
    if (params.size() == 1)
    {
        smash.current_prompt = "smash";
    }
    else
    {
        smash.current_prompt = params.at(1);
    }
}

void ShowPidCommand::execute()
{
    std::cout << "smash pid is " << getpid() << endl;
}

void GetCurrDirCommand::execute()
{
    char cwd[PATH_MAX];
    if (getcwd(cwd, PATH_MAX) == NULL)
    {
        perror("smash error: getcwd failed");
    }
    else
    {
        std::cout << cwd << endl;
    }
}

void BackgroundCommand::execute()
{
    vector<string> params = splitString(cmd_line);
    SmallShell &smash = SmallShell::getInstance();

    // too many arguments
    if (params.size() > 2)
    {
        perror("smash error: bg: invalid arguments");
        return;
    }

    if (params.size() == 2)
    {
        string job_id_str = params.at(1);

        // argument is not number
        if (!isNumber(job_id_str))
        {
            perror("smash error: bg: invalid arguments");
            return;
        }

        int job_id = atoi(job_id_str.c_str());
        JobsList::JobEntry *job = smash.jobs.getJobById(job_id);

        // job doesnt exists
        if (job == nullptr)
        {
            fprintf(stderr, "smash error: bg: job-id <%s> does not exist", job_id_str, strerror(errno));
            return;
        }

        // job is running
        if (!job->is_stopped)
        {
            fprintf(stderr, "smash error: bg: job-id <%s> is already running in the background", job_id_str, strerror(errno));
            return;
        }

        
        kill(job->pid, SIGCONT);
        job->is_stopped = false;
    }
    else
    { // no parameters
        vector<JobsList::JobEntry> &job_list = smash.jobs.jobs_list;
        vector<JobsList::JobEntry>::iterator it = job_list.begin();
        bool not_found = true;

        while (it != job_list.end() && not_found)
        { // find stopped job
            if (it->is_stopped)
            {
                not_found = false;
                kill(it->pid, SIGCONT);
                return;
            }
            it++;
        }

        perror("smash error: bg: there is no stopped jobs to resume");
    }
}

void ChangeDirCommand::execute()
{
    SmallShell &smash = SmallShell::getInstance();
    char cwd[PATH_MAX];
    string path;
    if (getcwd(cwd, PATH_MAX) == NULL)
    {
        // error
    }
    vector<string> params = splitString(cmd_line);
    if (params.size() > 2)
    {
        perror("smash error: cd: too many arguments");
        return;
    }
    if (params.size() == 2)
    {
        if (params.at(1).compare("-") == 0)
        {
            if (!smash.dir_changed_flag)
            {
                perror("smash error: cd: OLDPWD not set");
                return;
            }
            path = smash.prev_dir;
        }
        else
        {
            path = params.at(1);
        }
        int return_value = chdir(path.c_str());
        if (return_value == -1)
        {
            perror("smash error: cd failed");
            return;
        }
        smash.prev_dir = cwd;
        smash.dir_changed_flag = true;
    }
}

void JobsCommand::execute()
{
    SmallShell &smash = SmallShell::getInstance();
    smash.jobs.printJobsList();
}

void KillCommand::execute()
{
    SmallShell &smash = SmallShell::getInstance();
    vector<string> params = splitString(cmd_line);

    if (params.size() != 3)
    {
        perror("smash error: kill: invalid arguments");
        return;
    }

    string signum_str = params.at(1);
    if (signum_str[0] != '-')
    {
        perror("smash error: kill: invalid arguments");
        return;
    }

    signum_str = signum_str.substr(1);     // slice the beginning
    int signum = atoi(signum_str.c_str()); // convert to int

    int job_id = atoi(params.at(2).c_str());
    JobsList::JobEntry *job = smash.jobs.getJobById(job_id);

    if (job == nullptr)
    {
        fprintf(stderr, "smash error: kill: job-id <%d> does not exist", job_id, strerror(errno));
        return;
    }
    JobsList::JobEntry *job = smash.jobs.getJobById(job_id);
    if (kill(job->pid, signum) == -1)
    {
        perror("smash error: kill failed");
        return;
    }
    else
    {
        cout << "signal number " << signum << " was sent to pid " << job->pid << endl;
    }
}

void ExternalCommand::execute(){
    //meanwhile
    return;
}

void ForegroundCommand::execute()
{
    SmallShell &smash = SmallShell::getInstance();
    vector<string> params = splitString(cmd_line);

    string fg_cmd_line;
    pid_t fg_pid;
    int job_id = -1;
    if (params.size() > 1)
    {
        perror("smash error: fg: invalid arguments");
        return;
    }

    // case no specific job
    if (params.size() == 1)
    {
        if (smash.jobs.jobs_list.empty())
        {
            perror("smash error: fg: jobs list is empty");
            return;
        }
        else
        {
            fg_cmd_line = smash.jobs.jobs_list.back().command->cmd_line;
            fg_pid = smash.jobs.jobs_list.back().pid;
            job_id = smash.jobs.jobs_list.back().job_id;
            // need to make sure that the big job id is indeed at the back, but probably it is
        }
    }
    else // case with extra parmater
    {
        if (!isNumber(params.at(1)))
        { // checking if the paramter is valid
            perror("smash error: fg: invalid arguments");
            return;
        }
        else
        {
            int job_id = atoi(params.at(1).c_str());
            JobsList::JobEntry *job = smash.jobs.getJobById(job_id);

            if (job == nullptr)
            { // checking if
                fprintf(stderr, "smash error: kill: job-id <%d> does not exist", job_id, strerror(errno));
                return;
            }
            fg_cmd_line = job->command->cmd_line;
            fg_pid = job->pid;
        }
    }

    // sending the job to the foreground
    std::cout << fg_cmd_line << endl;
    if (kill(fg_pid, SIGCONT) == -1)
    {
        perror("smash error: kill failed");
        return;
    }
    //maybe check return value? no error message explicit
    waitpid(fg_pid, nullptr, 0); //that's what I understood for the start_loc and options
    smash.jobs.removeJobById(job_id);
}

// joblist functions
void JobsList::addJob(Command *cmd, bool isStopped = false)
{
    JobEntry new_job;
    new_job.command = cmd;
    new_job.create_time = time(nullptr);
    new_job.pid = getpid(); // maybe change
    new_job.job_id = jobs_list.empty() ? 1 : (jobs_list.back().job_id + 1);

    // the list sorted by job_id
    for (vector<JobEntry>::iterator it = jobs_list.begin(); it < jobs_list.end(); it++)
    {
        if (new_job.job_id < it->job_id)
        {
            jobs_list.insert(it, new_job);
        }
    }
}

void JobsList::printJobsList()
{
    for (vector<JobEntry>::iterator it = jobs_list.begin(); it < jobs_list.end(); it++)
    {
        std::cout << '[' << it->job_id << '] ' << it->command->cmd_line << ' : ' << it->pid << ' ' // might need to change the command printing
                  << int(difftime(time(nullptr), it->create_time)) << " secs";

        if (it->is_stopped)
        {
            std::cout << " (stopped)";
        }
        std::cout << endl;
    }
}

void JobsList::killAllJobs()
{
}

//void killAllJobs();
void JobsList::removeFinishedJobs()
{
    vector<JobsList::JobEntry>::iterator it = jobs_list.begin();

    while(it != jobs_list.end())
    {
        if (it->is_stopped)
            removeJobById(it->job_id);
    }
}
JobsList::JobEntry * JobsList::getJobById(int jobId)
{
    for(vector<JobEntry>::iterator it = jobs_list.begin(); it < jobs_list.end(); it++){
        if(it->job_id = jobId){
            return it.base(); // im not sure about this.
        }
    }
    return nullptr;
}
void JobsList::removeJobById(int jobId)
{
    vector<JobsList::JobEntry>::iterator it = jobs_list.begin();
    while(it != jobs_list.end())
    {
        if (it->job_id == jobId)
        {   
            jobs_list.erase(it);
            return;
        }
        it++;
    }
}
JobsList::JobEntry * JobsList::getLastJob(int* lastJobId)
{
    if(jobs_list.empty()){
        return nullptr;
    }
    JobEntry &job = jobs_list.back();
    *lastJobId = job.job_id;
    return &job;
    
}
JobsList::JobEntry * JobsList::getLastStoppedJob(int *jobId)
{
    vector<JobsList::JobEntry>::iterator it = jobs_list.begin();
    while(it != jobs_list.end())
    {
        if (it->is_stopped)
            return it.base();
        it++;
    }
}

SmallShell::SmallShell()
{
    // TODO: add your implementation
}

SmallShell::~SmallShell()
{
    // TODO: add your implementation
}

/**
 * Creates and returns a pointer to Command class which matches the given command line (cmd_line)
 */
Command *SmallShell::CreateCommand(const char *cmd_line)
{
    // For example:
    SmallShell &smash = SmallShell::getInstance();
    JobsList* job_list = &smash.jobs;
    char *cwd = (char*)malloc(MAX_INPUT); //TODO change
    if (getcwd(cwd, PATH_MAX) == NULL)
    {
        perror("smash error: getcwd failed");
    }

    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

    if (firstWord.compare("pwd") == 0)
    {
        return new GetCurrDirCommand(cmd_line);
    }
    else if (firstWord.compare("showpid") == 0)
    {
        return new ShowPidCommand(cmd_line);
    }
    else if (firstWord.compare("chprompt") == 0)
    {
        return new ChangePromptCommand(cmd_line);
    }
    else if (firstWord.compare("cd") == 0)
    {
        return new ChangeDirCommand(cmd_line, &cwd);
    }
    else if (firstWord.compare("jobs") == 0)
    {
        return new JobsCommand(cmd_line,job_list);
    }
    else if (firstWord.compare("kill") == 0)
    {
        return new KillCommand(cmd_line,job_list);
    }
    else if (firstWord.compare("fg") == 0)
    {
        return new ForegroundCommand(cmd_line,job_list);
    }
    else if (firstWord.compare("bg") == 0)
    {
        return new BackgroundCommand(cmd_line,job_list);
    }
    else if (firstWord.compare("quit") == 0)
    {
        return new QuitCommand(cmd_line,job_list);
    }
    else{
        //return new ExternalCommand(cmd_line);
        return nullptr;
    }
}

void SmallShell::executeCommand(const char *cmd_line)
{
        // TODO: Add your implementation here
        // for example:
        Command *cmd = CreateCommand(cmd_line);
        // delete finished jobs
        cmd->execute();

        // Please note that you must fork smash process for some commands (e.g., external commands....)
}
