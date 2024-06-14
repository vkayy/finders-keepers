//
// Created by Goh Kai Jet on 14/06/2024.
//
#include "getdirection.h"
#include <Python.h>

int getDirection(void) {
  Py_Initialize();

  // Add the current directory to the Python path
  //    PyRun_SimpleString("import sys");
  //    PyRun_SimpleString("sys.path.append(\".\")");
  //    setenv("PYTHONPATH","/Users/gohkaijet/CLionProjects/CallPythonFromC/venv/lib/python3.9",1);

  // Print Python path for debugging
  PyObject *sys = PyImport_ImportModule("sys");
  PyObject *path = PyObject_GetAttrString(sys, "path");
  PyObject *str_path = PyObject_Str(path);
  const char *c_path = PyUnicode_AsUTF8(str_path);
  printf("Python path: %s\n", c_path);
  Py_DECREF(sys);
  Py_DECREF(path);
  Py_DECREF(str_path);

  // Add the current directory to the Python path
  PyRun_SimpleString("import sys");
  PyRun_SimpleString("sys.path.append(\".\")");

  PyObject *pName, *pModule, *pFunc;
  PyObject *pArgs, *pValue;
  //
  //    PyConfig config;
  //    PyConfig_InitPythonConfig(&config);
  //    config.module_search_paths_set = 1;
  //    PyWideStringList_Append(&config.module_search_paths, L".");
  //    Py_InitializeFromConfig(&config);

  // Load the module object
  // SPECIFY ABSOLUTE PATH IF NOT IN cmake-build-debug

  pName = PyUnicode_FromString("speechrecog");
  pModule = PyImport_Import(pName);
  Py_DECREF(pName);
  if (pModule != NULL) {
    // pFunc is also a new reference
    pFunc = PyObject_GetAttrString(pModule, "listen");

    // pFunc is a new reference
    if (pFunc && PyCallable_Check(pFunc)) {
      // Prepare the arguments for the function call
      //            pArgs = PyTuple_New(2);
      //            PyTuple_SetItem(pArgs, 0, PyLong_FromLong(3)); // First
      //            argument PyTuple_SetItem(pArgs, 1, PyLong_FromLong(4)); //
      //            Second argument
      while (1) {
        pValue = PyObject_CallObject(pFunc, NULL);
        //                Py_DECREF(pArgs);

        if (pValue != NULL) {
          printf("Result of call: %ld\n", PyLong_AsLong(pValue));
          Py_DECREF(pValue);
        } else {
          Py_DECREF(pFunc);
          Py_DECREF(pModule);
          PyErr_Print();
          fprintf(stderr, "Call failed\n");
          return 1;
        }
      }
      // Call the function

    } else {
      if (PyErr_Occurred())
        PyErr_Print();
      fprintf(stderr, "Cannot find function \"%s\"\n", "multiply");
    }
    Py_XDECREF(pFunc);
    Py_DECREF(pModule);
  } else {
    PyErr_Print();
    fprintf(stderr, "Failed to load \"%s\"\n", "mymodule");
    return 1;
  }

  Py_Finalize();
  return 0;

  // Set PYTHONPATH TO working directory
  //    setenv("PYTHONPATH",".",1);
  //
  //    PyObject *pName, *pModule, *pDict, *pFunc, *pValue, *presult;
  //
  //
  //    // Initialize the Python Interpreter
  //    Py_Initialize();
  //
  //
  //    // Build the name object
  //    pName = PyUnicode_DecodeFSDefault((char*)"arbName");
  //
  //    // Load the module object
  //    pModule = PyImport_Import(pName);
  //
  //
  //    // pDict is a borrowed reference
  //    pDict = PyModule_GetDict(pModule);
  //
  //
  //    // pFunc is also a borrowed reference
  //    pFunc = PyDict_GetItemString(pDict, (char*)"someFunction");
  //
  //    if (PyCallable_Check(pFunc))
  //    {
  //        pValue=Py_BuildValue("(z)",(char*)"something");
  //        PyErr_Print();
  //        printf("Let's give this a shot!\n");
  //        presult=PyObject_CallObject(pFunc,pValue);
  //        PyErr_Print();
  //    } else
  //    {
  //        PyErr_Print();
  //    }
  //    printf("Result is %ld\n", PyLong_AsLong(presult));
  //    Py_DECREF(pValue);
  //
  //    // Clean up
  //    Py_DECREF(pModule);
  //    Py_DECREF(pName);
  //
  //    // Finish the Python Interpreter
  //    Py_Finalize();
  //
  //
  //    return 0;
}
