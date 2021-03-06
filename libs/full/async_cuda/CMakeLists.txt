# Copyright (c) 2019 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

if(NOT HPX_WITH_ASYNC_CUDA)
  return()
endif()

if(NOT TARGET Cuda::cuda)
  message(
    FATAL_ERROR
      "Cuda future support has been enabled, but the Cuda installation is not setup"
  )
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Default location is $HPX_ROOT/libs/async_cuda/include
set(async_cuda_headers
    hpx/async_cuda/cuda_event.hpp
    hpx/async_cuda/cuda_executor.hpp
    hpx/async_cuda/cuda_exception.hpp
    hpx/async_cuda/cuda_future.hpp
    hpx/async_cuda/cublas_executor.hpp
    hpx/async_cuda/get_targets.hpp
    hpx/async_cuda/target.hpp
)

# Default location is $HPX_ROOT/libs/async_cuda/include_compatibility
# cmake-format: off
set(async_cuda_compat_headers
    hpx/compute/cuda/get_targets.hpp => hpx/async_cuda/get_targets.hpp
    hpx/compute/cuda/target.hpp => hpx/async_cuda/target.hpp
)
# cmake-format: on

set(async_cuda_sources cuda_future.cpp cuda_target.cpp get_targets.cpp)

if(HPX_WITH_DISTRIBUTED_RUNTIME)
  set(async_cuda_extra_module_dependencies
      hpx_actions_base hpx_async_distributed hpx_performance_counters
  )
endif()

include(HPX_AddModule)
add_hpx_module(
  full async_cuda
  COMPATIBILITY_HEADERS ON
  DEPRECATION_WARNINGS
  GLOBAL_HEADER_GEN ON
  SOURCES ${async_cuda_sources}
  HEADERS ${async_cuda_headers}
  COMPAT_HEADERS ${async_cuda_compat_headers}
  MODULE_DEPENDENCIES hpx_naming hpx_program_options hpx_runtime_configuration
                      hpx_runtime_local ${async_cuda_extra_module_dependencies}
  DEPENDENCIES hpx_core hpx_parallelism Cuda::cuda
  CMAKE_SUBDIRS examples tests
)
