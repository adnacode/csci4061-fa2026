// SPDX-FileCopyrightText: 2026 John Kolb <jhkolb@umn.edu>
// SPDX-License-Identifier: GPL-3.0-or-later
#include <Python.h>
#include <stdio.h>
#include <math.h>
// Function definition at the end of this file
PyObject *square_roots(PyObject *self, PyObject *args);

static PyMethodDef square_root_methods[] = {
    {
        .ml_name = "square_roots",
        .ml_meth = square_roots,
        .ml_flags = METH_VARARGS,
        .ml_doc = "Compute square roots of all numbers in a list",
    },
    {
        .ml_name = NULL,
        .ml_meth = NULL,
        .ml_flags = 0,
        .ml_doc = NULL,
    }};

static struct PyModuleDef square_root_module = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "square_root_numbers",
    .m_doc = "Compute square roots of all numbers in a list",
    .m_size = -1,
    .m_methods = square_root_methods,
};

PyMODINIT_FUNC PyInit_square_root(void)
{
    return PyModule_Create(&square_root_module);
}

PyObject *square_roots(PyObject *self, PyObject *args)
{
    // TODO Complete this function
    PyObject *input_list = NULL;
    if (!PyArg_ParseTuple(args, "O!", &PyList_Type, &input_list))
    {
        return NULL;
    }

    Py_ssize_t list_len = PyList_Size(input_list);
    PyObject *output_list = PyList_New(list_len);
    if (output_list == NULL)
    {
        return PyErr_NoMemory();
    }

    for (Py_ssize_t i = 0; i < list_len; i++)
    {
        PyObject *current = PyList_GetItem(input_list, i);
        if (!PyLong_Check(current))
        {
            PyErr_Format(PyExc_TypeError, "list item %d is not anumber", i);
            Py_DECREF(output_list);
            return NULL;
        }

        long val = PyLong_AsLong(current);

        if (val < 0)
        {
            PyErr_SetString(PyExc_ValueError, "Cannot compute square root of negative value");
            Py_DECREF(output_list);
            return NULL;
        }

        PyObject *root = PyFloat_FromDouble(sqrt((double)val));
        if (root == NULL)
        {
            Py_DECREF(output_list);
            return NULL;
        }
        PyList_SetItem(output_list, i, root);
    }

    return output_list;
}
