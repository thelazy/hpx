//  Copyright (c) 2015 Thomas Heller
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_RUNTIME_COMPONENTS_SERVER_COMPONENT_BASE_HPP
#define HPX_RUNTIME_COMPONENTS_SERVER_COMPONENT_BASE_HPP

#include <hpx/config.hpp>
#include <hpx/runtime/agas/interface.hpp>
#include <hpx/runtime/applier/applier.hpp>
#include <hpx/runtime/applier/bind_naming_wrappers.hpp>
#include <hpx/runtime/applier_fwd.hpp>
#include <hpx/runtime/components/component_type.hpp>
#include <hpx/runtime/components/server/create_component_fwd.hpp>
#include <hpx/runtime/components_fwd.hpp>
#include <hpx/runtime/naming/address.hpp>
#include <hpx/runtime/naming/name.hpp>
#include <hpx/runtime_fwd.hpp>
#include <hpx/throw_exception.hpp>
#include <hpx/traits/is_component.hpp>
#include <hpx/util/assert.hpp>
#include <hpx/util/unique_function.hpp>

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <sstream>
#include <type_traits>
#include <utility>

namespace hpx {
namespace detail {
    HPX_API_EXPORT naming::gid_type get_next_id(std::size_t count = 1);
}
}

namespace hpx { namespace components {
    template <typename Component>
    class component;

    template <typename Component>
    class component_base : public traits::detail::component_tag
    {
    protected:
        typedef typename std::conditional<
            std::is_same<Component, detail::this_type>::value, component_base,
            Component>::type this_component_type;

        Component& derived()
        {
            return static_cast<Component&>(*this);
        }
        Component const& derived() const
        {
            return static_cast<Component const&>(*this);
        }

    public:
        typedef this_component_type wrapped_type;
        typedef this_component_type base_type_holder;
        typedef component<this_component_type> wrapping_type;

        /// \brief Construct an empty component
        component_base()
        {
        }

        /// \brief Destruct a component
        ~component_base()
        {
            if (gid_)
            {
                error_code ec;
                agas::unbind(launch::sync, gid_, 1, ec);
            }
        }

        // Copy construction and copy assignment should not copy the gid_.
        component_base(component_base const&)
        {
        }

        component_base& operator=(component_base const&)
        {
            return *this;
        }

        // just move our gid_
        component_base(component_base&& rhs)
          : gid_(std::move(rhs.gid_))
        {
        }

        component_base& operator=(component_base&& rhs)
        {
            if (this != &rhs)
                gid_ = std::move(rhs.gid_);
            return *this;
        }

        /// \brief finalize() will be called just before the instance gets
        ///        destructed
        void finalize()
        {
        }

        naming::address get_current_address() const
        {
            return naming::address(get_locality(),
                components::get_component_type<wrapped_type>(),
                std::uint64_t(static_cast<this_component_type const*>(this)));
        }

#if defined(HPX_HAVE_CXX11_EXTENDED_FRIEND_DECLARATIONS) &&                    \
    !defined(__NVCC__) && !defined(__CUDACC__)
    protected:
        // declare friends which are allowed to access get_base_gid()
        template <typename Component_, typename...Ts>
        friend naming::gid_type server::create(Ts&&... ts);

        template <typename Component, typename...Ts>
        friend naming::gid_type server::create_migrated(
            naming::gid_type const& gid, void** p, Ts&&...ts);

        template <typename Component, typename...Ts>
        friend std::vector<naming::gid_type> bulk_create(std::size_t count, Ts&&...ts);
#endif

        // Create a new GID (if called for the first time), assign this
        // GID to this instance of a component and register this gid
        // with the AGAS service
        //
        // Returns he global id (GID) assigned to this instance of a component
        naming::gid_type get_base_gid(
            naming::gid_type const& assign_gid = naming::invalid_gid) const
        {
            if (!gid_)
            {
                naming::address addr(get_current_address());
                if (!assign_gid)
                {
                    gid_ = hpx::detail::get_next_id();
                    if (!applier::bind_gid_local(gid_, addr))
                    {
                        std::ostringstream strm;
                        strm << "failed to bind id " << gid_
                             << "to locality: " << hpx::get_locality();

                        gid_ = naming::invalid_gid;    // invalidate GID

                        HPX_THROW_EXCEPTION(duplicate_component_address,
                            "component_base<Component>::get_base_gid",
                            strm.str());
                    }
                }
                else
                {
                    applier::applier& appl = hpx::applier::get_applier();
                    gid_ = assign_gid;
                    naming::detail::strip_credits_from_gid(gid_);

                    if (!agas::bind(
                            launch::sync, gid_, addr, appl.get_locality_id()))
                    {
                        std::ostringstream strm;
                        strm << "failed to rebind id " << gid_
                             << "to locality: " << hpx::get_locality();

                        gid_ = naming::invalid_gid;    // invalidate GID

                        HPX_THROW_EXCEPTION(duplicate_component_address,
                            "component_base<Component>::get_base_gid",
                            strm.str());
                    }
                }
            }

            std::unique_lock<naming::gid_type::mutex_type> l(gid_.get_mutex());

            if (!naming::detail::has_credits(gid_))
            {
                naming::gid_type gid = gid_;
                return gid;
            }

            // on first invocation take all credits to avoid a self reference
            naming::gid_type gid = gid_;

            naming::detail::strip_credits_from_gid(
                const_cast<naming::gid_type&>(gid_));

            HPX_ASSERT(naming::detail::has_credits(gid));

            // We have to assume this credit was split as otherwise the gid
            // returned at this point will control the lifetime of the
            // component.
            naming::detail::set_credit_split_mask_for_gid(gid);
            return gid;
        }

    public:
        naming::id_type get_id() const
        {
            // all credits should have been taken already
            naming::gid_type gid = derived().get_base_gid();
            HPX_ASSERT(!naming::detail::has_credits(gid));

            // any (subsequent) invocation causes the credits to be replenished
            naming::detail::replenish_credits(gid);
            return naming::id_type(gid, naming::id_type::managed);
        }

        naming::id_type get_unmanaged_id() const
        {
            return naming::id_type(
                derived().get_base_gid(), naming::id_type::managed);
        }

#if defined(HPX_HAVE_COMPONENT_GET_GID_COMPATIBILITY)
        HPX_DEPRECATED(HPX_DEPRECATED_MSG)
        naming::id_type get_gid() const
        {
            return get_id();
        }
#endif

        // Pinning functionality
        void pin()
        {
        }
        void unpin()
        {
        }
        std::uint32_t pin_count() const
        {
            return 0;
        }

        void mark_as_migrated()
        {
            // If this assertion is triggered then this component instance is
            // being migrated even if the component type has not been enabled
            // to support migration.
            HPX_ASSERT(false);
        }

        void on_migrated()
        {
            // If this assertion is triggered then this component instance is being
            // migrated even if the component type has not been enabled to support
            // migration.
            HPX_ASSERT(false);
        }

    protected:
        mutable naming::gid_type gid_;
    };
}}

#endif
