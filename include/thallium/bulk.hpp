/*
 * (C) 2017 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_BULK_HPP
#define __THALLIUM_BULK_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <margo.h>
#include <thallium/endpoint.hpp>
#include <thallium/margo_exception.hpp>

namespace thallium {

class engine;
class remote_bulk;

/**
 * @brief bulk objects represent abstractions of memory
 * segments exposed by a process for RDMA operations. A bulk
 * object can be serialized to be sent over RPC to another process.
 */
class bulk {

    friend class engine;
    friend class remote_bulk;

private:

    engine*   m_engine;
    hg_bulk_t m_bulk;
    bool      m_is_local;
    bool      m_eager_mode;

    /**
     * @brief Constructor. Made private as bulk objects
     * are instanciated by engine::expose (for example),
     * not directory by users.
     *
     * @param e Reference to the engine instance creating the object.
     * @param b Mercury bulk handle.
     * @param local Whether the bulk handle referes to memory that is
     * local to this process.
     */
    bulk(engine& e, hg_bulk_t b, bool local)
    : m_engine(&e), m_bulk(b), m_is_local(local), m_eager_mode(false) {}

    /**
     * @brief The bulk_segment class represents a portion
     * (represented by offset and size) of a bulk object.
     */
    class bulk_segment {

        friend class remote_bulk;
        
        std::size_t m_offset;
        std::size_t m_size;
        const bulk& m_bulk;

        public:

        /**
         * @brief Constructor. By default the size of the segment will be
         * that of the underlying bulk object, and the offset is 0.
         *
         * @param b Reference to the bulk object from which the segment is taken.
         */
        bulk_segment(const bulk& b)
        : m_offset(0), m_size(b.size()), m_bulk(b) {}

        /**
         * @brief Constructor.
         *
         * @param b Reference to the bulk object from which the segment is taken.
         * @param offset Offset at which the segment starts.
         * @param size Size of the segment.
         */
        bulk_segment(const bulk& b, std::size_t offset, std::size_t size)
        : m_offset(offset), m_size(size), m_bulk(b) {}

        /**
         * @brief Copy constructor is deleted.
         */
        bulk_segment(const bulk_segment&) = default;

        /**
         * @brief Move constructor is default.
         */
        bulk_segment(bulk_segment&&)      = default;

        /**
         * @brief Copy assignment operator is default.
         */
        bulk_segment& operator=(const bulk_segment&) = default;

        /**
         * @brief Move assignment operator is default.
         */
        bulk_segment& operator=(bulk_segment&&) = default;

        /**
         * @brief Destructor is default.
         */
        ~bulk_segment()                   = default;

        /**
         * @brief Associates the bulk segment with an endpoint to represent
         * a remote_bulk object.
         *
         * @param ep Endpoint where the bulk object has bee created.
         *
         * @return a remote_bulk object.
         */
        remote_bulk on(const endpoint& ep) const;

        /**
         * @brief Pushes data from the left operand (bulk_segment)
         * to the right operand (remote_bulk). If the size of the
         * segments don't match, the smallest size is used.
         *
         * @param b remote_bulk object towards which to push data.
         *
         * @return the size of data transfered.
         */
        std::size_t operator>>(const remote_bulk& b) const;

        /**
         * @brief Pulls data from the right operand (remote_bulk)
         * to the right operand (bulk_segment). If the size of the
         * segments don't match, the smallest size is used.
         *
         * @param b remote_bulk object from which to pull data.
         *
         * @return the size of data transfered.
         */
        std::size_t operator<<(const remote_bulk& b) const;

        /**
         * @brief Selects a subsegment from this segment. If the size is too large,
         * the maximum possible size is chosen.
         *
         * @param offset Offset of the subsegment relative to the current segment.
         * @param size Size of the subsegment.
         *
         * @return a new bulk_segment object.
         */
        bulk_segment select(std::size_t offset, std::size_t size) const {
            std::size_t effective_size = offset+size > m_size ? m_size - offset : size;
            return bulk_segment(m_bulk, m_offset+offset, effective_size);
        }

        /**
         * @see bulk_segment::select.
         */
        inline bulk_segment operator()(std::size_t offset, std::size_t size) const {
            return select(offset, size);
        }
    };

public:

    /**
     * @brief Default constructor, defined so that one can have a bulk
     * object as class member and associate it later with an actual bulk.
     */
    bulk()
    : m_engine(nullptr), m_bulk(HG_BULK_NULL), m_is_local(false), m_eager_mode(false) {}

    /**
     * @brief Copy constructor.
     */
    bulk(const bulk& other)
    : m_engine(other.m_engine), m_bulk(other.m_bulk), 
      m_is_local(other.m_is_local), m_eager_mode(other.m_eager_mode) {
        if(other.m_bulk != HG_BULK_NULL) {
            hg_return_t ret = margo_bulk_ref_incr(m_bulk);
            MARGO_ASSERT(ret, margo_bulk_ref_incr);
        }
    }

    /**
     * @brief Move constructor.
     */
    bulk(bulk&& other)
    : m_engine(other.m_engine), m_bulk(other.m_bulk),
      m_is_local(other.m_is_local), 
      m_eager_mode(other.m_eager_mode) {
          other.m_bulk     = HG_BULK_NULL;
    }

    /**
     * @brief Copy-assignment operator.
     */
    bulk& operator=(const bulk& other) {
        if(this == &other) return *this;
        if(m_bulk != HG_BULK_NULL) {
            hg_return_t ret = margo_bulk_free(m_bulk);
            MARGO_ASSERT(ret, margo_bulk_free);
        }
        m_bulk     = other.m_bulk;
        m_engine   = other.m_engine;
        m_is_local = other.m_is_local;
        m_eager_mode = other.m_eager_mode;
        if(m_bulk != HG_BULK_NULL) {
            hg_return_t ret = margo_bulk_ref_incr(m_bulk);
            MARGO_ASSERT(ret, margo_bulk_ref_incr);
        }
        return *this;
    }

    /**
     * @brief Move-assignment operator.
     */
    bulk& operator=(bulk&& other) {
        if(this == &other) return *this;
        if(m_bulk != HG_BULK_NULL) {
            hg_return_t ret = margo_bulk_free(m_bulk);
            MARGO_ASSERT(ret, margo_bulk_free);
        }
        m_engine     = other.m_engine;
        m_bulk       = other.m_bulk;
        m_is_local   = other.m_is_local;
        m_eager_mode = other.m_eager_mode;
        other.m_bulk = HG_BULK_NULL;
        return *this;
    }

    /**
     * @brief Destructor.
     */
    ~bulk() {
        if(m_bulk != HG_BULK_NULL) {
            hg_return_t ret = margo_bulk_free(m_bulk);
            MARGO_ASSERT_TERMINATE(ret, margo_bulk_free, -1);
        }
    }

    /**
     * @brief Returns the size of the data exposed by the bulk object.
     *
     * @return size of data exposed by the bulk object.
     */
    std::size_t size() const {
        if(m_bulk != HG_BULK_NULL)
            return margo_bulk_get_size(m_bulk);
        else
            return 0;
    }

    /**
     * @brief Indicates whether the bulk handle is null.
     *
     * @return true if the bulk handle is null, false otherwise.
     */
    bool is_null() const {
        return m_bulk == HG_BULK_NULL;
    }

    /**
     * @brief Set the eager mode. When eager mode is true,
     * Mercury will try to send the bulk's exposed data along
     * with the bulk handle when sending the bulk handle over RPC.
     *
     * @param eager Whether to use eager mode or not.
     */
    void set_eager_mode(bool eager) {
        m_eager_mode = eager;
    }

    /**
     * @brief Builds a remote_bulk object by associating it with an endpoint.
     *
     * @param ep endpoint with which the bulk object should be associated.
     *
     * @return a remote_bulk instance.
     */
    remote_bulk on(const endpoint& ep) const;

    /**
     * @brief Creates a bulk_segment object by selecting a given portion
     * of the bulk object given an offset and a size.
     *
     * @param offset Offset at which the segment starts.
     * @param size Size of the segment.
     *
     * @return a bulk_segment object.
     */
    bulk_segment select(std::size_t offset, std::size_t size) const;


    /**
     * @see bulk::select
     */
    bulk_segment operator()(std::size_t offset, std::size_t size) const;

    /**
     * @brief Pushes data from the left operand (entire bulk object)
     * to the right operand (remote_bulk). If the size of the
     * segments don't match, the smallest size is used.
     *
     * @param b remote_bulk object towards which to push data.
     *
     * @return the size of data transfered.
     */
    std::size_t operator>>(const remote_bulk& b) const;

    /**
     * @brief Pulls data from the right operand (remote_bulk)
     * to the left operand (bulk). If the size of the
     * segments don't match, the smallest size is used.
     *
     * @param b remote_bulk object from which to pull data.
     *
     * @return the size of data transfered.
     */
    std::size_t operator<<(const remote_bulk& b) const;

    /**
     * @brief Function that serializes a bulk object into an archive.
     *
     * @tparam A Archive type.
     * @param ar Input archive.
     */
    template<typename A>
    void save(A& ar) {
        if(m_bulk == HG_BULK_NULL) {
            std::vector<char> buf;
            ar & buf;
        } else {
            auto use_eager = m_eager_mode ? HG_TRUE : HG_FALSE;
            hg_size_t s = margo_bulk_get_serialize_size(m_bulk, use_eager);
            std::vector<char> buf(s);
            hg_return_t ret = margo_bulk_serialize(&buf[0], s, use_eager, m_bulk);
            MARGO_ASSERT(ret, margo_bulk_serialize);
            ar & buf;
        }
    }

    /**
     * @brief Deserializes a bulk object from an output archive.
     *
     * @tparam A Archive type.
     * @param ar Output archive.
     */
    template<typename A>
    void load(A& ar);

};

}

#include <thallium/engine.hpp>

namespace thallium {

template<typename A>
void bulk::load(A& ar) {
    if(!is_null()) {
        *this = bulk(); // reset
    }
    std::vector<char> buf;
    ar & buf;
    if(buf.size() > 0) {
        m_engine = &(ar.get_engine());
        hg_return_t ret = margo_bulk_deserialize(m_engine->m_mid, &m_bulk, &buf[0], buf.size());
        MARGO_ASSERT(ret, margo_bulk_deserialize);
        m_is_local = false;
    }
}

}

#endif
