#define _GNU_SOURCE

#include "swish_funcs.h"

#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "job_list.h"
#include "string_vector.h"

#define MAX_ARGS 10

int tokenize(char *s, strvec_t *tokens)
{
    // TODO Task 0: Tokenize string s
    // Assume each token is separated by a single space (" ")
    // Use the strtok() function to accomplish this
    char *token = strtok(s, " ");

    // if (token == NULL) {
    //     perror("No string in input");
    //     return -1;
    // }

    while (token != NULL)
    {
        if (strvec_add(tokens, token) == -1)
        {
            return -1;
        }
        token = strtok(NULL, " ");
    }

    // Add each token to the 'tokens' parameter (a string vector)
    // Return 0 on success, -1 on error
    return 0;
}

int run_command(strvec_t *tokens)
{
    // TODO Task 2: Execute the specified program (token 0) with the
    // specified command-line arguments
    // THIS FUNCTION SHOULD BE CALLED FROM A CHILD OF THE MAIN SHELL PROCESS
    // Hint: Build a string array from the 'tokens' vector and pass this into execvp()
    // Another Hint: You have a guarantee of the longest possible needed array, so you
    // won't have to use malloc.
    char *args[MAX_ARGS + 1];
    for (int i = 0; i < MAX_ARGS + 1; i++)
    {
        args[i] = NULL;
    }
    int arg_index = 0;
    for (int i = 0; i < tokens->length; i++)
    {
        char *token = strvec_get(tokens, i);

        if (strcmp(token, "<") == 0)
        {
            char *filename = strvec_get(tokens, i + 1);

            int fd = open(filename, O_RDONLY);

            if (fd == -1)
            {
                perror("Failed to open input file");
                return -1;
            }

            if (dup2(fd, STDIN_FILENO) == -1)
            {
                perror("dup2");
                close(fd);
                return -1;
            }
            if (close(fd) == -1)
            {
                perror("close");
                return -1;
            }
            i++; // for skipping the next tokenvector/filename in the next iteration
        }
        else if (strcmp(token, ">") == 0)
        {
            char *filename = strvec_get(tokens, i + 1);

            int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
            if (fd == -1)
            {
                perror("Failed to open input file");
                return -1;
            }
            if (dup2(fd, STDOUT_FILENO) == -1)
            {
                perror("dup2");
                close(fd);
                return -1;
            }
            if (close(fd) == -1)
            {
                perror("close");
                return -1;
            }
            i++; // for skipping the next tokenvector/filename in the next iteration
        }
        else if (strcmp(token, ">>") == 0)
        {
            char *filename = strvec_get(tokens, i + 1);

            int fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
            if (fd == -1)
            {
                perror("Failed to open output file");
                return -1;
            }
            if (dup2(fd, STDOUT_FILENO) == -1)
            {
                perror("dup2");
                close(fd);
                return -1;
            }
            if (close(fd) == -1)
            {
                perror("close");
                return -1;
            }
            i++; // for skipping the next tokenvector/filename in the next iteration
        }
        else
        {
            args[arg_index] = token;
            arg_index++;
        }
    }
    // Task4
    struct sigaction sa;
    sa.sa_handler = SIG_DFL;

    if (sigfillset(&sa.sa_mask) == -1)
    {
        perror("sigfillset");
        return -1;
    }

    sa.sa_flags = 0;

    // restore the signal handlers to their def values
    if (sigaction(SIGTTIN, &sa, NULL) == -1 || sigaction(SIGTTOU, &sa, NULL) == -1)
    {
        perror("sigaction");
        return -1;
    }

    // puts child into their own pgrps
    pid_t pid = getpid();

    if (setpgid(pid, pid) == -1)
    {
        perror("setpgid");
        return -1;
    }
    execvp(args[0], args);
    perror("exec");
    return -1;

    // TODO Task 3: Extend this function to perform output redirection before exec()'ing
    // Check for '<' (redirect input), '>' (redirect output), '>>' (redirect and append output)
    // entries inside of 'tokens' (the strvec_find() function will do this for you)
    // Open the necessary file for reading (<), writing (>), or appending (>>)
    // Use dup2() to redirect stdin (<), stdout (> or >>)
    // DO NOT pass redirection operators and file names to exec()'d program
    // E.g., "ls -l > out.txt" should be exec()'d with strings "ls", "-l", NULL

    // TODO Task 4: You need to do two items of setup before exec()'ing
    // 1. Restore the signal handlers for SIGTTOU and SIGTTIN to their defaults.
    // The code in main() within swish.c sets these handlers to the SIG_IGN value.
    // Adapt this code to use sigaction() to set the handlers to the SIG_DFL value.
    // 2. Change the process group of this process (a child of the main shell).
    // Call getpid() to get its process ID then call setpgid() and use this process
    // ID as the value for the new process group ID

    return 0;
}

int resume_job(strvec_t *tokens, job_list_t *jobs, int is_foreground)
{
    // TODO Task 5: Implement the ability to resume stopped jobs in the foreground
    // 1. Look up the relevant job information (in a job_t) from the jobs list
    //    using the index supplied by the user (in tokens index 1)
    const char *index_str = strvec_get(tokens, 1);
    //    Feel free to use sscanf() or atoi() to convert this string to an int
    int index_num = atoi(index_str);
    // 2. Call tcsetpgrp(STDIN_FILENO, <job_pid>) where job_pid is the job's process ID
    job_t *job = job_list_get(jobs, index_num);
    if (job == NULL)
    {
        fprintf(stderr, "Job index out of bounds!\n");
        return -1;
    }
    // 3. Send the process the SIGCONT signal with the kill() system call
    // 4. Use the same waitpid() logic as in main -- don't forget WUNTRACED
    // 5. If the job has terminated (not stopped), remove it from the 'jobs' list
    // 6. Call tcsetpgrp(STDIN_FILENO, <shell_pid>). shell_pid is the *current*
    //    process's pid, since we call this function from the main shell process
    if (is_foreground)
    {
        // handling
        if (tcsetpgrp(STDIN_FILENO, job->pid) == -1)
        {
            perror("tcsetpgrp");
            return -1;
        }

        if (kill(job->pid, SIGCONT) == -1)
        {
            perror("kill");
            return -1;
        }

        int status;
        if (waitpid(job->pid, &status, WUNTRACED) == -1)
        {
            perror("waitpid");
        }

        // if terminated, remove
        if (!WIFSTOPPED(status))
        {
            job_list_remove(jobs, index_num);
        }

        if (tcsetpgrp(STDIN_FILENO, getpid()) == -1)
        {
            perror("tcsetpgrp");
            return -1;
        }
    }
    else
    {
        // TODO Task 6: Implement the ability to resume stopped jobs in the background.
        // This really just means omitting some of the steps used to resume a job in the foreground:
        // 1. DO NOT call tcsetpgrp() to manipulate foreground/background terminal process group
        // 2. DO NOT call waitpid() to wait on the job
        // 3. Make sure to modify the 'status' field of the relevant job list entry to BACKGROUND
        //    (as it was STOPPED before this)
        if (kill(job->pid, SIGCONT) == -1)
        {
            perror("kill");
            return -1;
        }
        job->status = BACKGROUND;
    }

    return 0;
}

int await_background_job(strvec_t *tokens, job_list_t *jobs)
{
    // TODO Task 6: Wait for a specific job to stop or terminate
    // 1. Look up the relevant job information (in a job_t) from the jobs list
    //    using the index supplied by the user (in tokens index 1)
    const char *index_str = strvec_get(tokens, 1);
    int index_num = atoi(index_str);
    job_t *job = job_list_get(jobs, index_num);
    if (job == NULL)
    {
        fprintf(stderr, "Job index out of bounds!\n");
        return -1;
    }
    // 2. Make sure the job's status is BACKGROUND (no sense waiting for a stopped job)
    int status;
    if (job->status != BACKGROUND)
    {
        fprintf(stderr, "Job index is for stopped stopped process not background process!\n");
    }
    // 3. Use waitpid() to wait for the job to terminate, as you have in resume_job() and main().
    if (waitpid(job->pid, &status, WUNTRACED) == -1)
    {
        perror("waitpid");
        return -1;
    }
    // 4. If the process terminates (is not stopped by a signal) remove it from the jobs list
    if (!WIFSTOPPED(status))
    {
        job_list_remove(jobs, index_num);
    }

    return 0;
}

int await_all_background_jobs(job_list_t *jobs)
{
    job_t *cur_head = jobs->head;
    // TODO Task 6: Wait for all background jobs to stop or terminate
    // 1. Iterate through the jobs list, ignoring any stopped jobs
    while (cur_head != NULL)
    {
        if (cur_head->status == BACKGROUND)
        {
            int status;
            // 2. For a background job, call waitpid() with WUNTRACED.
            if (waitpid(cur_head->pid, &status, WUNTRACED) == -1)
            {
                perror("waitpid");
                return -1;
            }
            // 3. If the job has stopped (check with WIFSTOPPED), change its
            //    status to STOPPED. If the job has terminated, do nothing until the
            //    next step (don't attempt to remove it while iterating through the list).
            if (WIFSTOPPED(status))
            {
                cur_head->status = STOPPED;
            }
        }
        cur_head = cur_head->next;
    }

    // 4. Remove all background jobs (which have all just terminated) from jobs list.
    //    Use the job_list_remove_by_status() function.
    job_list_remove_by_status(jobs, BACKGROUND);

    return 0;
}
