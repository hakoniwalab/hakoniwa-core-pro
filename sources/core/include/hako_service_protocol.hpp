#pragma once

/*
 * NOTE:
 * These enums are defined as C-compatible (not enum class)
 * to ensure interoperability with C-based service implementations.
 *
 * This file defines the core protocol constants for Hakoniwa RPC services.
 * Each value corresponds directly to fields in the PDU structure exchanged
 * between service clients and servers.
 */

namespace hako::service {

    /*
     * Operation code to be set by the client when sending a service request.
     * This indicates the type of request the client wants to perform.
     * 
     * Field: HakoCpp_ServiceRequestHeader::opcode
     */
    enum HakoServiceOperationCodeType {
        HAKO_SERVICE_OPERATION_CODE_REQUEST = 0,  // Standard service request
        HAKO_SERVICE_OPERATION_CODE_CANCEL,       // Cancel the currently active request
        HAKO_SERVICE_OPERATION_NUM
    };

    /*
     * Service status to be set by the server when replying to a request.
     * Indicates the internal progress/state of the requested operation.
     * 
     * Field: HakoCpp_ServiceResponseHeader::status
     */
    enum HakoServiceStatusType {
        HAKO_SERVICE_STATUS_NONE = 0,      // No active service
        HAKO_SERVICE_STATUS_DOING,         // Service is currently being processed
        HAKO_SERVICE_STATUS_CANCELING,     // Cancel is in progress
        HAKO_SERVICE_STATUS_DONE,          // Service has completed
        HAKO_SERVICE_STATUS_ERROR,         // An error occurred during processing
        HAKO_SERVICE_STATUS_NUM
    };

    /*
     * Result code to be set by the server when replying to a request.
     * This represents the outcome of the request operation.
     * 
     * Field: HakoCpp_ServiceResponseHeader::result_code
     */
    enum HakoServiceResultCodeType {
        HAKO_SERVICE_RESULT_CODE_OK = 0,        // Request completed successfully
        HAKO_SERVICE_RESULT_CODE_ERROR,         // Execution failed due to an error
        HAKO_SERVICE_RESULT_CODE_CANCELED,      // Request was canceled by client
        HAKO_SERVICE_RESULT_CODE_INVALID,       // Request was malformed or in invalid state
        HAKO_SERVICE_RESULT_CODE_BUSY,          // Server is busy processing another request
        HAKO_SERVICE_RESULT_CODE_NUM
    };

}
