#include "Python.h"
#include <stdio.h>
#include "hako_asset.h"
#include "hako_conductor.h"

#ifdef __cplusplus
extern "C" {
#endif

PyObject* py_on_initialize_callback = NULL;
PyObject* py_on_simulation_step_callback = NULL;
PyObject* py_on_manual_timing_control_callback = NULL;
PyObject* py_on_reset_callback = NULL;

static int callback_wrapper(PyObject* callback, hako_asset_context_t* context) {
    if (callback != NULL) {
        PyObject* context_capsule;
        if (context != NULL) {
            context_capsule = PyCapsule_New(context, NULL, NULL);
            if (!context_capsule) {
                PyErr_Print();
                return -1;
            }
        }
        else {
            context_capsule = Py_None;
        }

        PyObject* arglist = Py_BuildValue("(O)", context_capsule);
        Py_DECREF(context_capsule);

        if (!arglist) {
            PyErr_Print();
            return -1;
        }

        PyObject* result = PyObject_CallObject(callback, arglist);
        Py_DECREF(arglist);

        if (result == NULL) {
            PyErr_Print();
            return -1;
        }

        long c_result = PyLong_AsLong(result);
        Py_DECREF(result);

        if (PyErr_Occurred()) {
            PyErr_Print();
            return -1;
        }

        return (int)c_result;
    }
    return 0;
}

static int on_initialize_wrapper(hako_asset_context_t* context)
{
    return callback_wrapper(py_on_initialize_callback, context);
}
static int on_reset_wrapper(hako_asset_context_t* context)
{
    return callback_wrapper(py_on_reset_callback, context);
}
static int on_simulation_step_wrapper(hako_asset_context_t* context)
{
    return callback_wrapper(py_on_simulation_step_callback, context);
}
static int on_manual_timing_control_wrapper(hako_asset_context_t* context)
{
    return callback_wrapper(py_on_manual_timing_control_callback, context);
}

static struct hako_asset_callbacks_s hako_asset_callback_python;

//asset_register
static PyObject* asset_register(PyObject*, PyObject* args) {
    const char *asset_name;
    const char *config_path;
    PyObject *callbacks_dict;
    long delta_usec;
    int model;

    if (!PyArg_ParseTuple(args, "ssOli", &asset_name, &config_path, &callbacks_dict, &delta_usec, &model)) {
        return NULL;
    }

    if (!PyDict_Check(callbacks_dict)) {
        PyErr_SetString(PyExc_TypeError, "callbacks must be a dictionary");
        return NULL;
    }
    py_on_initialize_callback = PyDict_GetItemString(callbacks_dict, "on_initialize");
    Py_XINCREF(py_on_initialize_callback);
    hako_asset_callback_python.on_initialize = on_initialize_wrapper;
    py_on_reset_callback = PyDict_GetItemString(callbacks_dict, "on_reset");
    Py_XINCREF(py_on_reset_callback);
    hako_asset_callback_python.on_reset = on_reset_wrapper;

    hako_asset_callback_python.on_simulation_step = on_simulation_step_wrapper;
    hako_asset_callback_python.on_manual_timing_control = on_manual_timing_control_wrapper;

    PyObject* temp;

    temp = PyDict_GetItemString(callbacks_dict, "on_simulation_step");
    if (temp == Py_None) {
        hako_asset_callback_python.on_simulation_step = NULL;
    } else if (temp != NULL) {
        Py_XINCREF(temp);
        Py_XDECREF(py_on_simulation_step_callback);
        py_on_simulation_step_callback = temp;
        hako_asset_callback_python.on_simulation_step = on_simulation_step_wrapper;
    }

    temp = PyDict_GetItemString(callbacks_dict, "on_manual_timing_control");
    if (temp == Py_None) {
        hako_asset_callback_python.on_manual_timing_control = NULL;
    } else if (temp != NULL) {
        Py_XINCREF(temp);
        Py_XDECREF(py_on_manual_timing_control_callback);
        py_on_manual_timing_control_callback = temp;
        hako_asset_callback_python.on_manual_timing_control = on_manual_timing_control_wrapper;
    }

    int result = hako_asset_register(asset_name, config_path, &hako_asset_callback_python, (hako_time_t)delta_usec, (HakoAssetModelType)model);

    if (result == 0) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}

static PyObject* py_hako_asset_start(PyObject*, PyObject* args) {
    if (!PyArg_ParseTuple(args, "")) {
        return NULL;
    }

    int result = hako_asset_start();
    if (result == 0) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}
static PyObject* py_hako_asset_simulation_time(PyObject*, PyObject*) {
    hako_time_t sim_time = hako_asset_simulation_time();

    return PyLong_FromLongLong(sim_time);
}
static PyObject* py_hako_asset_usleep(PyObject*, PyObject* args) {
    hako_time_t sleep_time_usec;

    if (!PyArg_ParseTuple(args, "L", &sleep_time_usec)) {
        return NULL;
    }

    int result = hako_asset_usleep(sleep_time_usec);

    if (result == 0) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}

static PyObject* py_hako_asset_pdu_read(PyObject*, PyObject* args) {
    const char* robo_name;
    HakoPduChannelIdType lchannel;
    Py_ssize_t buffer_len;

    if (!PyArg_ParseTuple(args, "siL", &robo_name, &lchannel, &buffer_len)) {
        return NULL;
    }

    char* buffer = (char*)malloc(buffer_len * sizeof(char));
    if (buffer == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    int result = hako_asset_pdu_read(robo_name, lchannel, buffer, (size_t)buffer_len);

    if (result != 0) {
        free(buffer);
        PyErr_Format(PyExc_RuntimeError, "hako_asset_pdu_read failed with error code: %d", result);
        return NULL;
    }

    PyObject* py_data = PyByteArray_FromStringAndSize(buffer, buffer_len);
    free(buffer);
    return py_data;
}

static PyObject* py_hako_asset_pdu_write(PyObject*, PyObject* args) {
    PyObject* py_pdu_data;
    char *robo_name;
    HakoPduChannelIdType lchannel;
    size_t len;
    if (!PyArg_ParseTuple(args, "siYL", &robo_name, &lchannel, &py_pdu_data, &len))
    {
        return NULL;
    }
    char* pdu_data = PyByteArray_AsString(py_pdu_data);
    int ret = hako_asset_pdu_write(robo_name, lchannel, pdu_data, len);
    if (ret == 0) {
        return Py_BuildValue("O", Py_True);
    }
    else {
        return Py_BuildValue("O", Py_False);
    }
}
static PyObject* py_hako_asset_pdu_create(PyObject*, PyObject* args) {
    const char* robo_name;
    HakoPduChannelIdType lchannel;
    size_t pdu_size;

    if (!PyArg_ParseTuple(args, "siL", &robo_name, &lchannel, &pdu_size)) {
        return NULL;
    }

    int result = hako_asset_pdu_create(robo_name, lchannel, pdu_size);

    if (result == 0) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}
static PyObject* py_hako_conductor_start(PyObject*, PyObject* args) {
    hako_time_t delta_usec, max_delay_usec;

    if (!PyArg_ParseTuple(args, "LL", &delta_usec, &max_delay_usec)) {
        return NULL;
    }

    int result = hako_conductor_start(delta_usec, max_delay_usec);
    if (result == 0) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}

static PyObject* py_hako_conductor_stop(PyObject*, PyObject*) {
    hako_conductor_stop();
    Py_RETURN_NONE;
}
static PyObject* py_init_for_external(PyObject*, PyObject*) {
    int result = hako_initialize_for_external();
    if (result == 0) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}

static PyMethodDef hako_asset_python_methods[] = {
    {"asset_register", asset_register, METH_VARARGS, "Register asset"},
    {"init_for_external", py_init_for_external, METH_NOARGS, "Initialize for external"},
    {"pdu_create", py_hako_asset_pdu_create, METH_VARARGS, "Create PDU data for the specified robot name and channel ID."},
    {"start", py_hako_asset_start, METH_VARARGS, "Start the asset."},
    {"simulation_time", py_hako_asset_simulation_time, METH_NOARGS, "Get the current simulation time."},
    {"usleep", py_hako_asset_usleep, METH_VARARGS, "Sleep for the specified time in microseconds."},
    {"pdu_read", py_hako_asset_pdu_read, METH_VARARGS, "Read PDU data for the specified robot name and channel ID."},
    {"pdu_write", py_hako_asset_pdu_write, METH_VARARGS, "Write PDU data for the specified robot name and channel ID."},
    {"conductor_start", py_hako_conductor_start, METH_VARARGS, "Start the conductor with specified delta and max delay usec."},
    {"conductor_stop", py_hako_conductor_stop, METH_NOARGS, "Stop the conductor."},
    { NULL,  NULL,  0, NULL},
};
//module creator
PyMODINIT_FUNC PyInit_hakopy(void)
{
    static struct PyModuleDef hako_obj;
    hako_obj.m_base = PyModuleDef_HEAD_INIT;
    hako_obj.m_name = "hako_asset_python";
    hako_obj.m_doc = "Python3 C API Module(Hakoniwa)";
    hako_obj.m_methods = hako_asset_python_methods;
    hako_obj.m_slots = NULL;
    PyObject* m = PyModule_Create(&hako_obj);
    if (m == NULL)
        return NULL;

    PyModule_AddIntConstant(m, "HAKO_ASSET_MODEL_PLANT", HAKO_ASSET_MODEL_PLANT);
    PyModule_AddIntConstant(m, "HAKO_ASSET_MODEL_CONTROLLER", HAKO_ASSET_MODEL_CONTROLLER);

    return m;
}
#ifdef __cplusplus
}
#endif
