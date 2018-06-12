/*
 * Copyright (c) 2017 UChicago Argonne, LLC
 *
 * See COPYRIGHT in top-level directory.
 */

#ifndef __THALLIUM_MUTEX_HPP
#define __THALLIUM_MUTEX_HPP

#include <abt.h>

namespace thallium {

class condition_variable;

/**
 * @brief The mutex class is an equivalent of std::mutex
 * but build around Argobot's mutex.
 */
class mutex {
	
	ABT_mutex m_mutex;

    friend class condition_variable;

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
		ABT_mutex_attr_create(&attr);
		if(recursive)
			ABT_mutex_attr_set_recursive(attr, ABT_TRUE);
		ABT_mutex_create_with_attr(attr, &m_mutex);
		ABT_mutex_attr_free(&attr);
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
    mutex& operator=(mutex&& other) = delete;

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
	void lock() noexcept {
		ABT_mutex_lock(m_mutex);
	}

    /**
     * @brief Lock the mutex in low priority.
     */
	void lock_low() noexcept {
		ABT_mutex_lock_low(m_mutex);
	}

    /**
     * @brief Lock the mutex without context switch.
     */
	void spin_lock() noexcept {
		ABT_mutex_spinlock(m_mutex);
	}

    /**
     * @brief Try locking the mutex without blocking.
     *
     * @return true if it managed to lock the mutex.
     */
	bool try_lock() noexcept {
		return (ABT_SUCCESS == ABT_mutex_trylock(m_mutex));
	}

    /**
     * @brief Unlock the mutex.
     */
	void unlock() noexcept {
		ABT_mutex_unlock(m_mutex);
	}

    /**
     * @brief Hand over the mutex within the ES.
     */
	void unlock_se() noexcept {
		ABT_mutex_unlock_se(m_mutex);
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
     * @brief Move assignment operator is deleted.
     */
    recursive_mutex& operator=(recursive_mutex&& other) = delete;
};

}

#endif /* end of include guard */
