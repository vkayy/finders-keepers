#include "getspeech.h"
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
    pFunc = PyObject_GetAttrString(pModule, "listenDir");

    // pFunc is a new reference
    if (pFunc && PyCallable_Check(pFunc)) {
      while (1) {
        pValue = PyObject_CallObject(pFunc, NULL);

        if (pValue != NULL) {
          int res = PyLong_AsLong(pValue);

          printf("Result of call: %d\n", res);
          return res;
          Py_DECREF(pValue);
        } else {
          Py_DECREF(pFunc);
          Py_DECREF(pModule);
          PyErr_Print();
          fprintf(stderr, "Call failed\n");
          return 0;
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

}

int getChoice(void) {
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
        pFunc = PyObject_GetAttrString(pModule, "listenDir");

        // pFunc is a new reference
        if (pFunc && PyCallable_Check(pFunc)) {
            while (1) {
                pValue = PyObject_CallObject(pFunc, NULL);

                if (pValue != NULL) {
                    int res = PyLong_AsLong(pValue);

                    printf("Result of call: %d\n", res);
                    return res;
                    Py_DECREF(pValue);
                } else {
                    Py_DECREF(pFunc);
                    Py_DECREF(pModule);
                    PyErr_Print();
                    fprintf(stderr, "Call failed\n");
                    return 0;
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

}
