
#include "pyodbc.h"

PyObject* Error = 0;
//  PyObject* Warning;
//  PyObject* InterfaceError;
//  PyObject* DatabaseError;
//  PyObject* InternalError;
//  PyObject* OperationalError;
//  PyObject* ProgrammingError;
PyObject* IntegrityError = 0;
//  PyObject* DataError;
//  PyObject* NotSupportedError;



  //  SQLRETURN SQLDriverConnect(
  //       SQLHDBC         ConnectionHandle,
  //       SQLHWND         WindowHandle,
  //       SQLCHAR *       InConnectionString,
  //       SQLSMALLINT     StringLength1,
  //       SQLCHAR *       OutConnectionString,
  //       SQLSMALLINT     BufferLength,
  //       SQLSMALLINT *   StringLength2Ptr,
  //       SQLUSMALLINT    DriverCompletion);

/*
  static PyObject* connect(PyObject *self, PyObject *args) {
  // Connects to the database and returns the HDBC handle.  Raises an exception on errors.
  //
  // The only expected parameter is the connection string, already converted to bytes.

  HDBC hdbc;
  Py_BEGIN_ALLOW_THREADS
  ret = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
  Py_END_ALLOW_THREADS


  SQLRETURN ret = SQLDriverConnect()
  }
*/


static const char* _pyodbc_alloc_env_doc =
  "alloc_env(pool=True) -> long\n\n"
  "Allocates an HENV and returns it as a Python long.  If successful, be sure to call\n"
  "free_env(henv) when finished with it.\n\n"
  "- pool :: Controls whether connection pooling is enabled.";

static PyObject *_pyodbc_alloc_env(PyObject *self, PyObject *args, PyObject *kwargs) {
  // Returns the HENV as an integer object.

  static char *kwlist[] = {"pool", NULL};

  int pool = 1;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "p", kwlist, &pool))
    return 0;

  if (pool) {
    if (!SQL_SUCCEEDED(SQLSetEnvAttr(SQL_NULL_HANDLE, SQL_ATTR_CONNECTION_POOLING, (SQLPOINTER)SQL_CP_ONE_PER_HENV, sizeof(int)))) {
      PyErr_SetString(PyExc_RuntimeError, "Unable to set SQL_ATTR_CONNECTION_POOLING attribute.");
      return 0;
    }
  }

  HENV henv;

  if (!SQL_SUCCEEDED(SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv))) {
    PyErr_SetString(PyExc_RuntimeError, "Can't initialize module pyodbc.  SQLAllocEnv failed.");
    return 0;
  }

  if (!SQL_SUCCEEDED(SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, sizeof(int)))) {
    PyErr_SetString(PyExc_RuntimeError, "Unable to set SQL_ATTR_ODBC_VERSION attribute.");
    SQLFreeHandle(SQL_HANDLE_ENV, henv);
    return 0;
  }

  return PyLong_FromVoidPtr(henv);
}


static PyObject *_pyodbc_connect(PyObject* self, PyObject* args, PyObject* kwargs) {

  static char *kwlist[] = {"encoding", "autocommit", "timeout", 0};

  unsigned long long _henv;
  PyObject* cstring = 0;
  PyObject* encoding = 0;
  int autocommit = 1;
  long timeout = 0;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "KOs|pl", kwlist, &_henv, &cstring, &encoding, &autocommit, &timeout))
    return 0;

  EncodedString ecstring;
  if (!ecstring.encode(cstring, encoding))
    return 0;

  SQLRETURN ret;
  HDBC hdbc;
  Py_BEGIN_ALLOW_THREADS
  ret = SQLDriverConnectW(hdbc, 0, (SQLCHAR*)ecstring.pb, ecstring.cb, 0, 0, 0, SQL_DRIVER_NOPROMPT);
  Py_END_ALLOW_THREADS;

  if (!SQL_SUCCEEDED(ret)) {
    return RaiseErrorFromHandle(0, "SQLDriverConnect", hdbc, SQL_NULL_HANDLE);
  }

  Py_RETURN_NONE;
}


static bool ImportErrors() {
  // Imports the error classes from the pyodbc.errors.py file so we can raise them here.  I
  // want to keep as much in Python as possible.

  Object errors;
  errors.Attach(PyImport_ImportModule("pyodbc.errors"));

  if (!errors)
    return false;

  Object error;
  error.Attach(PyObject_GetAttrString(errors, "Error"));

  Object interr;
  interr.Attach(PyObject_GetAttrString(errors, "IntegrityError"));

  if (!error || !interr) {
    return false;
  }

  Error = error.Detach();
  IntegrityError = interr.Detach();

  return true;
}


static PyMethodDef PyodbcMethods[] = {
  {"alloc_env", (PyCFunction)_pyodbc_alloc_env, METH_VARARGS|METH_KEYWORDS, _pyodbc_alloc_env_doc},
  {0, 0, 0, 0}
};

static struct PyModuleDef pyodbcmodule = {
  PyModuleDef_HEAD_INIT,
  "_pyodbc",
  0, /* module documentation */
  -1,       /* size of per-interpreter state of the module,
               or -1 if the module keeps state in global variables. */
  PyodbcMethods
};



PyMODINIT_FUNC
PyInit__pyodbc(void)
{
  printf("INIT\n");

  if (!ImportErrors()) {
    return 0;
  }

  PyObject *m = PyModule_Create(&pyodbcmodule);
  if (m == 0)
    return 0;

  return m;
}
