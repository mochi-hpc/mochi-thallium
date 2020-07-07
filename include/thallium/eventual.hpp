/*
 * Copyright (c) 2017 UChicago Argonne, LLC
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_EVENTUAL_HPP
#define __THALLIUM_EVENTUAL_HPP

#include <abt.h>
#include <thallium/exception.hpp>
#include <type_traits>

namespace thallium {

/**
 * Exception class thrown by the eventual class.
 */
class eventual_exception : public exception {
  public:
    template <typename... Args>
    eventual_exception(Args&&... args)
    : exception(std::forward<Args>(args)...) {}
};

#define TL_EVENTUAL_EXCEPTION(__fun, __ret)                                    \
    eventual_exception(#__fun, " returned ", abt_error_get_name(__ret), " (",  \
                       abt_error_get_description(__ret), ") in ", __FILE__,    \
                       ":", __LINE__);

#define TL_EVENTUAL_ASSERT(__call)                                             \
    {                                                                          \
        int __ret = __call;                                                    \
        if(__ret != ABT_SUCCESS) {                                             \
            throw TL_EVENTUAL_EXCEPTION(__call, __ret);                        \
        }                                                                      \
    }

/**
 * @brief The eventual class wraps an ABT_eventual object.
 * It is a template class, with the template type T being
 * the object stored by the eventual. T must be default-constructible
 * and assignable.
 */
template <typename T> class eventual {
  public:
    /**
     * @brief Type of value stored by the eventual.
     */
    using value_type =
        typename std::remove_reference<typename std::remove_cv<T>::type>::type;
    /**
     * @brief Native handle type.
     */
    using native_handle_type = ABT_eventual;

  private:
    ABT_eventual m_eventual;
    value_type   m_value;

  public:
    /**
     * @brief Get the underlying native handle.
     *
     * @return The underlying native handle.
     */
    ABT_eventual native_handle() const noexcept { return m_eventual; }

    /**
     * @brief Constryctor.
     */
    eventual() { TL_EVENTUAL_ASSERT(ABT_eventual_create(0, &m_eventual)); }

    /**
     * @brief Destructor.
     */
    ~eventual() { ABT_eventual_free(&m_eventual); }

    /**
     * @brief Copy constructor is deleted.
     */
    eventual(const eventual& other) = delete;

    /**
     * @brief Copy assignment operator is deleted.
     */
    eventual& operator=(const eventual& other) = delete;

    /**
     * @brief Move assignment operator. If the left and right
     * operands are different, this function will assign the
     * right operand's resource to the left. This invalidates
     * the right operand.
     */
    eventual& operator=(eventual&& other) {
        if(this == other.m_eventual)
            return *this;
        if(m_eventual != ABT_EVENTUAL_NULL) {
            TL_EVENTUAL_ASSERT(ABT_eventual_free(&m_eventual));
        }
        m_eventual       = other.m_eventual;
        other.m_eventual = ABT_EVENTUAL_NULL;
        return *this;
    }

    /**
     * @brief Move constructor.
     */
    eventual(eventual&& other)
    : m_eventual(other.m_eventual) {
        other.m_eventual = ABT_EVENTUAL_NULL;
    }

    /**
     * @brief Set the eventual's value (by copy).
     *
     * @param val Value to give the eventual.
     */
    void set_value(const T& val) {
        m_value = val;
        TL_EVENTUAL_ASSERT(ABT_eventual_set(m_eventual, nullptr, 0));
    }

    /**
     * @brief Set the eventual's value (by moving).
     *
     * @param val Value to give the eventual.
     */
    void set_value(T&& val) {
        m_value = std::move(val);
        TL_EVENTUAL_ASSERT(ABT_eventual_set(m_eventual, nullptr, 0));
    }

    /**
     * @brief Wait on the eventual.
     *
     * @return The value stored in the eventual.
     */
    value_type wait() {
        TL_EVENTUAL_ASSERT(ABT_eventual_wait(m_eventual, nullptr));
        return m_value;
    }

    /**
     * @brief Test the eventual.
     */
    bool test() {
        int flag;
        TL_EVENTUAL_ASSERT(ABT_eventual_test(m_eventual, nullptr, &flag));
        return flag;
    }

    /**
     * @brief Reset the eventual.
     */
    void reset() { TL_EVENTUAL_ASSERT(ABT_eventual_reset(m_eventual)); }
};

/**
 * @brief Specialization of eventual class for T=void
 */
template <> class eventual<void> {
  public:
    /**
     * @brief Native handle type.
     */
    using native_handle_type = ABT_eventual;

  private:
    ABT_eventual m_eventual;

  public:
    /**
     * @brief Returns the underlying native handle.
     *
     * @return The underlying native handle.
     */
    ABT_eventual native_handle() const noexcept { return m_eventual; }

    /**
     * @brief Constructor.
     */
    eventual() { TL_EVENTUAL_ASSERT(ABT_eventual_create(0, &m_eventual)); }

    /**
     * @brief Destructor.
     */
    ~eventual() { ABT_eventual_free(&m_eventual); }

    /**
     * @brief Copy constructor is deleted.
     */
    eventual(const eventual& other) = delete;

    /**
     * @brief Copy assignment operator is deleted.
     */
    eventual& operator=(const eventual& other) = delete;

    /**
     * @brief Move assignment operator. If the left and right
     * operands are different, this function will assign the
     * right operand's resource to the left. This invalidates
     * the right operand.
     */
    eventual& operator=(eventual&& other) {
        if(m_eventual == other.m_eventual)
            return *this;
        if(m_eventual != ABT_EVENTUAL_NULL) {
            TL_EVENTUAL_ASSERT(ABT_eventual_free(&m_eventual));
        }
        m_eventual       = other.m_eventual;
        other.m_eventual = ABT_EVENTUAL_NULL;
        return *this;
    }

    /**
     * @brief Move constructor.
     */
    eventual(eventual&& other) noexcept
    : m_eventual(other.m_eventual) {
        other.m_eventual = ABT_EVENTUAL_NULL;
    }

    /**
     * @brief Set the eventual.
     */
    void set_value() {
        TL_EVENTUAL_ASSERT(ABT_eventual_set(m_eventual, nullptr, 0));
    }

    /**
     * @brief Wait on the eventual.
     */
    void wait() { TL_EVENTUAL_ASSERT(ABT_eventual_wait(m_eventual, nullptr)); }

    /**
     * @brief Test the eventual.
     */
    bool test() {
        int flag;
        TL_EVENTUAL_ASSERT(ABT_eventual_test(m_eventual, nullptr, &flag));
        return flag;
    }

    /**
     * @brief Reset the eventual.
     */
    void reset() { TL_EVENTUAL_ASSERT(ABT_eventual_reset(m_eventual)); }
};

} // namespace thallium

#undef TL_EVENTUAL_EXCEPTION
#undef TL_EVENTUAL_ASSERT

#endif /* end of include guard */
