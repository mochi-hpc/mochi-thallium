/*
 * (C) 2017 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_BULK_HPP
#define __THALLIUM_BULK_HPP

#include <thallium/margo_instance_ref.hpp>
#include <thallium/margo_exception.hpp>
#include <cstdint>
#include <memory>
#include <margo.h>
#include <string>
#include <vector>

namespace thallium {

class engine;
class endpoint;
class remote_bulk;
class bulk_segment;

/**
 * @brief bulk objects represent abstractions of memory
 * segments exposed by a process for RDMA operations. A bulk
 * object can be serialized to be sent over RPC to another process.
 */
class bulk {

    friend class engine;
    friend class remote_bulk;

  private:

    margo_instance_ref m_mid;
    hg_bulk_t          m_bulk     = HG_BULK_NULL;
    bool               m_is_local = false;

    /**
     * @brief Constructor. Made private as bulk objects
     * are instanciated by engine::expose (for example),
     * not directory by users.
     *
     * @param mid Margo instance creating the object.
     * @param b Mercury bulk handle.
     * @param local Whether the bulk handle referes to memory that is
     * local to this process.
     */
    bulk(margo_instance_ref mid, hg_bulk_t b, bool local) noexcept
    : m_mid{std::move(mid)}
    , m_bulk(b)
    , m_is_local(local) {}

  public:
    /**
     * @brief Default constructor, defined so that one can have a bulk
     * object as class member and associate it later with an actual bulk.
     */
    bulk() noexcept = default;

    /**
     * @brief Copy constructor.
     */
    bulk(const bulk& other)
    : m_mid{other.m_mid}
    , m_bulk(other.m_bulk)
    , m_is_local(other.m_is_local) {
        if(other.m_bulk != HG_BULK_NULL) {
            hg_return_t ret = margo_bulk_ref_incr(m_bulk);
            MARGO_ASSERT(ret, margo_bulk_ref_incr);
        }
    }

    /**
     * @brief Move constructor.
     */
    bulk(bulk&& other) noexcept
    : m_mid(std::move(other.m_mid))
    , m_bulk(other.m_bulk)
    , m_is_local(other.m_is_local) {
        other.m_bulk = HG_BULK_NULL;
    }

    /**
     * @brief Copy-assignment operator.
     */
    bulk& operator=(const bulk& other) {
        if(this == &other)
            return *this;
        if(m_bulk != HG_BULK_NULL) {
            hg_return_t ret = margo_bulk_free(m_bulk);
            MARGO_ASSERT(ret, margo_bulk_free);
        }
        m_bulk     = other.m_bulk;
        m_is_local = other.m_is_local;
        if(m_bulk != HG_BULK_NULL) {
            hg_return_t ret = margo_bulk_ref_incr(m_bulk);
            MARGO_ASSERT(ret, margo_bulk_ref_incr);
        }
        m_mid = other.m_mid;
        return *this;
    }

    /**
     * @brief Move-assignment operator.
     */
    bulk& operator=(bulk&& other) {
        if(this == &other)
            return *this;
        if(m_bulk != HG_BULK_NULL) {
            hg_return_t ret = margo_bulk_free(m_bulk);
            MARGO_ASSERT(ret, margo_bulk_free);
        }
        m_bulk        = other.m_bulk;
        m_is_local    = other.m_is_local;
        other.m_bulk  = HG_BULK_NULL;
        m_mid         = std::move(other.m_mid);
        return *this;
    }

    /**
     * @brief Destructor.
     */
    ~bulk() noexcept {
        if(m_bulk != HG_BULK_NULL) {
            hg_return_t ret = margo_bulk_free(m_bulk);
            MARGO_ASSERT_TERMINATE(ret, margo_bulk_free);
        }
    }

    /**
     * @brief Returns the size of the data exposed by the bulk object.
     *
     * @return size of data exposed by the bulk object.
     */
    std::size_t size() const noexcept {
        if(m_bulk != HG_BULK_NULL)
            return margo_bulk_get_size(m_bulk);
        else
            return 0;
    }

    /**
     * @brief Get total number of segments abstracted by bulk handle.
     */
    std::uint32_t segment_count() const noexcept {
        if(m_bulk != HG_BULK_NULL)
            return margo_bulk_get_segment_count(m_bulk);
        else
            return 0;
    }

    /**
     * @brief Indicates whether the bulk handle is null.
     *
     * @return true if the bulk handle is null, false otherwise.
     */
    bool is_null() const noexcept { return m_bulk == HG_BULK_NULL; }

    /**
     * @brief Builds a remote_bulk object by associating it with an endpoint.
     *
     * @param ep endpoint with which the bulk object should be associated.
     *
     * @return a remote_bulk instance.
     */
    remote_bulk on(const endpoint& ep) const noexcept;

    /**
     * @brief Creates a bulk_segment object by selecting a given portion
     * of the bulk object given an offset and a size.
     *
     * @param offset Offset at which the segment starts.
     * @param size Size of the segment.
     *
     * @return a bulk_segment object.
     */
    bulk_segment select(std::size_t offset, std::size_t size) const noexcept;

    /**
     * @see bulk::select
     */
    bulk_segment operator()(std::size_t offset, std::size_t size) const noexcept;

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
     * @brief Returns the underlying hg_bulk_t handle.
     * If copy == false, the returned handle is a reference to the internal
     * handle managed by this bulk object, it should not be destroyed by the
     * user and its lifetime will not exceed that of the bulk object.
     * If copy == true, the returned handle should be destroyed by the user.
     *
     * @param copy whether to make a copy or not.
     *
     * @return The underlying hg_bulk_t handle.
     */
    hg_bulk_t get_bulk(bool copy = false) const noexcept;

    /**
     * @brief Function that serializes a bulk object into/from an archive.
     *
     * @tparam A Archive type.
     * @param ar archive.
     */
    template <typename A> void serialize(A& ar) {
        using namespace std::string_literals;
        auto ret = hg_proc_hg_bulk_t(ar.get_proc(), &m_bulk);
        if(ret != HG_SUCCESS) {
            std::stringstream ss;
            throw exception{
                "Error during serialization, hg_proc_hg_bulk_t returned "s +
                HG_Error_to_string(ret)};
        }
        if(!m_mid) m_mid = ar.get_engine().get_margo_instance();
    }
};

/**
 * @brief The bulk_segment class represents a portion
 * (represented by offset and size) of a bulk object.
 */
class bulk_segment {
    friend class remote_bulk;

    std::size_t m_offset;
    std::size_t m_size;
    bulk        m_bulk;

    public:
    /**
     * @brief Constructor. By default the size of the segment will be
     * that of the underlying bulk object, and the offset is 0.
     *
     * @param b Reference to the bulk object from which the segment is
     * taken.
     */
    bulk_segment(const bulk& b) noexcept
    : m_offset(0)
    , m_size(b.size())
    , m_bulk(b) {}

    /**
     * @brief Constructor.
     *
     * @param b Reference to the bulk object from which the segment is
     * taken.
     * @param offset Offset at which the segment starts.
     * @param size Size of the segment.
     */
    bulk_segment(const bulk& b, std::size_t offset, std::size_t size) noexcept
    : m_offset(offset)
    , m_size(size)
    , m_bulk(b) {}

    /**
     * @brief Copy constructor is deleted.
     */
    bulk_segment(const bulk_segment&) = default;

    /**
     * @brief Move constructor is default.
     */
    bulk_segment(bulk_segment&&) = default;

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
    ~bulk_segment() = default;

    /**
     * @brief Associates the bulk segment with an endpoint to represent
     * a remote_bulk object.
     *
     * @param ep Endpoint where the bulk object has bee created.
     *
     * @return a remote_bulk object.
     */
    remote_bulk on(const endpoint& ep) const noexcept;

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
     * @brief Selects a subsegment from this segment. If the size is too
     * large, the maximum possible size is chosen.
     *
     * @param offset Offset of the subsegment relative to the current
     * segment.
     * @param size Size of the subsegment.
     *
     * @return a new bulk_segment object.
     */
    bulk_segment select(std::size_t offset, std::size_t size) const noexcept {
        std::size_t effective_size =
            offset + size > m_size ? m_size - offset : size;
        return bulk_segment(m_bulk, m_offset + offset, effective_size);
    }

    /**
     * @see bulk_segment::select.
     */
    inline bulk_segment operator()(std::size_t offset,
            std::size_t size) const noexcept {
        return select(offset, size);
    }
};

} // namespace thallium

#include <thallium/endpoint.hpp>
#include <thallium/remote_bulk.hpp>

namespace thallium {

inline hg_bulk_t bulk::get_bulk(bool copy) const noexcept {
    if(copy && m_bulk != HG_BULK_NULL)
        margo_bulk_ref_incr(m_bulk);
    return m_bulk;
}

inline bulk_segment bulk::select(std::size_t offset, std::size_t size) const noexcept {
    return bulk_segment(*this, offset, size);
}

inline bulk_segment bulk::operator()(std::size_t offset,
                                    std::size_t size) const noexcept {
    return select(offset, size);
}

inline remote_bulk bulk_segment::on(const endpoint& ep) const noexcept {
    return remote_bulk(*this, ep);
}

inline remote_bulk bulk::on(const endpoint& ep) const noexcept {
    return remote_bulk(*this, ep);
}

inline std::size_t bulk_segment::operator>>(const remote_bulk& b) const {
    return b << *this;
}

inline std::size_t bulk_segment::operator<<(const remote_bulk& b) const {
    return b >> *this;
}

inline std::size_t bulk::operator>>(const remote_bulk& b) const {
    return b << (this->select(0, size()));
}

inline std::size_t bulk::operator<<(const remote_bulk& b) const {
    return b >> (this->select(0, size()));
}

} // namespace thallium

#endif
