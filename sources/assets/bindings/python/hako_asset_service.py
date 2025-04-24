import ctypes



class HakoServiceHandle(ctypes.Structure):
    _fields_ = [
        ('service_id', ctypes.c_int),
        ('client_id', ctypes.c_int),
    ]
class HakoAssetService:
    def __init__(self, lib_path: str):
        self.lib = ctypes.CDLL(lib_path, mode=ctypes.RTLD_GLOBAL)

        # Server APIs
        #HAKO_API int hako_asset_service_initialize(const char* service_config_path);
        self.lib.hako_asset_service_initialize.argtypes = [ctypes.c_char_p]
        self.lib.hako_asset_service_initialize.restype = ctypes.c_int
        #HAKO_API int hako_asset_service_server_create(const char* assetName, const char* serviceName);
        self.lib.hako_asset_service_server_create.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
        self.lib.hako_asset_service_server_create.restype = ctypes.c_int
        #HAKO_API int hako_asset_service_server_poll(int service_id);
        self.lib.hako_asset_service_server_poll.argtypes = [ctypes.c_int]
        self.lib.hako_asset_service_server_poll.restype = ctypes.c_int
        #HAKO_API int hako_asset_service_server_get_current_client_id(int service_id);
        self.lib.hako_asset_service_server_get_current_client_id.argtypes = [ctypes.c_int]
        self.lib.hako_asset_service_server_get_current_client_id.restype = ctypes.c_int
        #HAKO_API int hako_asset_service_server_get_current_channel_id(int service_id, int* request_channel_id, int* response_channel_id);
        self.lib.hako_asset_service_server_get_current_channel_id.argtypes = [ctypes.c_int, ctypes.POINTER(ctypes.c_int), ctypes.POINTER(ctypes.c_int)]
        self.lib.hako_asset_service_server_get_current_channel_id.restype = ctypes.c_int
        #HAKO_API int hako_asset_service_server_status(int service_id, int* status);
        self.lib.hako_asset_service_server_status.argtypes = [ctypes.c_int, ctypes.POINTER(ctypes.c_int)]
        self.lib.hako_asset_service_server_status.restype = ctypes.c_int
        #HAKO_API int hako_asset_service_server_get_request(int service_id, char** packet, size_t* packet_len);
        self.lib.hako_asset_service_server_get_request.argtypes = [ctypes.c_int, ctypes.POINTER(ctypes.c_char_p), ctypes.POINTER(ctypes.c_size_t)]
        self.lib.hako_asset_service_server_get_request.restype = ctypes.c_int
        #HAKO_API int hako_asset_service_server_get_response_buffer(int service_id, char** packet, size_t* packet_len, int status, int result_code);
        self.lib.hako_asset_service_server_get_response_buffer.argtypes = [ctypes.c_int, ctypes.POINTER(ctypes.c_char_p), ctypes.POINTER(ctypes.c_size_t), ctypes.c_int, ctypes.c_int]
        self.lib.hako_asset_service_server_get_response_buffer.restype = ctypes.c_int
        #HAKO_API int hako_asset_service_server_put_response(int service_id, char* packet, size_t packet_len);
        self.lib.hako_asset_service_server_put_response.argtypes = [ctypes.c_int, ctypes.c_char_p, ctypes.c_size_t]
        self.lib.hako_asset_service_server_put_response.restype = ctypes.c_int
        #HAKO_API int hako_asset_service_server_is_canceled(int service_id);
        self.lib.hako_asset_service_server_is_canceled.argtypes = [ctypes.c_int]
        self.lib.hako_asset_service_server_is_canceled.restype = ctypes.c_int
        #HAKO_API int hako_asset_service_server_set_progress(int service_id, int percentage);
        self.lib.hako_asset_service_server_set_progress.argtypes = [ctypes.c_int, ctypes.c_int]
        self.lib.hako_asset_service_server_set_progress.restype = ctypes.c_int

        # Client APIs
        #HAKO_API int hako_asset_service_client_create(const char* assetName, const char* serviceName, const char* clientName, HakoServiceHandleType* handle);
        self.lib.hako_asset_service_client_create.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.POINTER(HakoServiceHandle)]
        self.lib.hako_asset_service_client_create.restype = ctypes.c_int
        #HAKO_API int hako_asset_service_client_poll(const HakoServiceHandleType* handle);
        self.lib.hako_asset_service_client_poll.argtypes = [ctypes.POINTER(HakoServiceHandle)]
        self.lib.hako_asset_service_client_poll.restype = ctypes.c_int
        #HAKO_API int hako_asset_service_client_get_current_channel_id(int service_id, int* request_channel_id, int* response_channel_id);
        self.lib.hako_asset_service_client_get_current_channel_id.argtypes = [ctypes.POINTER(HakoServiceHandle), ctypes.POINTER(ctypes.c_int), ctypes.POINTER(ctypes.c_int)]
        self.lib.hako_asset_service_client_get_current_channel_id.restype = ctypes.c_int
        #HAKO_API int hako_asset_service_client_get_request_buffer(const HakoServiceHandleType* handle, char** packet, size_t* packet_len, int opcode, int poll_interval_msec);
        self.lib.hako_asset_service_client_get_request_buffer.argtypes = [ctypes.POINTER(HakoServiceHandle), ctypes.POINTER(ctypes.c_char_p), ctypes.POINTER(ctypes.c_size_t), ctypes.c_int, ctypes.c_int]
        self.lib.hako_asset_service_client_get_request_buffer.restype = ctypes.c_int
        #HAKO_API int hako_asset_service_client_call_request(const HakoServiceHandleType* handle, char* packet, size_t packet_len, int timeout_msec);
        self.lib.hako_asset_service_client_call_request.argtypes = [ctypes.POINTER(HakoServiceHandle), ctypes.c_char_p, ctypes.c_size_t, ctypes.c_int]
        self.lib.hako_asset_service_client_call_request.restype = ctypes.c_int
        #HAKO_API int hako_asset_service_client_get_response(const HakoServiceHandleType* handle, char** packet, size_t* packet_len, int timeout);
        self.lib.hako_asset_service_client_get_response.argtypes = [ctypes.POINTER(HakoServiceHandle), ctypes.POINTER(ctypes.c_char_p), ctypes.POINTER(ctypes.c_size_t), ctypes.c_int]
        self.lib.hako_asset_service_client_get_response.restype = ctypes.c_int
        #HAKO_API int hako_asset_service_client_cancel_request(const HakoServiceHandleType* handle);
        self.lib.hako_asset_service_client_cancel_request.argtypes = [ctypes.POINTER(HakoServiceHandle)]
        self.lib.hako_asset_service_client_cancel_request.restype = ctypes.c_int
        #HAKO_API int hako_asset_service_client_get_progress(const HakoServiceHandleType* handle);
        self.lib.hako_asset_service_client_get_progress.argtypes = [ctypes.POINTER(HakoServiceHandle), ctypes.POINTER(ctypes.c_int)]
        self.lib.hako_asset_service_client_get_progress.restype = ctypes.c_int
        #HAKO_API int hako_asset_service_client_status(const HakoServiceHandleType* handle, int* status);
        self.lib.hako_asset_service_client_status.argtypes = [ctypes.POINTER(HakoServiceHandle), ctypes.POINTER(ctypes.c_int)]
        self.lib.hako_asset_service_client_status.restype = ctypes.c_int

    def initialize(self, service_config_path: str) -> int:
        """
        Initialize the Hako Asset Service with the given configuration path.
        :param service_config_path: Path to the service configuration file.
        :return: Status code of the initialization.
        """
        return self.lib.hako_asset_service_initialize(service_config_path.encode('utf-8'))