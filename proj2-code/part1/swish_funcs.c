#include "swish_funcs.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "string_vector.h"

#define MAX_ARGS 10

/*
 * Helper function to run a single command within a pipeline. You should make
 * use of the provided 'run_command' function here.
 * tokens: String vector containing the tokens representing the command to be
 * executed, possible redirection, and the command's arguments.
 * pipes: An array of pipe file descriptors.
 * n_pipe_fds: Length of the 'pipes' array
 * in_idx: Index of the file descriptor in the array from which the program
 *         should read its input, or -1 if input should not be read from a pipe.
 * out_idx: Index of the file descriptor in the array to which the program
 *          should write its output, or -1 if output should not be written to
 *          a pipe.
 * Returns 0 on success or -1 on error.
 */
int run_piped_command(strvec_t *tokens, int *pipes, int n_pipe_fds, int in_idx, int out_idx)
{
    // TODO Complete this function's implementation
    // redirects input from pipe if necessary
    if (in_idx != -1)
    {
        if (dup2(pipes[in_idx], STDIN_FILENO) == -1)
        {
            perror("dup2 input");
            return -1;
        }
    }
    // redicts output to pipe if necessary
    if (out_idx != -1)
    {
        if (dup2(pipes[out_idx], STDOUT_FILENO) == -1)
        {
            perror("dup2 output");
            return -1;
        }
    }
    // close all pipe fds
    for (int i = 0; i < n_pipe_fds; i++)
    {
        if (close(pipes[i]) == -1)
        {
            perror("close");
            return -1;
        }
    }
    return run_command(tokens);
}

int run_pipelined_commands(strvec_t *tokens)
{
    // TODO Complete this function's implementation
    int n_commands = strvec_num_occurrences(tokens, "|") + 1;
    int n_pipes = n_commands - 1;
    int n_pipes_fds = n_pipes * 2;

    int *pipes = NULL;
    pid_t *pids = NULL;

    // simple space allocation
    if (n_pipes_fds > 0)
    {
        pipes = malloc(n_pipes_fds * sizeof(int));
        if (pipes == NULL)
        {
            perror("malloc");
            return -1;
        }
    }
    // create pipes to connect the commands
    for (int i = 0; i < n_pipes; i++)
    {
        if (pipe(&pipes[2 * i]) == -1)
        {
            perror("pipe");

            for (int j = 0; j < 2 * i; j++)
            {
                close(pipes[j]);
            }
            free(pipes);
            return -1;
        }
    }
    // allocate space for pids
    pids = malloc(sizeof(pid_t) * n_commands);
    if (pids == NULL)
    {
        perror("malloc");
        // close pipes and free memory if allocation fails
        if (pipes != NULL)
        {
            for (int i = 0; i < n_pipes_fds; i++)
            {
                close(pipes[i]);
            }
            free(pipes);
        }
        return -1;
    }

    int start = 0;
    for (int cmd = 0; cmd < n_commands; cmd++)
    {
        int end = start;
        while (end < (int)tokens->length &&
               strvec_get(tokens, end)[0] != '|')
        {
            end++;
        }
        // extract the tokens for the current command
        strvec_t command_tokens;
        if (strvec_slice(tokens, &command_tokens, start, end) == -1)
        {
            perror("strvec_slice");
            if (pipes != NULL)
            {
                for (int i = 0; i < n_pipes_fds; i++)
                {
                    close(pipes[i]);
                }
                free(pipes);
            }
            free(pids);
            return -1;
        }
        // creat a new process for the current command
        pid_t pid = fork();

        if (pid == -1)
        {
            perror("fork");
            strvec_clear(&command_tokens);

            if (pipes != NULL)
            {
                for (int i = 0; i < n_pipes_fds; i++)
                {
                    close(pipes[i]);
                }
                free(pipes);
            }
            free(pids);
            return -1;
        }
        // child process setting up its pipes
        if (pid == 0)
        {
            int in_idx = -1;
            int out_idx = -1;
            // reading end
            if (cmd > 0)
            {
                in_idx = 2 * (cmd - 1);
            }
            // writing end
            if (cmd < n_commands - 1)
            {
                out_idx = 2 * cmd + 1;
            }
            int result = run_piped_command(&command_tokens, pipes, n_pipes_fds, in_idx, out_idx);
            strvec_clear(&command_tokens);
            _exit(result == 0 ? 0 : 1);
        }

        pids[cmd] = pid;
        strvec_clear(&command_tokens);
        start = end + 1;
    }
    // prarent closes all pipe fds
    if (pipes != NULL)
    {
        for (int i = 0; i < n_pipes_fds; i++)
        {
            close(pipes[i]);
        }
        free(pipes);
    }
    // wait for all child processes to finish
    for (int i = 0; i < n_commands; i++)
    {
        int status;
        if (waitpid(pids[i], &status, 0) == -1)
        {
            perror("waitpid");
            free(pids);
            return -1;
        }
    }
    // free the allocated memory for pids and return success code
    free(pids);
    return 0;
}
