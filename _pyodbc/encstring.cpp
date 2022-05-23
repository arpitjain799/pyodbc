
#include "pyodbc.h"
#include "encstring.h"


bool EncodedString::encode(PyObject* src, const char *encoding) {
  PyObject* pT = PyUnicode_AsEncodedString(src, encoding, "strict");
  if (!pT)
    return false;

  Py_XDECREF(pobj);
  pobj = pT;

  pb = PyBytes_AsString(pobj);
  cb = PyUnicode_GetLength(src);

  return true;
}
