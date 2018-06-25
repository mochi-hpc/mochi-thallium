/*
 * Copyright (c) 2017 UChicago Argonne, LLC
 *
 * See COPYRIGHT in top-level directory.
 */

#ifndef __THALLIUM_ABT_ERRORS_HPP
#define __THALLIUM_ABT_ERRORS_HPP

namespace thallium {

    /**
     * @brief For internal use. Converts an error code
     * returned by an Argobots function into a string name.
     *
     * @param err Error code
     *
     * @return Name of the error.
     */
    const char* abt_error_get_name(int err);

    /**
     * @brief For internal use. Converts an error code
     * returned by an Argobots function into a string description.
     *
     * @param err Error code
     *
     * @return Description of the error.
     */
    const char* abt_error_get_description(int err);

}

#endif
