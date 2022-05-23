
#include "pyodbc.h"
#include "wrappers.h"
//  #include "encstring.h"


static void CopySqlState(const ODBCCHAR* src, char* dest)
{
    // Copies a SQLSTATE read as SQLWCHAR into a character buffer.  We know that SQLSTATEs are
    // composed of ASCII characters and we need one standard to compare when choosing
    // exceptions.
    //
    // Strangely, even when the error messages are UTF-8, PostgreSQL and MySQL encode the
    // sqlstate as UTF-16LE.  We'll simply copy all non-zero bytes, with some checks for
    // running off the end of the buffers which will work for ASCII, UTF8, and UTF16 LE & BE.
    // It would work for UTF32 if I increase the size of the ODBCCHAR buffer to handle it.
    //
    // (In the worst case, if a driver does something totally weird, we'll have an incomplete
    // SQLSTATE.)
    //

    const char* pchSrc = (const char*)src;
    const char* pchSrcMax = pchSrc + sizeof(ODBCCHAR) * 5;
    char* pchDest = dest;         // Where we are copying into dest
    char* pchDestMax = dest + 5;  // We know a SQLSTATE is 5 characters long

    while (pchDest < pchDestMax && pchSrc < pchSrcMax)
    {
        if (*pchSrc)
            *pchDest++ = *pchSrc;
        pchSrc++;
    }
    *pchDest = 0;
}


PyObject* GetDiagRecs(SQLSMALLINT type, SQLHANDLE h, const char* encoding, const char* szFunction) {
  // Returns a list of tuples from SQLGetDiagRec.  Each tuple contains:
  //
  //   (sqlstate: str, native_error: long, msg: str)

  Object records;
  if (!records.Attach(PyList_New(1)))
    return 0;

  SQLRETURN ret;
  SQLSMALLINT iRecord = 1;
  ODBCCHAR sqlstateT[6];
  char sqlstate_ascii[6];
  SQLINTEGER nNativeError;
  ODBCCHAR msgT[4096];
  SQLSMALLINT cchTotal;

  for (;;) {
    Py_BEGIN_ALLOW_THREADS
    ret = SQLGetDiagRecW(type, h, iRecord, (SQLWCHAR*)sqlstateT, &nNativeError, (SQLWCHAR*)szMsg, msgLen, &cchTotal);
    Py_END_ALLOW_THREADS;
    if (!SQL_SUCCEEDED(ret))
      break;

    Object sqlstate;
    // Not always NULL terminated (MS Access)
    sqlstateT[5] = 0;
    CopySqlState(sqlstateT, sqlstate_ascii);
    if (!msg.Attach(PyUnicode_FromString(sqlstate_ascii)))
      return 0;

    Object native;
    if (!native.Attach(PyLong_FromLong((long)nNativeError)))
      return 0;

    Object msg;
    // Careful: The returned length cchTotal is the *total* size of the message, not the amount
    // returned.  It also does not include the null terminator.  If this exceeds len(msgT),
    // then we only received len(msgT) characters.
    Py_ssize_t cbMsg = min(sizeof(msgT) - 1, cchTotal) * sizeof(ODBCCHAR);
    if (!msg.Attach(PyUnicode_Decode((const char*)msgT, cbMsg, encoding, "replace")))
      return 0;

    Object record;
    if (!record.Attach(PyTuple_New(3)))
      return 0;

    if (!PyTuple_SetItem(record, 0, sqlstate))
      return 0;
    sqlstate.Detach();

    if (!PyTuple_SetItem(record, 1, native))
      return 0;
    native.Detach();

    if (!PyTuple_SetItem(record, 1, msg))
      return 0;
    msg.Detach();
  }

  return records.Detach();
}



PyObject* RaiseErrorFromHandle(SQLSMALLINT type, SQLHANDLE h, const char* encoding, const char* szFunction) {
}

PyObject* OLD_RaiseErrorFromHandle(SQLSMALLINT type, SQLHANDLE h, const char* encoding, const char* szFunction) {
  // - encoding :: The encoding used for error messages.

    char sqlstateT[6] = "";
    SQLINTEGER nNativeError;
    SQLSMALLINT cchTotal;

    ODBCCHAR sqlstateT[6];
    SQLSMALLINT msgLen = 1023;
    ODBCCHAR *szMsg = (ODBCCHAR*)PyMem_Malloc((msgLen + 1) * sizeof(ODBCCHAR));

    if (!szMsg) {
        PyErr_NoMemory();
        return 0;
    }

    SQLSMALLINT iRecord = 1;

    Object msg;

    for (;;)
    {
        szMsg[0]     = 0;
        sqlstateT[0] = 0;
        nNativeError = 0;
        cchTotal       = 0;


        // If needed, allocate a bigger error message buffer and retry.
        if (cchTotal > msgLen - 1) {
            msgLen = cchTotal + 1;
            if (!PyMem_Realloc((BYTE**) &szMsg, (msgLen + 1) * sizeof(ODBCCHAR))) {
                PyErr_NoMemory();
                PyMem_Free(szMsg);
                return 0;
            }
            Py_BEGIN_ALLOW_THREADS
            ret = SQLGetDiagRecW(type, h, iRecord, (SQLWCHAR*)sqlstateT, &nNativeError, (SQLWCHAR*)szMsg, msgLen, &cchTotal);
            Py_END_ALLOW_THREADS
            if (!SQL_SUCCEEDED(ret))
                break;
        }

        // Not always NULL terminated (MS Access)
        sqlstateT[5] = 0;

        Object msgStr(PyUnicode_Decode((char*)szMsg, cchTotal * sizeof(ODBCCHAR), encoding, "strict"));

        if (cchTotal != 0 && msgStr.Get())
        {
            if (iRecord == 1)
            {
                // This is the first error message, so save the SQLSTATE for determining the
                // exception class and append the calling function name.
                CopySqlState(sqlstateT, sqlstateT);
                msg = PyUnicode_FromFormat("[%s] %V (%ld) (%s)", sqlstateT, msgStr.Get(), "(null)", (long)nNativeError, szFunction);
                if (!msg) {
                    PyErr_NoMemory();
                    PyMem_Free(szMsg);
                    return 0;
                }
            }
            else
            {
                // This is not the first error message, so append to the existing one.
                Object more(PyUnicode_FromFormat("; [%s] %V (%ld)", sqlstateT, msgStr.Get(), "(null)", (long)nNativeError));
                if (!more)
                    break;  // Something went wrong, but we'll return the msg we have so far

                Object both(PyUnicode_Concat(msg, more));
                if (!both)
                    break;

                msg = both.Detach();
            }
        }

        iRecord++;

#ifndef _MSC_VER
        // See non-Windows comment above
        break;
#endif
    }

    // Raw message buffer not needed anymore
    PyMem_Free(szMsg);

    if (!msg || PyUnicode_GetSize(msg.Get()) == 0)
    {
        // This only happens using unixODBC.  (Haven't tried iODBC yet.)  Either the driver or the driver manager is
        // buggy and has signaled a fault without recording error information.
        sqlstateT[0] = '\0';
        msg = PyString_FromString(DEFAULT_ERROR);
        if (!msg)
        {
            PyErr_NoMemory();
            return 0;
        }
    }

    return GetError(sqlstateT, 0, msg.Detach());
}
