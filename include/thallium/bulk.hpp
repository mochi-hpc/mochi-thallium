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

namespace thallium {

class engine;
class resolved_bulk;

class bulk {

    friend class engine;
    friend class resolved_bulk;

private:

    engine*   m_engine;
	hg_bulk_t m_bulk;
    bool      m_is_local;

	bulk(engine& e, hg_bulk_t b, bool local)
	: m_engine(&e), m_bulk(b), m_is_local(local) {}

    class bulk_segment {

        friend class resolved_bulk;
        
        std::size_t m_offset;
        std::size_t m_size;
        const bulk& m_bulk;

        public:

        bulk_segment(const bulk& b)
        : m_offset(0), m_size(b.size()), m_bulk(b) {}

        bulk_segment(const bulk& b, std::size_t offset, std::size_t size)
        : m_offset(offset), m_size(size), m_bulk(b) {}

        bulk_segment(const bulk_segment&) = delete;
        bulk_segment(bulk_segment&&)      = default;

        ~bulk_segment()                   = default;

        resolved_bulk on(const endpoint& ep) const;

        std::size_t operator>>(const resolved_bulk& b) const;

        std::size_t operator<<(const resolved_bulk& b) const;
    };

public:

    bulk()
    : m_engine(nullptr), m_bulk(HG_BULK_NULL), m_is_local(false) {}

	bulk(const bulk& other)
    : m_engine(other.m_engine), m_bulk(other.m_bulk), m_is_local(other.m_is_local) {
        margo_bulk_ref_incr(m_bulk);
    }

	bulk(bulk&& other)
	: m_engine(other.m_engine), m_bulk(other.m_bulk), m_is_local(std::move(other.m_is_local)) {
		other.m_bulk     = HG_BULK_NULL;
	}

	bulk& operator=(const bulk& other) {
        if(this == &other) return *this;
        if(m_bulk != HG_BULK_NULL) {
            margo_bulk_free(m_bulk);
        }
        m_bulk     = other.m_bulk;
        m_engine   = other.m_engine;
        m_is_local = other.m_is_local;
        if(m_bulk != HG_BULK_NULL) {
            margo_bulk_ref_incr(m_bulk);
        }
        return *this;
    }

	bulk& operator=(bulk&& other) {
        if(this == &other) return *this;
        if(m_bulk != HG_BULK_NULL) {
            margo_bulk_free(m_bulk);
        }
        m_engine     = other.m_engine;
        m_bulk       = other.m_bulk;
        m_is_local   = other.m_is_local;
        other.m_bulk = HG_BULK_NULL;
        return *this;
    }
	
	~bulk() {
        if(m_bulk != HG_BULK_NULL) {
            margo_bulk_free(m_bulk);
        }
    }

    std::size_t size() const {
        if(m_bulk != HG_BULK_NULL)
            return margo_bulk_get_size(m_bulk);
        else
            return 0;
    }

    bool is_null() const {
        return m_bulk == HG_BULK_NULL;
    }

    resolved_bulk on(const endpoint& ep) const;

    bulk_segment select(std::size_t offset, std::size_t size) const;

    bulk_segment operator()(std::size_t offset, std::size_t size) const;

    std::size_t operator>>(const resolved_bulk& b) const;

    std::size_t operator<<(const resolved_bulk& b) const;

    template<typename A>
    void save(A& ar) {
        hg_size_t s = margo_bulk_get_serialize_size(m_bulk, HG_TRUE);
        std::vector<char> buf(s);
        margo_bulk_serialize(&buf[0], s, HG_TRUE, m_bulk);
        // XXX check return values
        ar & buf;
    }

    template<typename A>
    void load(A& ar);

};

}

#include <thallium/engine.hpp>

namespace thallium {

template<typename A>
void bulk::load(A& ar) {
    std::vector<char> buf;
    ar & buf;
    m_engine = &(ar.get_engine());
    margo_bulk_deserialize(m_engine->m_mid, &m_bulk, &buf[0], buf.size());
    // XXX check return value
    m_is_local = false;
}

}

#endif
