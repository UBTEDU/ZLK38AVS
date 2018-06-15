%module hbi
 %{
  typedef uint16_t reg_addr_t;
  #include "../../../vproc_sdk/drivers/hbi/inc/hbi.h"
 %}


%include "stdint.i"

%typemap(in) unsigned char* pData = char*;

/****************************************************************************
 * These typemaps match HBI_open()
 * It allows python to call HBI_open as such:
 *   handle = HBI_open(cfg)
 ****************************************************************************/
/* This creates a "temp" hbi_handle on the HBI_open wrapper function stack
 * numinputs=0 causes the argument to not be required in python land
 * Resulting wrapper code:
 *     hbi_handle_t *arg1 = &temp
 */
%typemap(in, numinputs=0) (hbi_handle_t *pHandle) (hbi_handle_t temp) {
    $1 = &temp;
}
/* This causes the python wrapper to return the handle 
 * After SWIG calls: result=HBI_open(arg1, arg2)
 * Then we change the result to: *arg1 (arg1 dereferenced)
 */
%typemap(argout) (hbi_handle_t *pHandle) {
    /* Blow away any previous result */
    Py_XDECREF($result);
    $result = PyLong_FromUnsignedLong(*$1);
}


/****************************************************************************
 * These typemaps match HBI_write()
 * It allows python to call HBI_write as such:
 *   HBI_write(handle, reg, python_list)
 ****************************************************************************/
%typemap(in) (user_buffer_t *pInput, size_t length) {
    int i;
    if (!PySequence_Check($input)) {
        PyErr_SetString(PyExc_ValueError, "Expecting a sequence type");
        return NULL;
    }
    $2 = PySequence_Size($input);
    $1 = (unsigned char *)malloc($2);
    for (i = 0; i < $2; i++) {
        PyObject *pE = PySequence_GetItem($input, i);
        if (!PyInt_Check(pE)) {
            free($1);
            PyErr_SetString(PyExc_ValueError, "Expecting a list of ints");
            return NULL;
        }
        $1[i] = PyInt_AsLong(pE);
    }
}
%typemap(freearg) (user_buffer_t *pInput, size_t length) {
    if ($1) free($1);
}


/****************************************************************************
 * These typemaps match HBI_read()
 * It allows python to call HBI_read as such:
 *   python_list = HBI_read(handle, reg, num_bytes)
 ****************************************************************************/
/* This typemap matches both the pOutput and length arguments, but only
 * requires (expects) one python argument in replacement ($input)
 * It results in:
 *  arg3 = malloc(arg4)
 *  arg4 = num_bytes
 */
%typemap(in) (user_buffer_t *pOutput, size_t length) {
    if (!PyInt_Check($input)) {
        PyErr_SetString(PyExc_ValueError, "Expecting an size integer");
        return NULL;
    }
    $2 = (size_t)PyInt_AsLong($input);
    if ($2 < 0) {
        PyErr_SetString(PyExc_ValueError, "Positive integer length expected");
        return NULL;
    }
    $1 = (unsigned char *)malloc($2);
}
/* This typemap creates a list out of the pOutput c array and returns it */
%typemap(argout) (user_buffer_t *pOutput, size_t length) {
    PyObject *o = PyList_New($2);
    /* Blow away any previous result */
    Py_XDECREF($result);
    int i;

    for (i = 0; i < $2; i++) {
        PyList_SetItem(o,i,PyInt_FromLong($1[i]));
    }
    $result = o;
    /* Free the memory malloced by the input typemap */
    free($1);
}

/* These typedefs are not defined in hbi.h, but are needed by SWIG */
typedef uint32_t hbi_handle_t;
typedef uint16_t reg_addr_t;
typedef int tw_device_id_t;

/* Everything in here will have a SWIG wrapper implementation */
%include "../../../vproc_sdk/drivers/hbi/inc/hbi.h"
