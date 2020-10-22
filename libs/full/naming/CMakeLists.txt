# Copyright (c) 2019-2020 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

if(NOT HPX_WITH_DISTRIBUTED_RUNTIME)
  return()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(naming_headers hpx/naming/credit_handling.hpp hpx/naming/naming.hpp)

# cmake-format: off
set(naming_compat_headers
    hpx/runtime/naming_fwd.hpp => hpx/modules/naming.hpp
)
# cmake-format: on

set(naming_sources credit_handling.cpp)

include(HPX_AddModule)
add_hpx_module(
  full naming
  COMPATIBILITY_HEADERS ON # added in hpx V1.6.0
  DEPRECATION_WARNINGS
  GLOBAL_HEADER_GEN ON
  SOURCES ${naming_sources}
  HEADERS ${naming_headers}
  COMPAT_HEADERS ${naming_compat_headers}
  DEPENDENCIES hpx_core
  MODULE_DEPENDENCIES hpx_agas hpx_naming_base
  CMAKE_SUBDIRS examples tests
)