#include "Python.h"
#include <stdio.h>
#include "hako_asset.h"
#include "hako_asset_service.h"
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

static PyObject* recv_event_callback_map = NULL;
static void on_data_recv_event_callback(int recv_event_id)
{
    if (recv_event_callback_map == NULL) {
        return;
    }

    PyObject* key = PyLong_FromLong(recv_event_id);
    PyObject* callback = PyDict_GetItem(recv_event_callback_map, key);
    Py_DECREF(key);

    if (callback && PyCallable_Check(callback)) {
        PyObject* args = Py_BuildValue("(i)", recv_event_id);
        PyObject* result = PyObject_CallObject(callback, args);
        Py_DECREF(args);
        if (!result) {
            PyErr_Print();
        } else {
            Py_DECREF(result);
        }
    }
}

static PyObject* py_hako_asset_register_data_recv_event(PyObject*, PyObject* args) {
    const char* robo_name;
    int lchannel;
    PyObject* callback;

    if (!PyArg_ParseTuple(args, "siO", &robo_name, &lchannel, &callback)) {
        return NULL;
    }
    //None check
    if (callback == Py_None) {
        // nothing to do
    }
    else if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "callback must be callable");
        return NULL;
    }
    int recv_event_id = -1;
    int result = -1;
    if (callback != Py_None) {
        result = hako_asset_register_data_recv_event(robo_name, lchannel, on_data_recv_event_callback, &recv_event_id);
    }
    else {
        result = hako_asset_register_data_recv_event(robo_name, lchannel, NULL, &recv_event_id);
    }
    if (result != 0) {
        PyErr_SetString(PyExc_RuntimeError, "hako_asset_register_data_recv_event failed");
        return NULL;
    }
    if (recv_event_callback_map == NULL) {
        recv_event_callback_map = PyDict_New();
    }
    if (callback != Py_None)
    {
        PyObject* key = PyLong_FromLong(recv_event_id);
        Py_XINCREF(callback);
        PyDict_SetItem(recv_event_callback_map, key, callback);
        Py_DECREF(key);
    }

    return PyLong_FromLong(recv_event_id);
}
static PyObject* py_hako_asset_check_data_recv_event(PyObject*, PyObject* args) {
    const char* robo_name;
    int lchannel;

    if (!PyArg_ParseTuple(args, "si", &robo_name, &lchannel)) {
        return NULL;
    }

    int result = hako_asset_check_data_recv_event(robo_name, lchannel);
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

static PyObject* py_hako_asset_service_initialize(PyObject*, PyObject* args) {
    const char* service_config_path;
    if (!PyArg_ParseTuple(args, "s", &service_config_path)) {
        return NULL;
    }
    int result = hako_asset_service_initialize(service_config_path);
    if (result == 0) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}

//#HAKO_API int hako_asset_service_server_create(const char* assetName, const char* serviceName);
static PyObject* py_hako_asset_service_server_create(PyObject*, PyObject* args) {
    const char* asset_name;
    const char* service_name;

    if (!PyArg_ParseTuple(args, "ss", &asset_name, &service_name)) {
        return NULL;
    }

    int service_id = hako_asset_service_server_create(asset_name, service_name);
    if (service_id < 0) {
        Py_RETURN_NONE;  // または PyErr_SetString() してもOK
    }
    return PyLong_FromLong(service_id);
}

//#HAKO_API int hako_asset_service_server_poll(int service_id);
static PyObject* py_hako_asset_service_server_poll(PyObject*, PyObject* args) {
    int service_id;

    if (!PyArg_ParseTuple(args, "i", &service_id)) {
        return NULL;
    }

    int event = hako_asset_service_server_poll(service_id);
    return PyLong_FromLong(event);
}
//#HAKO_API int hako_asset_service_server_get_current_client_id(int service_id);
static PyObject* py_hako_asset_service_server_get_current_client_id(PyObject*, PyObject* args) {
    int service_id;

    if (!PyArg_ParseTuple(args, "i", &service_id)) {
        return NULL;
    }

    int result = hako_asset_service_server_get_current_client_id(service_id);
    return PyLong_FromLong(result);
}
//#HAKO_API int hako_asset_service_server_get_current_channel_id(int service_id, int* request_channel_id, int* response_channel_id);
static PyObject* py_hako_asset_service_server_get_current_channel_id(PyObject*, PyObject* args) {
    int service_id;
    int request_channel_id;
    int response_channel_id;

    if (!PyArg_ParseTuple(args, "i", &service_id)) {
        return NULL;
    }

    int result = hako_asset_service_server_get_current_channel_id(service_id, &request_channel_id, &response_channel_id);
    if (result == 0) {
        return Py_BuildValue("(ii)", request_channel_id, response_channel_id);
    } else {
        return NULL;
    }
}
//#HAKO_API int hako_asset_service_server_status(int service_id, int* status);
static PyObject* py_hako_asset_service_server_status(PyObject*, PyObject* args) {
    int service_id;
    int status;

    if (!PyArg_ParseTuple(args, "i", &service_id)) {
        return NULL;
    }

    int result = hako_asset_service_server_status(service_id, &status);
    if (result == 0) {
        return PyLong_FromLong(status);
    } else {
        return NULL;
    }
}
//#HAKO_API int hako_asset_service_server_get_request(int service_id, char** packet, size_t* packet_len);
static PyObject* py_hako_asset_service_server_get_request(PyObject*, PyObject* args) {
    int service_id;
    char* packet;
    size_t packet_len;

    if (!PyArg_ParseTuple(args, "i", &service_id)) {
        return NULL;
    }

    int result = hako_asset_service_server_get_request(service_id, &packet, &packet_len);
    if (result == 0) {
        PyObject* py_data = PyByteArray_FromStringAndSize(packet, packet_len);
        return py_data;
    } else {
        return NULL;
    }
}
//#HAKO_API int hako_asset_service_server_get_response_buffer(int service_id, char** packet, size_t* packet_len, int status, int result_code);
static PyObject* py_hako_asset_service_server_get_response_buffer(PyObject*, PyObject* args) {
    int service_id;
    int status;
    int result_code;
    char* packet = NULL;
    size_t packet_len = 0;

    // ← 引数3つ: service_id, status, result_code を受け取るように修正
    if (!PyArg_ParseTuple(args, "iii", &service_id, &status, &result_code)) {
        return NULL;
    }

    int result = hako_asset_service_server_get_response_buffer(
        service_id,
        &packet,
        &packet_len,
        status,
        result_code
    );

    if (result == 0 && packet != NULL) {
        return PyByteArray_FromStringAndSize(packet, packet_len);
    } else {
        Py_RETURN_NONE;
    }
}

//#HAKO_API int hako_asset_service_server_put_response(int service_id, char* packet, size_t packet_len);
static PyObject* py_hako_asset_service_server_put_response(PyObject*, PyObject* args) {
    int service_id;
    PyObject* py_packet;

    if (!PyArg_ParseTuple(args, "iO!", &service_id, &PyByteArray_Type, &py_packet)) {
        return NULL;
    }

    char* packet = PyByteArray_AsString(py_packet);
    Py_ssize_t packet_len = PyByteArray_Size(py_packet);

    if (packet == NULL) {
        PyErr_SetString(PyExc_ValueError, "Invalid bytearray object for packet");
        return NULL;
    }

    int result = hako_asset_service_server_put_response(service_id, packet, (size_t)packet_len);
    if (result == 0) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}

//#HAKO_API int hako_asset_service_server_is_canceled(int service_id);
static PyObject* py_hako_asset_service_server_is_canceled(PyObject*, PyObject* args) {
    int service_id;

    if (!PyArg_ParseTuple(args, "i", &service_id)) {
        return NULL;
    }

    int result = hako_asset_service_server_is_canceled(service_id);
    if (result == 0) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}
//#HAKO_API int hako_asset_service_server_set_progress(int service_id, int percentage);
static PyObject* py_hako_asset_service_server_set_progress(PyObject*, PyObject* args) {
    int service_id;
    int percentage;

    if (!PyArg_ParseTuple(args, "ii", &service_id, &percentage)) {
        return NULL;
    }

    int result = hako_asset_service_server_set_progress(service_id, percentage);
    if (result == 0) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}
static PyObject* py_hako_asset_service_client_create(PyObject*, PyObject* args) {
    const char* asset_name;
    const char* service_name;
    const char* client_name;
    HakoServiceHandleType* handle = (HakoServiceHandleType*)malloc(sizeof(HakoServiceHandleType));
    if (handle == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "sss", &asset_name, &service_name, &client_name)) {
        free(handle);
        return NULL;
    }

    int result = hako_asset_service_client_create(asset_name, service_name, client_name, handle);
    if (result == 0) {
        // Python Capsule に包んで Python に渡す
        PyObject* capsule = PyCapsule_New((void*)handle, "HakoServiceHandleType", NULL);
        if (!capsule) {
            free(handle);
            return NULL;
        }
        return capsule;
    } else {
        free(handle);
        Py_RETURN_NONE;
    }
}

//#HAKO_API int hako_asset_service_client_poll(const HakoServiceHandleType* handle);
static PyObject* py_hako_asset_service_client_poll(PyObject*, PyObject* args) {
    PyObject* capsule;
    if (!PyArg_ParseTuple(args, "O", &capsule)) {
        return NULL;
    }
    HakoServiceHandleType* handle = (HakoServiceHandleType*)PyCapsule_GetPointer(capsule, "HakoServiceHandleType");
    if (handle == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid capsule");
        return NULL;
    }
    int result = hako_asset_service_client_poll(handle);
    if (result == 0) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}
static PyObject* py_hako_asset_service_client_get_current_channel_id(PyObject*, PyObject* args) {
    PyObject* capsule;

    if (!PyArg_ParseTuple(args, "O", &capsule)) {
        return NULL;
    }

    HakoServiceHandleType* handle = (HakoServiceHandleType*)PyCapsule_GetPointer(capsule, "HakoServiceHandleType");
    if (handle == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid capsule for HakoServiceHandleType");
        return NULL;
    }

    int request_channel_id = 0;
    int response_channel_id = 0;

    int result = hako_asset_service_client_get_current_channel_id(handle->service_id, &request_channel_id, &response_channel_id);
    if (result == 0) {
        return Py_BuildValue("(ii)", request_channel_id, response_channel_id);
    } else {
        Py_RETURN_NONE;
    }
}

//#HAKO_API int hako_asset_service_client_get_request_buffer(const HakoServiceHandleType* handle, char** packet, size_t* packet_len, int opcode, int poll_interval_msec);
static PyObject* py_hako_asset_service_client_get_request_buffer(PyObject*, PyObject* args) {
    PyObject* capsule;
    int opcode, poll_interval_msec;
    if (!PyArg_ParseTuple(args, "Oii", &capsule, &opcode, &poll_interval_msec)) {
        return NULL;
    }
    HakoServiceHandleType* handle = (HakoServiceHandleType*)PyCapsule_GetPointer(capsule, "HakoServiceHandleType");
    if (!handle) return NULL;
    
    char* packet = NULL;
    size_t packet_len = 0;
    
    int result = hako_asset_service_client_get_request_buffer(handle, &packet, &packet_len, opcode, poll_interval_msec);
    if (result == 0 && packet != NULL) {
        return PyByteArray_FromStringAndSize(packet, packet_len);
    }
    Py_RETURN_NONE;
}
//#HAKO_API int hako_asset_service_client_call_request(const HakoServiceHandleType* handle, char* packet, size_t packet_len, int timeout_msec);
static PyObject* py_hako_asset_service_client_call_request(PyObject*, PyObject* args) {
    PyObject* capsule;
    PyObject* py_packet;
    int timeout_msec;
    if (!PyArg_ParseTuple(args, "OOi", &capsule, &py_packet, &timeout_msec)) {
        return NULL;
    }
    HakoServiceHandleType* handle = (HakoServiceHandleType*)PyCapsule_GetPointer(capsule, "HakoServiceHandleType");
    if (!handle) return NULL;
    
    char* packet = PyByteArray_AsString(py_packet);
    Py_ssize_t packet_len = PyByteArray_Size(py_packet);
    
    int result = hako_asset_service_client_call_request(handle, packet, packet_len, timeout_msec);
    if (result == 0) Py_RETURN_TRUE;
    Py_RETURN_FALSE;    
}
//#HAKO_API int hako_asset_service_client_get_response(const HakoServiceHandleType* handle, char** packet, size_t* packet_len, int timeout);
static PyObject* py_hako_asset_service_client_get_response(PyObject*, PyObject* args) {
    PyObject* capsule;
    int timeout;
    if (!PyArg_ParseTuple(args, "Oi", &capsule, &timeout)) {
        return NULL;
    }
    HakoServiceHandleType* handle = (HakoServiceHandleType*)PyCapsule_GetPointer(capsule, "HakoServiceHandleType");
    if (!handle) return NULL;
    
    char* packet = NULL;
    size_t packet_len = 0;
    
    int result = hako_asset_service_client_get_response(handle, &packet, &packet_len, timeout);
    if (result == 0 && packet != NULL) {
        return PyByteArray_FromStringAndSize(packet, packet_len);
    }
    Py_RETURN_NONE;
}
//#HAKO_API int hako_asset_service_client_cancel_request(const HakoServiceHandleType* handle);
static PyObject* py_hako_asset_service_client_cancel_request(PyObject*, PyObject* args) {
    PyObject* capsule;
    if (!PyArg_ParseTuple(args, "O", &capsule)) {
        return NULL;
    }
    HakoServiceHandleType* handle = (HakoServiceHandleType*)PyCapsule_GetPointer(capsule, "HakoServiceHandleType");
    if (!handle) return NULL;
    
    int result = hako_asset_service_client_cancel_request(handle);
    if (result == 0) Py_RETURN_TRUE;
    Py_RETURN_FALSE;    
}
//#HAKO_API int hako_asset_service_client_get_progress(const HakoServiceHandleType* handle);
static PyObject* py_hako_asset_service_client_get_progress(PyObject*, PyObject* args) {
    PyObject* capsule;
    if (!PyArg_ParseTuple(args, "O", &capsule)) {
        return NULL;
    }
    HakoServiceHandleType* handle = (HakoServiceHandleType*)PyCapsule_GetPointer(capsule, "HakoServiceHandleType");
    if (!handle) return NULL;
    
    int progress = hako_asset_service_client_get_progress(handle);
    return PyLong_FromLong(progress);    
}
//#HAKO_API int hako_asset_service_client_status(const HakoServiceHandleType* handle, int* status);
static PyObject* py_hako_asset_service_client_status(PyObject*, PyObject* args) {
    PyObject* capsule;
    if (!PyArg_ParseTuple(args, "O", &capsule)) {
        return NULL;
    }
    HakoServiceHandleType* handle = (HakoServiceHandleType*)PyCapsule_GetPointer(capsule, "HakoServiceHandleType");
    if (!handle) return NULL;
    
    int status = 0;
    int result = hako_asset_service_client_status(handle, &status);
    if (result == 0) {
        return PyLong_FromLong(status);
    }
    Py_RETURN_NONE;    
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
    {"register_data_recv_event", py_hako_asset_register_data_recv_event, METH_VARARGS, "Register data receive event callback."},
    {"check_data_recv_event", py_hako_asset_check_data_recv_event, METH_VARARGS, "Check if data was received for a given channel (flag-based)."},
    {"conductor_start", py_hako_conductor_start, METH_VARARGS, "Start the conductor with specified delta and max delay usec."},
    {"conductor_stop", py_hako_conductor_stop, METH_NOARGS, "Stop the conductor."},
    {"service_initialize", py_hako_asset_service_initialize, METH_VARARGS, "Initialize asset service."},
    //#HAKO_API int hako_asset_service_server_create(const char* assetName, const char* serviceName);
    {"asset_service_create", py_hako_asset_service_server_create, METH_VARARGS, "Initialize asset service."},
    //#HAKO_API int hako_asset_service_server_poll(int service_id);
    {"hako_asset_service_server_poll", py_hako_asset_service_server_poll, METH_VARARGS, "Poll asset service."},
    //#HAKO_API int hako_asset_service_server_get_current_client_id(int service_id);
    {"hako_asset_service_server_get_current_client_id", py_hako_asset_service_server_get_current_client_id, METH_VARARGS, "Get current client ID."},
    //#HAKO_API int hako_asset_service_server_get_current_channel_id(int service_id, int* request_channel_id, int* response_channel_id);
    {"hako_asset_service_server_get_current_channel_id", py_hako_asset_service_server_get_current_channel_id, METH_VARARGS, "Get current channel ID."},
    //#HAKO_API int hako_asset_service_server_status(int service_id, int* status);
    {"hako_asset_service_server_status", py_hako_asset_service_server_status, METH_VARARGS, "Get server status."},
    //#HAKO_API int hako_asset_service_server_get_request(int service_id, char** packet, size_t* packet_len);
    {"hako_asset_service_server_get_request", py_hako_asset_service_server_get_request, METH_VARARGS, "Get request data."},
    //#HAKO_API int hako_asset_service_server_get_response_buffer(int service_id, char** packet, size_t* packet_len, int status, int result_code);
    {"hako_asset_service_server_get_response_buffer", py_hako_asset_service_server_get_response_buffer, METH_VARARGS, "Get response buffer."},
    //#HAKO_API int hako_asset_service_server_put_response(int service_id, char* packet, size_t packet_len);
    {"hako_asset_service_server_put_response", py_hako_asset_service_server_put_response, METH_VARARGS, "Put response data."},
    //#HAKO_API int hako_asset_service_server_is_canceled(int service_id);
    {"hako_asset_service_server_is_canceled", py_hako_asset_service_server_is_canceled, METH_VARARGS, "Check if service is canceled."},
    //#HAKO_API int hako_asset_service_server_set_progress(int service_id, int percentage);
    {"hako_asset_service_server_set_progress", py_hako_asset_service_server_set_progress, METH_VARARGS, "Set progress."},
    //#HAKO_API int hako_asset_service_client_create(const char* assetName, const char* serviceName, const char* clientName, HakoServiceHandleType* handle);
    {"hako_asset_service_client_create", py_hako_asset_service_client_create, METH_VARARGS, "Create asset service client."},
    //#HAKO_API int hako_asset_service_client_poll(const HakoServiceHandleType* handle);
    {"hako_asset_service_client_poll", py_hako_asset_service_client_poll, METH_VARARGS, "Poll asset service client."},
    //#HAKO_API int hako_asset_service_client_get_current_channel_id(int service_id, int* request_channel_id, int* response_channel_id);
    {"hako_asset_service_client_get_current_channel_id", py_hako_asset_service_client_get_current_channel_id, METH_VARARGS, "Get current channel ID."},
    //#HAKO_API int hako_asset_service_client_get_request_buffer(const HakoServiceHandleType* handle, char** packet, size_t* packet_len, int opcode, int poll_interval_msec);
    {"hako_asset_service_client_get_request_buffer", py_hako_asset_service_client_get_request_buffer, METH_VARARGS, "Get request buffer."},
    //#HAKO_API int hako_asset_service_client_call_request(const HakoServiceHandleType* handle, char* packet, size_t packet_len, int timeout_msec);
    {"hako_asset_service_client_call_request", py_hako_asset_service_client_call_request, METH_VARARGS, "Call request."},
    //#HAKO_API int hako_asset_service_client_get_response(const HakoServiceHandleType* handle, char** packet, size_t* packet_len, int timeout);
    {"hako_asset_service_client_get_response", py_hako_asset_service_client_get_response, METH_VARARGS, "Get response."},
    //#HAKO_API int hako_asset_service_client_cancel_request(const HakoServiceHandleType* handle);
    {"hako_asset_service_client_cancel_request", py_hako_asset_service_client_cancel_request, METH_VARARGS, "Cancel request."},
    //#HAKO_API int hako_asset_service_client_get_progress(const HakoServiceHandleType* handle);
    {"hako_asset_service_client_get_progress", py_hako_asset_service_client_get_progress, METH_VARARGS, "Get progress."},
    //#HAKO_API int hako_asset_service_client_status(const HakoServiceHandleType* handle, int* status);
    {"hako_asset_service_client_status", py_hako_asset_service_client_status, METH_VARARGS, "Get client status."},
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
