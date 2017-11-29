/*
 * (C) 2017 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_PROC_BUFFER_HPP
#define __THALLIUM_PROC_BUFFER_HPP

#include <vector>
#include <mercury_proc.h>
#include <thallium/buffer.hpp>

namespace thallium {

/**
 * @brief Mercury callback that serializes/deserializes
 * a buffer (std::vector<char>).
 *
 * @param proc Mercury proc object.
 * @param data pointer to a buffer object.
 *
 * @return HG_SUCCESS or a Mercury error code.
 */
hg_return_t process_buffer(hg_proc_t proc, void* data);

} // namespace thallium

#endif
