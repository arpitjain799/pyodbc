
#pragma once

class EncodedString {
private:
  const char* pb;
  Py_ssize_t cb;
  // The logical *character* length (the length of the original Python string before encoding).
  PyObject* pobj;
  // Optional bytes object if one was needed to perform the conversion.  If not zero, `pb`
  // is owned by this object.  This object is owned by this class and must be decrefed.

public:
  EncodedString() {
    pb = 0;
    cb = 0;
    pobj = 0;
  }

  ~EncodedString() {
    Py_XDECREF(pobj);
  }

  bool encode(PyObject* src, const char *encoding);
};
