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

vector<string> splitString(string s, char token)
{
    vector<string> v;
    string temp = "";
    for (int i = 0; i < s.length(); ++i)
    {

        if (s[i] == token)
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
    vector<string> params = splitString(cmd_line, ' ');
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

bool pipe_to_stderr(string cmd, int index){
    int i = index + 1;
    while(cmd.at(i) == ' '){
        i++;
    }
    if (cmd.at(i)=='&'){
        return i;
    }
    return -1;
}

void PipeCommand::execute() {
    int index = cmd_line.find('|');
    int index2 = pipe_to_stderr(cmd_line,index);
    int pipe_arr[2];
    SmallShell &smash = SmallShell::getInstance();
    if(pipe(pipe_arr) == -1){
        perror("smash error: pipe failed");
    }

    int pid_first = fork();
    if(pid_first == -1){
        perror("smash error: fork failed");
    }
    if(pid_first == 0){
        if(setpgrp() == -1){
            perror("smash error: setpgrp failed");
        }
        if(close(pipe_arr[READ_PIPE]) == -1) { //disable reading
            perror("smash error: close failed");
        }
        if(index2>0){
            if (dup2(pipe_arr[WRITE_PIPE], STD_ERROR_CHANNEL) == -1) { //redirect stderr to write channel
                perror("smash error: dup2 failed");
            }
        }else {
            if (dup2(pipe_arr[WRITE_PIPE], STD_OUT_CHANNEL) == -1) { //redirect stdout to write channel
                perror("smash error: dup2 failed");
            }
            int i = pipe_to_stderr(cmd_line,0); //removing the & from the beginning
            smash.executeCommand(cmd_line.substr(i + 1, index - 1).c_str());
        }
    }
    waitpid(pid_first, nullptr, 0);
    int pid_second = fork();
    if(pid_second == -1){
        perror("smash error: fork failed");
    }
    if(pid_second == 0){ //child
        if(setpgrp() == -1){
            perror("smash error: setpgrp failed");
        }
        if(close(pipe_arr[WRITE_PIPE]) == -1){ //disable writing
            perror("smash error: close failed");
        }
        if(dup2(pipe_arr[READ_PIPE],STD_IN_CHANNEL) == -1){
            perror("smash error: dup2 failed"); //redirect stdin to read channel
        }
        smash.executeCommand(cmd_line.substr(index2+1,cmd_line.length()).c_str());
    }
    if(close(pipe_arr[READ_PIPE]) == -1) {
        perror("smash error: close failed");
    }
    if(close(pipe_arr[WRITE_PIPE]) == -1){
        perror("smash error: close failed");
    }
    if(waitpid(pid_second, nullptr,0) == -1){
        perror("smash error: waitpid failed");
    }
}

void GetCurrDirCommand::execute()
{
    char cwd[PATH_MAX];
    if (getcwd(cwd, PATH_MAX) == NULL) //TODO maybe change to -1
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
    vector<string> params = splitString(cmd_line, ' ');
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
            fprintf(stderr, "smash error: bg: job-id <%s> does not exist", job_id_str.c_str() , strerror(errno));
            return;
        }

        // job is running
        if (!job->is_stopped)
        {
            fprintf(stderr, "smash error: bg: job-id <%s> is already running in the background", job_id_str.c_str() , strerror(errno));
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
        perror("smash error: getcwd failed");
    }
    vector<string> params = splitString(cmd_line, ' ');
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
    smash.jobs.removeFinishedJobs();
    smash.jobs.printJobsList();
}

void KillCommand::execute()
{
    SmallShell &smash = SmallShell::getInstance();
    vector<string> params = splitString(cmd_line, ' ');

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
    SmallShell &smash = SmallShell::getInstance();
    vector<string> params = splitString(cmd_line, ' ');
    bool is_background = params.back() == "&" || (params.back()).back() == '&';
    int pid = fork();
    if(pid == -1){
        perror("smash error: fork failed");
    }

    if (pid == 0){ //child
        if(setpgrp() == -1){
            perror("smash error: setpgrp failed");
        }
        char command[COMMAND_ARGS_MAX_LENGTH] = {};
        cmd_line.copy(command , cmd_line.length());
        _removeBackgroundSign(command);
        char* argv[4] = {"/bin/bash","-c",command, nullptr};

        if(execv(argv[0],argv)==-1){
            perror("smash error: execv failed");
            exit(1);
        }
        return;
    } else { //father code
        JobsList::JobEntry *job = new JobsList::JobEntry(this, pid);//smash.jobs.addJob(this, pid);
        job->job_id = smash.jobs.jobs_list.empty() ? 1 : (smash.jobs.jobs_list.back().job_id + 1);
        if(!is_background){
            int status = 0;
            smash.foreground = job;
            int wait_pid_return = waitpid(pid,&status,WUNTRACED);
            if(WIFSTOPPED(status)){
                    job->is_stopped = true;
            }
            smash.foreground = nullptr;
        }
        else
        {
            smash.jobs.removeFinishedJobs();
            smash.jobs.insertJob(job);
        }
    }
}

void ForegroundCommand::execute()
{
    SmallShell &smash = SmallShell::getInstance();
    vector<string> params = splitString(cmd_line, ' ');

    string fg_cmd_line;
    pid_t fg_pid;
    int job_id = -1;
    if (params.size() > 2)
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
            JobsList::JobEntry *job = &smash.jobs.jobs_list.back();
            fg_pid = smash.jobs.jobs_list.back().pid;
            job_id = smash.jobs.jobs_list.back().job_id;
            smash.foreground = job;
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
            smash.jobs.removeJobById(job_id);
            smash.foreground = job;
        }
    }

    // sending the job to the foreground
    std::cout << fg_cmd_line << endl;
    if (kill(fg_pid, SIGCONT) == -1)
    {
        perror("smash error: SIGCONT failed");
        return;
    }
    //maybe check return value? no error message explicit
    waitpid(fg_pid, nullptr, WUNTRACED); //that's what I understood for the start_loc and options
    smash.jobs.removeJobById(job_id);
}

void QuitCommand::execute() {
    //job list kill
    SmallShell &smash = SmallShell::getInstance();
    vector<string> params = splitString(cmd_line, ' ');
    if (params.size() != 2 || params[1] != "kill")
    {
        smash.quit = true;
        return;
    }
    cout << "sending SIGKILL signal to " << smash.jobs.jobs_list.size() << " jobs";
    smash.jobs.printJobsList();//check print format
    smash.jobs.killAllJobs();
    smash.quit = true;
}

void TailCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    vector<string> params = splitString(cmd_line, ' ');
    long N = 10;

    string path;
    if (params.size()>3) //checking paramters
    {
        perror("smash error: tail: invalid arguments");
        return;
    }

    //setting up N and PATH for file
    if (params.size()==3)
    {
        char* p;
        long converted = strtol(params.at(1).c_str(), &p, 10);
        if (*p) {
            perror("smash error: tail: invalid arguments");
            return;
        }
        else {
            N = converted;
        }
        path = params.at(2);
    }
    else
        path = params.at(1);

    //getting file descriptor
    int fd = open(path.c_str(),O_RDONLY);
    if (fd == -1)
    {
        perror("smash error: open failed");
        return;
    }

    string line;
    long line_size;
    string input ;
    int count = 0;
    long write_res;
    while(count<N) //going through N lines in the file
    {
        long read_size = read(fd, (void *)&input, 1);
        if (read_size == -1)
        {
            perror("smash error: read failed");
            return;
        }
        if (read_size == 0) return;

        if (input == "\n")
        {
            count++;
            line_size++;
            (line) += "\n";
            write_res = write(1, (void *) &line, line_size); //pretty sure 1 is the fd to write to but should check later.
            if (write_res == -1)
            {
                perror("smash error: write failed");
                return;
            }
            line_size = 0;
            line = "";
        }
        else
        {
            line_size++;
            line += input;
        }
    }
}

void TouchCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    vector<string> params = splitString(cmd_line, ' ');

    //checking format of parameters
    if (params.size() != 3)
    {
        perror("smash error: touch: invalid arguments");
        return;
    }
    utimbuf time;
    tm t_time;

    //splitting the time data into a dates vector
    std::vector<std::string> dates;
    istringstream iss(params.at(2));
    std::string token;
    while (std::getline(iss, token, ':')) {
        if (!token.empty())
            dates.push_back(token);
    }

    //setting time struct
    t_time.tm_sec = atoi(dates.at(0).c_str());
    t_time.tm_min = atoi(dates.at(1).c_str());
    t_time.tm_hour = atoi(dates.at(2).c_str());
    t_time.tm_mday = atoi(dates.at(3).c_str());
    t_time.tm_mon = atoi(dates.at(4).c_str());
    t_time.tm_year = atoi(dates.at(5).c_str());
    t_time.tm_wday = 0;
    t_time.tm_yday = 0;
    t_time.tm_isdst = -1;

    time.actime = mktime(&t_time);
    if (time.actime == -1)
    {
        perror("smash error: mktime failed");
    }
    time.modtime = mktime(&t_time);
    if (time.modtime == -1)
    {
        perror("smash error: mktime failed");
    }
    int res = utime(params.at(1).c_str(), &time);
    if (res == -1)
    {
        perror("smash error: utime failed");
    }
}

void RedirectionCommand::execute() { //">>" append //remove background sign
    SmallShell &smash = SmallShell::getInstance();
    string cmd_s = _trim(string(cmd_line));
    vector<string> request = splitString(cmd_s, REDIRECTION_CHAR);

    int old_fd = dup(STD_OUT_CHANNEL);
    if (old_fd == -1) {
        perror("smash error: dup failed");
        return;
    }
    string data = request[-1];
    int new_fd = open(data.c_str(), O_CREAT | (request.size()==3 ? O_APPEND : O_TRUNC) | S_IRUSR | S_IWUSR | S_IRWXG | S_IRWXO);
    if (new_fd == -1){
        perror("smash error: open failed");
        return;
    }
    int res = request.size() == 3 ? dup2(STD_OUT_CHANNEL, new_fd) : dup3(STD_OUT_CHANNEL, new_fd, O_APPEND); //swap between standard out put to the file fd
    if (res == -1)
    {
        perror("smash error: dup failed"); //might need to print dup2 or dup3 :/
    }

    smash.CreateCommand(request[0].c_str())->execute();

    res = dup2(STD_OUT_CHANNEL, old_fd);
    if (res == -1)
    {
        perror("smash open: dup2 failed"); //might need to print dup2 or dup3 :/
    }
}

// joblist functions

//JobsList::JobEntry *JobsList::addJob(Command *cmd, pid_t pid, bool isStopped)
//{
//    JobEntry *new_job = new JobEntry(cmd, pid, isStopped);
//    new_job->command = cmd;
//    new_job->create_time = time(nullptr);
//    new_job->pid = pid; // maybe change
//    new_job->job_id = jobs_list.empty() ? 1 : (jobs_list.back().job_id + 1);
//
//    // the list sorted by job_id
//    insertJob(new_job);
//
//    return new_job;
//    //for (vector<JobEntry>::iterator it = jobs_list.begin(); it < jobs_list.end(); it++)
//    //{
//    //    if (new_job.job_id < it->job_id)
//    //    {
//    //        jobs_list.insert(it, new_job);
//    //    }
//    //}
//}

void JobsList::insertJob(JobEntry *new_job) {
    // the list sorted by job_id
    if (jobs_list.empty()) {
        jobs_list.push_back(*new_job);
        return;
    }
    for (vector<JobEntry>::iterator it = jobs_list.begin(); it < jobs_list.end(); it++)
    {
        if (new_job->job_id < it->job_id)
        {
            jobs_list.insert(it, *new_job);
            return;
        }
    }
    jobs_list.push_back(*new_job);
}

void JobsList::printJobsList()
{
    for (vector<JobEntry>::iterator it = jobs_list.begin(); it != jobs_list.end(); it++)
    {
        //std::cout << "it->job_id: " << it->job_id << endl;
        //std::cout << "it->command->cmd_line: " << it->command->cmd_line <<endl;
        //std::cout << "it->pid: " << it->pid << endl;
        //std::cout << "difftime: " << int(difftime(time(nullptr), it->create_time)) << endl;
        std::cout << "[" << (it->job_id) << "] " << (it->command->cmd_line) << " : " << it->pid << " " // might need to change the command printing
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
    for (vector<JobsList::JobEntry>::iterator it = jobs_list.begin(); it < jobs_list.end(); it++)
    {
        if (kill(it->pid, SIGKILL) == -1)
        {
            perror("smash error: SIGKILL failed");
        }
    }
}

void JobsList::removeFinishedJobs()
{
    if( jobs_list.empty())
        return;
    vector<JobsList::JobEntry>::iterator it = jobs_list.begin();
    while(it != jobs_list.end())
    {
        if( !it->is_stopped && waitpid(it->pid , nullptr, WNOHANG) > 0){
            jobs_list.erase(it);
        }
        else
            it++;
    }
}
JobsList::JobEntry * JobsList::getJobById(int jobId)
{
    for(vector<JobEntry>::iterator it = jobs_list.begin(); it < jobs_list.end(); it++){
        if(it->job_id == jobId){
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

// IO redirection commands



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
    string cmd_s = _trim(string(cmd_line));

    if (std::find(cmd_s.begin(), cmd_s.end(), REDIRECTION_CHAR) != cmd_s.end())
    {
        return new RedirectionCommand(cmd_line);
    }
    else return SmallShell::decideCommand(cmd_line);
}

Command *SmallShell::decideCommand(const char *cmd_line)
{
    SmallShell &smash = SmallShell::getInstance();
    JobsList* job_list = &smash.jobs;
    char *cwd = (char*)malloc(MAX_INPUT); //TODO change
    if (getcwd(cwd, PATH_MAX) == NULL)
    {
        perror("smash error: getcwd failed");
    }

    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
    if(std::find(cmd_s.begin(), cmd_s.end(), '|') != cmd_s.end()){
        return new PipeCommand(cmd_line);
    }
    else if (firstWord.compare("pwd") == 0)
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
    else if (firstWord == "tail")
    {
        return new TailCommand(cmd_line);
    }
    else if (firstWord == "touch")
    {
        return new TouchCommand(cmd_line);
    }
    else{
        return new ExternalCommand(cmd_line);
    }
}

void SmallShell::executeCommand(const char *cmd_line)
{
    // TODO: Add your implementation here
    // for example:
    Command *cmd = CreateCommand(cmd_line);
    // delete finished jobs
    if (cmd == nullptr) return;
    //might need to fork here
    //then save the child pid which will be the foreground pid
    //when getting ^c or ^z we will use the child pid
    //need to check if this is good
    SmallShell &smash = SmallShell::getInstance();

    smash.jobs.removeFinishedJobs();
    cmd->execute();

    // Please note that you must fork smash process for some commands (e.g., external commands....)
}