#!/usr/bin/env python
'''
Copyright (c) 2018 Thomas Heller

Distributed under the Boost Software License, Version 1.0. (See accompanying
file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

create_library_skeleton.py - A tool to generate a library skeleton to be used
as library component of HPX
'''

import sys, os

if len(sys.argv) != 2:
    print('Usage: %s <lib_name>' % sys.argv[0])
    print('Generates the skeleton for lib_name in the current working directory')
    sys.exit(1)

lib_name = sys.argv[1]
lib_name_upper = lib_name.upper()
header_str = '=' * len(lib_name)

# CMake minimum version
cmake_version = '3.3.2'

cmake_header = f'''# Copyright (c) 2019 The STE||AR-Group
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
'''

readme_template = f'''<!-- Copyright (c) 2019 The STE||AR-Group                                         -->
<!--                                                                              -->
<!-- Distributed under the Boost Software License, Version 1.0. (See accompanying -->
<!-- file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)        -->

# {lib_name}

This library is part of HPX.

Extensive documentation can be found at
https://stellar-group.github.io/hpx/docs/sphinx/latest/html/libs/{lib_name}/docs/index.html
'''

index_rst = f'''..
    Copyright (c) 2019 The STE||AR-Group

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _libs_{lib_name}:

{header_str}
{lib_name}
{header_str}

'''

root_cmakelists_template = cmake_header + f'''
# We require at least CMake V{cmake_version}
cmake_minimum_required(VERSION {cmake_version} FATAL_ERROR)

project(HPX.{lib_name} CXX)

list(APPEND CMAKE_MODULE_PATH "${{CMAKE_CURRENT_SOURCE_DIR}}/cmake")

option(HPX_{lib_name_upper}_WITH_TESTS "Include tests for {lib_name}" On)

message(STATUS "{lib_name}: Configuring")

add_subdirectory(examples)
add_subdirectory(src)
add_subdirectory(tests)

message(STATUS "{lib_name}: Configuring done")
'''

examples_cmakelists_template = cmake_header + f'''
if (HPX_WITH_TESTS_EXAMPLES)
  add_hpx_pseudo_target(tests.examples.{lib_name})
  add_hpx_pseudo_dependencies(tests.examples tests.examples.{lib_name})
endif()

'''

tests_cmakelists_template = cmake_header + f'''
if (NOT HPX_WITH_TESTS AND HPX_TOP_LEVEL)
  return()
endif()
if (NOT HPX_{lib_name_upper}_WITH_TESTS)
  message(STATUS "Tests for {lib_name} disabled")
  return()
endif()

if (HPX_WITH_TESTS_UNIT)
  add_hpx_pseudo_target(tests.unit.{lib_name})
  add_hpx_pseudo_dependencies(tests.unit tests.unit.{lib_name})
  add_subdirectory(unit)
endif()

if (HPX_WITH_TESTS_REGRESSIONS)
  add_hpx_pseudo_target(tests.regressions.{lib_name})
  add_hpx_pseudo_dependencies(tests.regressions tests.regressions.{lib_name})
  add_subdirectory(regressions)
endif()

if (HPX_WITH_TESTS_BENCHMARKS)
  add_hpx_pseudo_target(tests.performance.{lib_name})
  add_hpx_pseudo_dependencies(tests.performance tests.performance.{lib_name})
  add_subdirectory(performance)
endif()

if (HPX_WITH_TESTS_HEADERS)
  add_hpx_lib_header_tests({lib_name})
endif()
'''

if lib_name != '--recreate-index':
    def mkdir(path):
        if not os.path.exists(path):
            os.makedirs(path)

    mkdir(lib_name)

    ################################################################################
    # Generate basic directory structure
    for subdir in ['cmake', 'docs', 'examples', 'include', 'src', 'tests']:
        path = os.path.join(lib_name, subdir)
        mkdir(path)
    # Generate include directory structure
    # Normalize path...
    include_path = ''.join(lib_name)
    path = os.path.join(lib_name, 'include', 'hpx', include_path)
    mkdir(path)
    path = os.path.join(lib_name, 'tests', 'unit')
    mkdir(path)
    path = os.path.join(lib_name, 'tests', 'regressions')
    mkdir(path)
    path = os.path.join(lib_name, 'tests', 'performance')
    mkdir(path)
    ################################################################################

    ################################################################################
    # Generate Readme skeleton
    f = open(os.path.join(lib_name, 'Readme.md'), 'w')
    f.write(readme_template)
    ################################################################################

    ################################################################################
    # Generate CMakeLists.txt skeletons

    # Generate top level CMakeLists.txt
    f = open(os.path.join(lib_name, 'CMakeLists.txt'), 'w')
    f.write(root_cmakelists_template)

    # Generate docs/index.rst
    f = open(os.path.join(lib_name, 'docs', 'index.rst'), 'w')
    f.write(index_rst)

    # Generate examples/CMakeLists.txt
    f = open(os.path.join(lib_name, 'examples', 'CMakeLists.txt'), 'w')
    f.write(examples_cmakelists_template)

    # Generate src/CMakeLists.txt
    f = open(os.path.join(lib_name, 'src', 'CMakeLists.txt'), 'w')
    f.write(cmake_header)

    # Generate tests/CMakeLists.txt
    f = open(os.path.join(lib_name, 'tests', 'CMakeLists.txt'), 'w')
    f.write(tests_cmakelists_template)

    # Generate tests/unit/CMakeLists.txt
    f = open(os.path.join(lib_name, 'tests', 'unit', 'CMakeLists.txt'), 'w')
    f.write(cmake_header)

    # Generate tests/regressions/CMakeLists.txt
    f = open(os.path.join(lib_name, 'tests', 'regressions', 'CMakeLists.txt'), 'w')
    f.write(cmake_header)
    f.write('\n')

    # Generate tests/performance/CMakeLists.txt
    f = open(os.path.join(lib_name, 'tests', 'performance', 'CMakeLists.txt'), 'w')
    f.write(cmake_header)
    ################################################################################

################################################################################

# Scan directory to get all libraries...
cwd = os.getcwd()
libs = sorted([ lib for lib in os.listdir(cwd) if os.path.isdir(lib) ])

# Adapting top level CMakeLists.txt
libs_cmakelists = cmake_header + f'''
# This file is auto generated. Please do not edit manually

include(HPX_CreateSymbolicLink)

# We create a special directory to collect all our modular headers, to make
# it easier to include those files. The directory is created from scratch if
# changes occured to avoid dangling links
execute_process(COMMAND "${{CMAKE_COMMAND}}" -E remove_directory ${{CMAKE_BINARY_DIR}}/include/hpx)
execute_process(COMMAND "${{CMAKE_COMMAND}}" -E make_directory ${{CMAKE_BINARY_DIR}}/include/hpx)
'''

libs_cmake_dir_add = '''
add_subdirectory({lib})
file(GLOB PP_INCLUDE_LIST
  LIST_DIRECTORIES true ${{DO_CONFIGURE_DEPENDS}}
  RELATIVE ${{CMAKE_CURRENT_SOURCE_DIR}}/{lib}/include/hpx/
  ${{CMAKE_CURRENT_SOURCE_DIR}}/{lib}/include/hpx/*)
foreach(include ${{PP_INCLUDE_LIST}})
  create_symbolic_link(
    ${{CMAKE_CURRENT_SOURCE_DIR}}/{lib}/include/hpx/${{include}}
    ${{CMAKE_BINARY_DIR}}/include/hpx/${{include}})
endforeach()
'''

for lib in libs:
    # Ignore subdirectories starting with _
    if not lib.startswith('_'):
        libs_cmakelists += libs_cmake_dir_add.format(lib = lib)
f = open(os.path.join(cwd, 'CMakeLists.txt'), 'w')
f.write(libs_cmakelists)

# Adapting top level index.rst
index_rst = f'''..
    Copyright (c) 2018-2019 The STE||AR-Group

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. toctree::
   :caption: Libraries
   :maxdepth: 2

'''
for lib in libs:
    index_rst += f'   /libs/{lib}/docs/index.rst\n'

f = open(os.path.join(cwd, 'index.rst'), 'w')
f.write(index_rst)

################################################################################

