/*
 * Copyright (c) 2017 UChicago Argonne, LLC
 *
 * See COPYRIGHT in top-level directory.
 */

#ifndef __THALLIUM_MUTEX_HPP
#define __THALLIUM_MUTEX_HPP

#include <abt.h>
#include <thallium/exception.hpp>
#include <thallium/abt_errors.hpp>

namespace thallium {

/**
 * Exception class thrown by the mutex class.
 */
class mutex_exception : public exception {

    public:

        template<typename ... Args>
        mutex_exception(Args&&... args)
        : exception(std::forward<Args>(args)...) {}
};

#define TL_MUTEX_EXCEPTION(__fun,__ret) \
    mutex_exception(#__fun," returned ", abt_error_get_name(__ret),\
            " (", abt_error_get_description(__ret),") in ",__FILE__,":",__LINE__);

#define TL_MUTEX_ASSERT(__call) {\
    int __ret = __call; \
    if(__ret != ABT_SUCCESS) {\
        throw TL_MUTEX_EXCEPTION(__call, __ret);\
    }\
}

/**
 * @brief The mutex class is an equivalent of std::mutex
 * but build around Argobot's mutex.
 */
class mutex {
	
	ABT_mutex m_mutex;

	public:

    /**
     * @brief Type of the underlying native handle.
     */
    typedef ABT_mutex native_handle_type;

    /**
     * @brief Constructor.
     *
     * @param recursive whether the mutex is recursive or not.
     */
	explicit mutex(bool recursive = false) {
        ABT_mutex_attr attr;
		TL_MUTEX_ASSERT(ABT_mutex_attr_create(&attr));
		if(recursive) {
            TL_MUTEX_ASSERT(ABT_mutex_attr_set_recursive(attr, ABT_TRUE));
        }
		TL_MUTEX_ASSERT(ABT_mutex_create_with_attr(attr, &m_mutex));
        TL_MUTEX_ASSERT(ABT_mutex_attr_free(&attr));
	}

    /**
     * @brief Copy constructor is deleted.
     */
	mutex(const mutex& other) = delete;

    /**
     * @brief Copy assignment operator is deleted.
     */
    mutex& operator=(const mutex& other) = delete;

    /**
     * @brief Move assignment operator is deleted.
     */
    mutex& operator=(mutex&& other) {
        if(m_mutex != ABT_MUTEX_NULL) 
            ABT_mutex_free(&m_mutex);
        m_mutex = other.m_mutex;
        other.m_mutex = ABT_MUTEX_NULL;
        return *this;
    }

    /**
     * @brief Move constructor.
     */
	mutex(mutex&& other) {
		m_mutex = other.m_mutex;
		other.m_mutex = ABT_MUTEX_NULL;
	}

    /**
     * @brief Destructor.
     */
	virtual ~mutex() noexcept {
		if(m_mutex != ABT_MUTEX_NULL)
			ABT_mutex_free(&m_mutex);
	}

    /**
     * @brief Lock the mutex.
     */
	void lock() {
        TL_MUTEX_ASSERT(ABT_mutex_lock(m_mutex));
	}

    /**
     * @brief Lock the mutex in low priority.
     */
	void lock_low() {
		TL_MUTEX_ASSERT(ABT_mutex_lock_low(m_mutex));
	}

    /**
     * @brief Lock the mutex without context switch.
     */
	void spin_lock() {
		TL_MUTEX_ASSERT(ABT_mutex_spinlock(m_mutex));
	}

    /**
     * @brief Try locking the mutex without blocking.
     *
     * @return true if it managed to lock the mutex.
     */
	bool try_lock() {
        int ret = ABT_mutex_trylock(m_mutex);
        if(ABT_SUCCESS == ret) {
            return true;
        } else if(ABT_ERR_MUTEX_LOCKED == ret) {
            return false;
        } else {
           TL_MUTEX_EXCEPTION(ABT_mutex_trylock(m_mutex), ret); 
        }
        return false;
	}

    /**
     * @brief Unlock the mutex.
     */
	void unlock() {
		TL_MUTEX_ASSERT(ABT_mutex_unlock(m_mutex));
	}

    /**
     * @brief Hand over the mutex within the ES.
     */
	void unlock_se() {
		TL_MUTEX_ASSERT(ABT_mutex_unlock_se(m_mutex));
	}
    
    /**
     * @brief Get the underlying native handle.
     *
     * @return the underlying native handle.
     */
    ABT_mutex native_handle() const noexcept {
        return m_mutex;
    }
};

/**
 * @brief Child class of mutex to represent a recursive mutex.
 */
class recursive_mutex : public mutex {

    public:

    /**
     * @brief Constructor.
     */
    recursive_mutex()
    : mutex(true) {}

    /**
     * @brief Copy constructor is deleted.
     */
    recursive_mutex(const recursive_mutex& other) = delete;

    /**
     * @brief Move constructor.
     */
    recursive_mutex(recursive_mutex&& other)
        : mutex(std::move(other)) {}

    /**
     * @brief Copy assignment operator is deleted.
     */
    recursive_mutex& operator=(const recursive_mutex& other) = delete;

    /**
     * @brief Move assignment operator.
     */
    recursive_mutex& operator=(recursive_mutex&& other) = default;
};

}

#undef TL_MUTEX_EXCEPTION
#undef TL_MUTEX_ASSERT

#endif /* end of include guard */
