
#pragma once

struct Object {
  PyObject* ptr;

  Object() {
    ptr = 0;
  }

  operator PyObject*() { return ptr; }

  bool Attach(PyObject* p) {
    // Takes ownership of the pointer *without* incrementing the reference count.  It will
    // decrement the reference count if not detached, so use this to hold a new reference.
    //
    // Returns true if p is non-zero, allowing you to attach and error check at the same time:
    //
    //     if (!x.Attach(func()))
    //         return 0;

    Py_XDECREF(ptr);
    ptr = p;

    return (ptr != 0);
  }

  PyObject* Detach() {
    // Removes the pointer from this object without decrementing it and returns it.
    PyObject* p = ptr;
    ptr = 0;
    return p;
  }

  void IncRef(PyObject* p) {
    // Accepts the pointer and increments the reference count.

    Py_XDECREF(ptr);
    ptr = p;
    Py_XINCREF(ptr);
  }

  ~Object() {
    Py_XDECREF(ptr);
  }
};
