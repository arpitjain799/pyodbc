
#pragma once

#ifndef PYODBC_H
#define PYODBC_H

#define Py_LIMITED_API 0x03070000
#define PY_SSIZE_T_CLEAN 1


#ifdef _MSC_VER
  // The MS headers generate a ton of warnings.
  #pragma warning(push, 0)
  #define _CRT_SECURE_NO_WARNINGS
  #include <windows.h>
  #include <malloc.h>
  #pragma warning(pop)
  typedef __int64 INT64;
  typedef unsigned __int64 UINT64;
  #ifdef __MINGW32__
    #include <windef.h>
    #include <malloc.h>
  #else
    inline int max(int lhs, int rhs) { return (rhs > lhs) ? rhs : lhs; }
  #endif
#endif


#include <Python.h>
/*  #include <floatobject.h>  */
/*  #include <longobject.h>  */
/*  #include <boolobject.h>  */
/*  #include <unicodeobject.h>  */
/*  #include <structmember.h>  */

#include <sql.h>
#include <sqlext.h>

#ifdef UNUSED
#undef UNUSED
#endif
inline void UNUSED(...) { }


typedef unsigned short ODBCCHAR;
// I'm not sure why, but unixODBC seems to define SQLWCHAR as wchar_t even with
// the size is incorrect.  So we might get 4-byte SQLWCHAR on 64-bit Linux even
// though it requires 2-byte characters.  We have to define our own type to
// operate on.

enum {
    ODBCCHAR_SIZE = 2
};


#endif // PYODBC_H
