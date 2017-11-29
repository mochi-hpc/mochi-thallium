/*
 * (C) 2017 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_REMOTE_BULK_HPP
#define __THALLIUM_REMOTE_BULK_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <margo.h>
#include <thallium/bulk.hpp>

namespace thallium {

/**
 * @brief A remote_bulk object represents a bulk_segment object
 * that has been associated with an endpoint and is ready for
 * RDMA operations.
 */
class remote_bulk {

    friend class bulk;

private:

    bulk::bulk_segment  m_segment;
    endpoint            m_endpoint;

    /**
     * @brief Constructor. Made private since remote_bulk objects
     * are created by the function bulk::on() or bulk_segment::on()
     * functions. 
     *
     * @param b bulk_segment that created the remote_bulk object.
     * @param ep endpoint on which the bulk_segment is.
     */
	remote_bulk(bulk::bulk_segment b, endpoint ep)
	: m_segment(std::move(b)), m_endpoint(std::move(ep)) {}

public:

    /**
     * @brief Performs a pull operation from the remote_bulk
     * (left operand) to the destination bulk (right operand).
     * The destination must be local. If the sizes don't match,
     * the smallest size is picked.
     *
     * @param dest Local bulk segment on which to pull the data.
     *
     * @return the size of data transfered.
     */
    std::size_t operator>>(const bulk::bulk_segment& dest) const;

    /**
     * @brief Performs a push operation from the source bulk
     * (right operand) to the remote_bulk (left operand).
     * The source must be local. If the sizes don't match,
     * the smallest size is picked.
     *
     * @param src Local bulk segment from which to push the data.
     *
     * @return the size of data transfered.
     */
    std::size_t operator<<(const bulk::bulk_segment& src) const;
};

}

#endif
