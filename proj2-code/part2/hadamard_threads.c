#define _GNU_SOURCE

#include <Python.h>
#include <assert.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hadamard.h"

/*
 * Represents arguments for each thread
 * 'a': Pointer to first matrix operand
 * 'b': Pointer to second matrix operand
 * 'c': Pointer to result matrix
 * 'start': Index (in flattened representation) of first element of matrix portion
 * 'end': Index (in flattened representation) of second element of matrix portion
 */
struct thread_args {
    int *a;
    int *b;
    int *c;
    unsigned start;
    unsigned end;
};

/*
 * Function executed in each thread to compute its portion of the hadamard product
 * 'data': Pointer to struct thread_args instance
 * Declared here so this function can be referenced in hadamard_threads().
 * Complete the definition at the bottom of this file.
 * Returns NULL
 */
// TODO Uncomment this function definition and complete it
static void *worker(void *data) {
//     /*
//      * TODO Complete this function
//      * Multiply each pair of elements and store the result, for the assigned
//      * portion of the input matrices.
//      */
    struct thread_args *t = (struct thread_args *) data;
    for (unsigned i = t->start; i < t->end; i++) {
        t->c[i] = t->a[i] * t->b[i];
    }
     return NULL;
}

PyObject *hadamard_threads(PyObject *self, PyObject *args) {
    /*
     * TODO Complete this function
     * Read arguments passed in by Python
     * Read the two input matrices from their files
     * Set up a result matrix on the heap
     * Launch 'n_workers' threads, each assigned a roughly equal portion of the
     * matrices to work on.
     * Wait for all threads to complete, then pass the result matrix to the
     * print_matrix() function.
     * Don't forget error handling and cleanup!
     */
    const char *mat_a_file;
    const char *mat_b_file;
    unsigned n_threads;
    if (!PyArg_ParseTuple(args, "ssI", &mat_a_file, &mat_b_file, &n_threads)) {
        // ParseTuple sets up a Python exception on our behalf
        return NULL;
    }

    unsigned a_rows;
    unsigned a_cols;
    int *mat_a_data = read_matrix_from_file(mat_a_file, &a_rows, &a_cols, 0);
    if (mat_a_data == NULL) {
        PyErr_SetString(PyExc_OSError, "Failed to read matrix data from file");
        return NULL;
    }
    unsigned b_rows;
    unsigned b_cols;
    int *mat_b_data = read_matrix_from_file(mat_b_file, &b_rows, &b_cols, 0);
    if (mat_b_data == NULL) {
        PyErr_SetString(PyExc_OSError, "Failed to read matrix data from file.");
        free(mat_a_data);
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

     int *mat_c_data = malloc(total * sizeof(int));
     pthread_t *threads = malloc(n_threads * sizeof(pthread_t));

     struct thread_args *thread_data = malloc(n_threads * sizeof(struct thread_args));

     if (mat_c_data == NULL || threads == NULL || thread_data == NULL) {
        PyErr_SetString(PyExc_OSError, "Failed to malloc");
        free(mat_a_data);
        free(mat_b_data);
        free(mat_c_data);
        free(threads);
        free(thread_data);
        return NULL;
     }

     unsigned chuck = (unsigned)ceil((double)total / n_threads);

     unsigned n_created = 0;
     int failed = 0;
     for (unsigned i = 0; i < n_threads; i++) {
        thread_data[i].a = mat_a_data;
        thread_data[i].b = mat_b_data;
        thread_data[i].c = mat_c_data;

        thread_data[i].start = min(i* chuck, total);
        thread_data[i].end = min(thread_data[i].start + chuck, total);

        if(pthread_create(&threads[i], NULL, worker, &thread_data[i]) != 0) {
            PyErr_SetString(PyExc_OSError, "Failed to make thread");
            failed = 1;
            break;
        }
        n_created++;
     }

     for (unsigned i =0; i < n_created; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            if (!failed) {
                PyErr_SetString(PyExc_OSError, "Failed to join thread");
            }
            failed = 1;
        }
     }

     if (!failed) {
        print_matrix(mat_c_data, a_rows, a_cols);
     }

     free(mat_c_data);
     free(mat_b_data);
     free(mat_a_data);

     free(threads);
     free(thread_data);
     if (failed) {
        return NULL;
     }

    Py_RETURN_NONE;
}
