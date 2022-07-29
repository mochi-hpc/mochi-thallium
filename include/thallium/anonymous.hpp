/*
 * Copyright (c) 2017 UChicago Argonne, LLC
 *
 * See COPYRIGHT in top-level directory.
 */

#ifndef __THALLIUM_ANONYMOUS_HPP
#define __THALLIUM_ANONYMOUS_HPP

namespace thallium {

/**
 * The anonymous structure is used as a tag in pool::make_thread
 * and other such functions to create anonymous threads and tasks.
 * Anonymous threads and tasks will be automatically freed by
 * Argobots upon termination, hence their creation functions won't
 * return a managed object.
 */
struct anonymous {};

} // namespace thallium

#endif
