/*
 * Copyright (c) 2017 UChicago Argonne, LLC
 *
 * See COPYRIGHT in top-level directory.
 */

#ifndef __THALLIUM_RWLOCK_HPP
#define __THALLIUM_RWLOCK_HPP

#include <abt.h>

namespace thallium {

/**
 * Exception class thrown by the rwlock class.
 */
class rwlock_exception : public exception {

    public:

    template<typename ... Args>
        rwlock_exception(Args&&... args)
        : exception(std::forward<Args>(args)...) {}
};

#define TL_RWLOCK_EXCEPTION(__fun,__ret) \
    rwlock_exception(#__fun," returned ", abt_error_get_name(__ret),\
            " (", abt_error_get_description(__ret),") in ",__FILE__,":",__LINE__);

#define TL_RWLOCK_ASSERT(__call) {\
    int __ret = __call; \
    if(__ret != ABT_SUCCESS) {\
        throw TL_RWLOCK_EXCEPTION(__call, __ret);\
    }\
}

/**
 * @brief The rwlock class wraps and managed an ABT_rwlock.
 */
class rwlock {
	
	ABT_rwlock m_lock;

	public:

    /**
     * @brief Native handle type.
     */
    typedef ABT_rwlock native_handle_type;

    /**
     * @brief Constructor.
     */
	explicit rwlock() {
        TL_RWLOCK_ASSERT(ABT_rwlock_create(&m_lock));
	}

    /**
     * @brief Copy constructor is deleted.
     */
	rwlock(const rwlock&) = delete;

    /**
     * @brief Move constructor.
     */
	rwlock(rwlock&& other) {
		m_lock = other.m_lock;
		other.m_lock = ABT_RWLOCK_NULL;
	}

    /**
     * @brief Copy assignment operator is deleted.
     */
    rwlock& operator=(const rwlock&) = delete;

    /**
     * @brief Move assignment operator.
     */
    rwlock& operator=(rwlock&& other) {
        if(this == &other) return *this;
        TL_RWLOCK_ASSERT(ABT_rwlock_free(&m_lock));
        m_lock = other.m_lock;
        other.m_lock = ABT_RWLOCK_NULL;
        return *this;
    }

    /**
     * @brief Destructor.
     */
	~rwlock() noexcept {
		ABT_rwlock_free(&m_lock);
	}

    /**
     * @brief Lock for reading.
     */
	void rdlock() {
        TL_RWLOCK_ASSERT(ABT_rwlock_rdlock(m_lock));
	}

    /**
     * @brief Lock for writing.
     */
	void wrlock() {
	    TL_RWLOCK_ASSERT(ABT_rwlock_wrlock(m_lock));
	}

    /**
     * @brief Unlock.
     */
	void unlock() {
		TL_RWLOCK_ASSERT(ABT_rwlock_unlock(m_lock));
	}

    /**
     * @brief Get the underlying native handle.
     *
     * @return the underlying native handle.
     */
    native_handle_type native_handle() const noexcept {
        return m_lock;
    }
};

} 

#undef TL_RWLOCK_EXCEPTION
#undef TL_RWLOCK_ASSERT

#endif /* end of include guard */
