/*
 * (C) 2017 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_BULK_MODE_HPP
#define __THALLIUM_BULK_MODE_HPP

#include <margo.h>

namespace thallium {

/**
 * @brief bulk_mode enum indicates whether a bulk object is
 * created for read/write, or read only, or write only.
 */
enum class bulk_mode : hg_uint32_t {
    read_write = HG_BULK_READWRITE,
    read_only  = HG_BULK_READ_ONLY,
    write_only = HG_BULK_WRITE_ONLY
};

} // namespace thallium

#endif
