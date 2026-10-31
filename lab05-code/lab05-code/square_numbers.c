#define _GNU_SOURCE
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define NUMBERS_LEN 40
#define min(a, b) ((a) < (b) ? (a) : (b)) // You may find this useful when assigning work chunks

int numbers[NUMBERS_LEN] = {
    75,
    3,
    99,
    36,
    47,
    88,
    93,
    93,
    0,
    96,
    63,
    27,
    12,
    67,
    75,
    22,
    34,
    41,
    61,
    48,
    13,
    100,
    86,
    15,
    87,
    35,
    19,
    92,
    59,
    70,
    73,
    65,
    35,
    35,
    37,
    44,
    3,
    20,
    37,
    27,
};

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage: %s <num_procs>\n", argv[0]);
        return 1;
    }

    // Uncomment the next line to use this variable definition
    int n_procs = atoi(argv[1]);

    if (n_procs < 1)
    {
        perror("num of processes");
        return 1;
    }

    // TODO Fork 'n_procs' worker processes, ensuring they have access to same
    // shared memory segment
    int *squares = mmap(NULL, NUMBERS_LEN * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (squares == MAP_FAILED)
    {
        perror("mmap");
        return 1;
    }
    // Assign each child a disjoint chunk of 'numbers' array to square
    // Chunks should be disjoint and (approximately) evenly sized
    int chunk = (int)ceil((double)NUMBERS_LEN / n_procs);

    for (int p = 0; p < n_procs; p++)
    {
        pid_t pid = fork();
        if (pid < 0)
        {
            perror("fork");
            return 1;
        }
        if (pid == 0)
        {
            int start = min(p * chunk, NUMBERS_LEN);
            int end = min(start + chunk, NUMBERS_LEN);

            for (int i = start; i < end; i++)
            {
                squares[i] = numbers[i] * numbers[i];
            }
            if (munmap(squares, NUMBERS_LEN * sizeof(int)) != 0)
            {
                perror("munmap");
                _exit(1);
            }
            _exit(0);
        }
    }

    for (int p = 0; p < n_procs; p++)
    {
        wait(NULL);
    }
    printf("Squares:\n");
    for (int i = 0; i < NUMBERS_LEN; i++)
    {
        // TODO Print out results from shared mem segment
        printf("%d\n", squares[i]);
    }

    return 0;
}
