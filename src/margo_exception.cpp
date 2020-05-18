/*
 * (C) 2017 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include <margo.h>
#include <thallium/margo_exception.hpp>

namespace thallium {

std::string translate_margo_error_code(hg_return_t ret) {
    switch(ret) {
    case HG_SUCCESS: /*!< operation succeeded */
        return "HG_SUCCESS";
    case HG_NA_ERROR: /*!< error in NA layer */
        return "HG_NA_ERROR";
    case HG_TIMEOUT: /*!< reached timeout */
        return "HG_TIMEOUT";
    case HG_INVALID_PARAM: /*!< invalid parameter */
        return "HG_INVALID_PARAM";
    case HG_SIZE_ERROR: /*!< size error */
        return "HG_SIZE_ERROR";
    case HG_NOMEM_ERROR: /*!< no memory error */
        return "HG_NOMEM_ERROR";
    case HG_PROTOCOL_ERROR: /*!< protocol does not match */
        return "HG_PROTOCOL_ERROR";
    case HG_NO_MATCH: /*!< no function match */
        return "HG_NO_MATCH";
    case HG_CHECKSUM_ERROR: /*!< checksum error */
        return "HG_CHECKSUM_ERROR";
    case HG_CANCELED: /*!< operation was canceled */
        return "HG_CANCELED";
    case HG_OTHER_ERROR: /*!< error from mercury_util or external to mercury */
        return "HG_OTHER_ERROR";
    }
    return "Unknown error";
}

} // namespace thallium
