/*
 * (C) 2017 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_PROVIDER_HANDLE_HPP
#define __THALLIUM_PROVIDER_HANDLE_HPP

#include <cstdint>
#include <string>
#include <margo.h>
#include <thallium/endpoint.hpp>
#include <thallium/margo_exception.hpp>

namespace thallium {

/**
 * The provider_handle class encapsulates an enpoint
 * with a provider id to reference a particular provider object
 * at a given address. provider_handle inherites from endpoint
 * so it can be used wherever endpoint is needed. 
 */
class provider_handle : public endpoint {

private:

    uint16_t m_provider_id;

public:

    /**
     * @brief Constructor from a HG address.
     *
     * @param e engine.
     * @param addr Address to encapsulate.
     * @param provider_id provider id.
     */
    provider_handle(engine& e, hg_addr_t addr, uint16_t provider_id=0, bool take_ownership=true)
    : endpoint(e, addr, take_ownership), m_provider_id(provider_id) {}

    /**
     * @brief Constructor.
     *
     * @param e enpoint to encapsulate.
     * @param provider_id provider id.
     */
	provider_handle(endpoint e, uint16_t provider_id=0)
	: endpoint(std::move(e)), m_provider_id(provider_id) {}

    /**
     * @brief Default constructor defined so that provider_handlers can
     * be member of other objects and assigned later.
     */
    provider_handle() = default;

    /**
     * @brief Copy constructor.
     */
	provider_handle(const provider_handle& other) = default;

    /**
     * @brief Move constructor.
     */
	provider_handle(provider_handle&& other) = default;

    /**
     * @brief Copy-assignment operator.
     */
	provider_handle& operator=(const provider_handle& other) = default;

    /**
     * @brief Move-assignment operator.
     */
	provider_handle& operator=(provider_handle&& other) = default;
	
    /**
     * @brief Destructor.
     */
	~provider_handle() = default;

    /**
     * @brief Get the provider id.
     *
     * @return the provider id.
     */
    uint16_t provider_id() const {
        return m_provider_id;
    }
};

}

#endif
