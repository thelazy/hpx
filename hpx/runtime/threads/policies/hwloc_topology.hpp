////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2007-2012 Hartmut Kaiser
//  Copyright (c) 2008-2009 Chirag Dekate, Anshul Tandon
//  Copyright (c)      2012 Thomas Heller
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////

#if !defined(HPX_50DFC0FC_EE99_43F5_A918_01EC45A58036)
#define HPX_50DFC0FC_EE99_43F5_A918_01EC45A58036

#include <hwloc.h>

#include <boost/format.hpp>

namespace hpx { namespace threads
{

struct topology
{
    topology()
    { // {{{
        std::size_t const num_of_cores = hardware_concurrency();

        numa_node_numbers_.reserve(num_of_cores);
        numa_node_affinity_masks_.reserve(num_of_cores);
        ns_numa_node_affinity_masks_.reserve(num_of_cores);
        thread_affinity_masks_.reserve(num_of_cores);
        ns_thread_affinity_masks_.reserve(num_of_cores);

        // Initialize each set of data entirely, as some of the initialization
        // routines rely on access to other pieces of topology data. The
        // compiler will optimize the loops where possible anyways.

        for (std::size_t i = 0; i < num_of_cores; ++i)
            numa_node_numbers_.push_back(init_numa_node_number(i));

        for (std::size_t i = 0; i < num_of_cores; ++i)
        {
            numa_node_affinity_masks_.push_back(
                init_numa_node_affinity_mask(i, false));
            ns_numa_node_affinity_masks_.push_back(
                init_numa_node_affinity_mask(i, true));
        }

        for (std::size_t i = 0; i < num_of_cores; ++i)
        {
            thread_affinity_masks_.push_back(
                init_thread_affinity_mask(i, false));
            ns_thread_affinity_masks_.push_back(
                init_thread_affinity_mask(i, true));
        }
    } // }}}

    std::size_t get_numa_node_number(
        std::size_t num_thread
      , error_code& ec = throws
        ) const
    { // {{{
        if (num_thread < numa_node_numbers_.size())
        {
            if (&ec != &throws)
                ec = make_success_code();

            return numa_node_numbers_[num_thread];
        }

        else
        {
            HPX_THROWS_IF(ec, bad_parameter
              , "hpx::threads::topology::get_numa_node_number"
              , boost::str(boost::format(
                    "thread number %1% is out of range")
                    % num_thread));
            return std::size_t(-1);
        }
    } // }}}

    std::size_t get_numa_node_affinity_mask(
        std::size_t num_thread
      , bool numa_sensitive
      , error_code& ec = throws
        ) const
    { // {{{
        if (num_thread < numa_node_affinity_masks_.size())
        {
            if (&ec != &throws)
                ec = make_success_code();

            return numa_sensitive ? ns_numa_node_affinity_masks_[num_thread]
                                  : numa_node_affinity_masks_[num_thread];
        }

        else
        {
            HPX_THROWS_IF(ec, bad_parameter
              , "hpx::threads::topology::get_numa_node_affinity_mask"
              , boost::str(boost::format(
                    "thread number %1% is out of range")
                    % num_thread));
            return 0;
        }
    } // }}}

    std::size_t get_thread_affinity_mask(
        std::size_t num_thread
      , bool numa_sensitive
      , error_code& ec = throws
        ) const
    { // {{{
        if (num_thread < thread_affinity_masks_.size())
        {
            if (&ec != &throws)
                ec = make_success_code();

            return numa_sensitive ? ns_thread_affinity_masks_[num_thread]
                                  : thread_affinity_masks_[num_thread];
        }

        else
        {
            HPX_THROWS_IF(ec, bad_parameter
              , "hpx::threads::topology::get_thread_affinity_mask"
              , boost::str(boost::format(
                    "thread number %1% is out of range")
                    % num_thread));
            return 0;
        }
    } // }}}

    void set_thread_affinity(
        boost::thread&
      , std::size_t //num_thread
      , bool //numa_sensitive
      , error_code& ec = throws
        ) const
    {
        if (&ec != &throws)
            ec = make_success_code();
    }

    void set_thread_affinity(
        std::size_t num_thread
      , bool numa_sensitive
      , error_code& ec = throws
        ) const
    { // {{{
        // Figure out how many cores are available.
        hwloc_topology_t topo;

        if (0 == hwloc_topology_init(&topo) &&
            0 == hwloc_topology_load(topo))
        {
            // Now set the affinity to the required PU.
            std::size_t mask
                = get_thread_affinity_mask(num_thread, numa_sensitive);
            hwloc_cpuset_t cpuset = hwloc_bitmap_alloc();

            hwloc_bitmap_from_ith_ulong(cpuset, 0, mask & 0xFFFFFFFF);
            hwloc_bitmap_set_ith_ulong(cpuset, 1, (mask >> 32) & 0xFFFFFFFF);

#if defined(DEBUG)
            char buf[1024];
            hwloc_bitmap_snprintf(buf, 1024, cpuset);
#endif

            if (hwloc_set_cpubind(topo, cpuset,
                  HWLOC_CPUBIND_STRICT | HWLOC_CPUBIND_THREAD))
            {
                // Strict binding not supported or failed, try weak binding.
                if (hwloc_set_cpubind(topo, cpuset, HWLOC_CPUBIND_THREAD))
                {
                    hwloc_bitmap_free(cpuset);
                    hwloc_topology_destroy(topo);

                    HPX_THROWS_IF(ec, kernel_error
                      , "hpx::threads::topology::set_thread_affinity_mask"
                      , boost::str(boost::format(
                            "failed to set thread %1% affinity mask")
                            % num_thread));

                    if (ec)
                        return;
                }
            }

            hwloc_bitmap_free(cpuset);
        }

        if (&ec != &throws)
            ec = make_success_code();

        hwloc_topology_destroy(topo);
    } // }}}

    std::size_t get_thread_affinity_mask_from_lva(
        naming::address::address_type
      , error_code& ec = throws
        ) const
    { // {{{
        if (&ec != &throws)
            ec = make_success_code();

        return 0;
    } // }}}

  private:
    std::size_t init_numa_node_number(
        std::size_t num_thread
        )
    { // {{{
        if (std::size_t(-1) == num_thread)
             return std::size_t(-1);

        hwloc_topology_t topo;

        if (0 == hwloc_topology_init(&topo) &&
            0 == hwloc_topology_load(topo))
        {
            hwloc_obj_t obj = hwloc_get_obj_by_type(topo, HWLOC_OBJ_PU,
                static_cast<unsigned>(num_thread));
            while (obj)
            {
                if (hwloc_compare_types(obj->type, HWLOC_OBJ_NODE) == 0)
                {
                    std::size_t numa_node = obj->logical_index;
                    hwloc_topology_destroy(topo);
                    return numa_node;
                }
                obj = obj->parent;
            }
        }

        hwloc_topology_destroy(topo);
        return std::size_t(-1);
    } // }}}

    void extract_node_mask(
        hwloc_topology_t topo
      , hwloc_obj_t parent
      , std::size_t& mask
        )
    { // {{{
        hwloc_obj_t obj = hwloc_get_next_child(topo, parent, NULL);
        while (obj)
        {
            if (hwloc_compare_types(HWLOC_OBJ_PU, obj->type) == 0)
            {
                do {
                    mask |= (static_cast<std::size_t>(1) << obj->os_index);
                    obj = hwloc_get_next_child(topo, parent, obj);
                } while (obj != NULL &&
                         hwloc_compare_types(HWLOC_OBJ_PU, obj->type) == 0);
                return;
            }
            extract_node_mask(topo, obj, mask);
            obj = hwloc_get_next_child(topo, parent, obj);
        }
    } // }}}

    std::size_t init_numa_node_affinity_mask(
        std::size_t num_thread
      , bool
        )
    { // {{{
        std::size_t node_affinity_mask = 0;
        std::size_t numa_node = get_numa_node_number(num_thread);

        if (std::size_t(-1) == numa_node)
            return 0;

        hwloc_topology_t topo;

        if (0 == hwloc_topology_init(&topo) &&
            0 == hwloc_topology_load(topo))
        {
            hwloc_obj_t numa_node_obj = hwloc_get_obj_by_type(topo,
                HWLOC_OBJ_NODE, static_cast<unsigned>(numa_node));
            if (numa_node_obj)
            {
                extract_node_mask(topo, numa_node_obj, node_affinity_mask);
                hwloc_topology_destroy(topo);
                return node_affinity_mask;
            }
        }

        hwloc_topology_destroy(topo);
        HPX_THROW_EXCEPTION(kernel_error
          , "hpx::threads::topology::init_numa_node_affinity_mask"
          , boost::str(boost::format(
                "failed to initialize NUMA node affinity mask for thread %1%")
                % num_thread));
        return 0;
    } // }}}

    std::size_t init_thread_affinity_mask(
        std::size_t num_thread
      , bool numa_sensitive
        )
    { // {{{
        std::size_t num_of_cores = hardware_concurrency();
        std::size_t affinity = num_thread % num_of_cores;

        hwloc_topology_t topo;

        if (0 == hwloc_topology_init(&topo) &&
            0 == hwloc_topology_load(topo))
        {
            std::size_t numa_nodes = 0;
            {
                int numa_nodes_int = hwloc_get_nbobjs_by_type(topo, HWLOC_OBJ_NODE);
                if(numa_nodes_int == -1) return 0;
                numa_nodes = static_cast<std::size_t>(numa_nodes_int);
            }
            hwloc_topology_destroy(topo);

            if (numa_nodes == 0)
                return 0;

            std::size_t num_of_cores_per_numa_node = num_of_cores / numa_nodes;
            std::size_t node_affinity_mask = 0;
            std::size_t mask = 0;

            node_affinity_mask
                = get_numa_node_affinity_mask(num_thread, numa_sensitive);

            std::size_t node_index = affinity % num_of_cores_per_numa_node;

            // We need to detect the node_index-th bit which is set in
            // node_affinity_mask, this bit must be set in the result mask.
            std::size_t count = 0;
            for (std::size_t i = 0; i < 64; ++i)
            {
                // Is the i-th bit set?
                if (node_affinity_mask & (static_cast<std::size_t>(1) << i))
                {
                    if (count == node_index)
                    {
                        mask = (static_cast<std::size_t>(1) << i);
                        break;
                    }
                    ++count;
                }
            }

            return mask;
        }

        hwloc_topology_destroy(topo);
        return 0;
    } // }}}

    std::vector<std::size_t> numa_node_numbers_;

    std::vector<std::size_t> numa_node_affinity_masks_;
    std::vector<std::size_t> ns_numa_node_affinity_masks_;

    std::vector<std::size_t> thread_affinity_masks_;
    std::vector<std::size_t> ns_thread_affinity_masks_;
};

}}

#endif // HPX_50DFC0FC_EE99_43F5_A918_01EC45A58036

