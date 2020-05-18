/*
 * Copyright (c) 2017 UChicago Argonne, LLC
 *
 * See COPYRIGHT in top-level directory.
 */

#ifndef __THALLIUM_UNIT_TYPE_HPP
#define __THALLIUM_UNIT_TYPE_HPP

#include <abt.h>

namespace thallium {

/**
 * @brief Type of work units. Used when defining
 * custom pools.
 */
enum class unit_type : std::uint8_t {
    thread  = ABT_UNIT_TYPE_THREAD,
    task    = ABT_UNIT_TYPE_TASK,
    xstream = ABT_UNIT_TYPE_XSTREAM,
    other   = ABT_UNIT_TYPE_EXT
};

} // namespace thallium

#endif
