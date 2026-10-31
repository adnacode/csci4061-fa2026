#define _GNU_SOURCE

#include <Python.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "hadamard.h"

/*
 * Function executed in each child process to compute its portion of the hadamard product
 * 'a': Pointer to shared memory segment storing first matrix operand
 * 'b': Pointer to shared memory segment storing second matrix operand
 * 'c': Pointer to shared memory segment storing result matrix
 * 'start': Index (in flattened representation) of first element of matrix portion
 * 'end': Index (in flattened representation) of last element of matrix portion
 * 'mapped_size': Size (in bytes) of each shared memory segment
 *
 * Returns 0 on success or -1 on failure
 */
// TODO: Uncomment the function definition below and complete it
static int worker(int *a, int *b, int *c, unsigned start, unsigned end, unsigned mapped_size) {
//     /*
//      * TODO Implement this function
//      * Multiply each pair of elements from the assigned portion of the input
//      * matrices and store in the result matrix.
//      * Don't forget error handling and cleanup!
//      */
//
    for (unsigned i = start; i < end; i++) {
        c[i] = a[i] * b[i];
    }
    int result = 0;
    if (munmap(c, mapped_size) == -1) {
        result = -1;
    }
    if (munmap(b, mapped_size) == -1) {
        result = -1;
    }
    if (munmap(a, mapped_size) == -1) {
        result = -1;
    }

    return result;
}

PyObject *hadamard_shmem(PyObject *self, PyObject *args) {
    /*
     * TODO Implement this function
     * Read in arguments passed from Python
     * Read the two operand matrices from their files into shared memory
     * Set up shared memory segment to hold result matrix
     * Fork 'n_workers' children, giving each a roughly equal portion of the
     * input matrices to work on
     * Wait for all children to complete their work back in the parent
     * Then pass the result matrix to print_matrix()
     *
     * Don't forget error handling and cleanup!
     */
    const char *mat_a_file;
    const char *mat_b_file;
    unsigned n_procs;
    if (!PyArg_ParseTuple(args, "ssI", &mat_a_file, &mat_b_file, &n_procs)) {
        // ParseTuple sets up a Python exception on our behalf
        return NULL;
    }

    unsigned a_rows;
    unsigned a_cols;
    int *mat_a_data = read_matrix_from_file(mat_a_file, &a_rows, &a_cols, 1);
    if (mat_a_data == NULL) {
        PyErr_SetString(PyExc_OSError, "Failed to read matrix data from file");
        return NULL;
    }
    unsigned b_rows;
    unsigned b_cols;
    int *mat_b_data = read_matrix_from_file(mat_b_file, &b_rows, &b_cols, 1);
    if (mat_b_data == NULL) {
        PyErr_SetString(PyExc_OSError, "Failed to read matrix data from file.");
        munmap(mat_a_data, a_rows * a_cols * sizeof(int));
        return NULL;
    }
    assert(a_rows == b_rows && a_cols == b_cols);

    /*
     * TODO Complete the implementation of this function
     * Create one pipe for all processes to share
     * Fork 'n_workers' child processes
     * Assign each child a roughly equal portion of matrix to work on
     * Set up a result matrix to store matrix elements sent by children
     * Collect results in the parent via the pipe
     * Don't forget cleanup and error handling!
     */
     unsigned total = a_rows * a_cols;
     unsigned mapped_size = total * sizeof(int);

     int *mat_c_data = mmap(NULL, mapped_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
     if (mat_c_data == MAP_FAILED) {
        PyErr_SetFromErrno(PyExc_OSError);
        munmap(mat_a_data, mapped_size);
        munmap(mat_b_data, mapped_size);
        return NULL;
     }

     pid_t *pids = malloc(n_procs * sizeof(pid_t));
     if (pids == NULL) {
        PyErr_SetString(PyExc_OSError, "Failed to malloc");
        munmap(mat_c_data, mapped_size);
        munmap(mat_b_data, mapped_size);
        munmap(mat_a_data, mapped_size);
        return NULL;
     }

     unsigned chuck = (unsigned)ceil((double)total / n_procs);

     unsigned n_forked = 0;
     int not_fork_failed = 0;
     for (unsigned i = 0; i < n_procs; i++) {
        pid_t pid = fork();
        if (pid == -1) {
            PyErr_SetFromErrno(PyExc_OSError);
            not_fork_failed = 1;
            break;
        }
        if (pid == 0) {
            unsigned start = min(i * chuck, total);
            unsigned end = min(start + chuck, total);
            int result = worker(mat_a_data, mat_b_data, mat_c_data, start, end, mapped_size);
            free(pids);

            _exit(result == 0 ? 0 : 1);
        }
        pids[i] = pid;
        n_forked++;
     }

     for (unsigned i =0; i < n_forked; i++) {
        int status;
        if (waitpid(pids[i], &status, 0) == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            if (!not_fork_failed) {
                PyErr_SetString(PyExc_OSError, "A worker process failed");
            }
            not_fork_failed = 1;
        }
     }

     if (!not_fork_failed) {
        print_matrix(mat_c_data, a_rows, a_cols);
     }

     free(pids);
     munmap(mat_c_data, mapped_size);
     munmap(mat_b_data, mapped_size);
     munmap(mat_a_data, mapped_size);

     if (not_fork_failed) {
        return NULL;
     }

    Py_RETURN_NONE;
}
