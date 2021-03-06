# Project CMake File

cmake_minimum_required(VERSION 2.6)

# Project
project(ATP)


# Platform specific parameters
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
   set(OperatingSystem "Mac OS X")
   set(OPTIMIZE_LEVEL 0)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
   set(OperatingSystem "Linux")
   set(OPTIMIZE_LEVEL 4)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")


# -O2 -fomit-frame-pointer for prod (no profiling)

# Build for debugging and profiling
# See http://gcc.gnu.org/onlinedocs/gcc/Option-Summary.html
#
set(CFLAGS "-pg -g -W")

# Macros to include
include(${PROJECT_SOURCE_DIR}/tools/cmake/Init.cmk)


# GIT Commit Hash
execute_process(
  COMMAND git rev-list --max-count=1 --reverse HEAD
  OUTPUT_VARIABLE GIT_LAST_COMMIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
  COMMAND git log -1
  COMMAND tail -1
  COMMAND sed -e "s/^[ ]*//g"
  OUTPUT_VARIABLE GIT_LAST_COMMIT_MSG
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
message(STATUS
  "GIT COMMIT - " ${GIT_LAST_COMMIT_HASH}
)
message(STATUS
  "GIT COMMIT - " ${GIT_LAST_COMMIT_MSG}
)

# Common dependencies
#

# IB API VERSION
set(IBAPI_VERSION_MAJOR 9)
set(IBAPI_VERSION_MINOR 66)


set(IBAPI_VERSION ${IBAPI_VERSION_MAJOR}.${IBAPI_VERSION_MINOR})
add_definitions("-DIBAPI_${IBAPI_VERSION_MAJOR}${IBAPI_VERSION_MINOR}")
set(IBAPI_IMPL_DIR api${IBAPI_VERSION_MAJOR}${IBAPI_VERSION_MINOR})
set(IBAPI_VERSION_TEST_PREFIX V${IBAPI_VERSION_MAJOR}${IBAPI_VERSION_MINOR})

# The class filename that implements the IBAPI messages
set(IBAPI_ROOT ${THIRD_PARTY_DIR}/interactivebrokers/api-${IBAPI_VERSION})
set(IBCLIENT_VERSION 0.1)

message(STATUS
  "InteractiveBrokers API version ${IBAPI_VERSION}, implDir=${IBAPI_IMPL_DIR}")

# FIX/QuickFix paths
set(FIX_VERSION 4.2)
set(QUICKFIX_ROOT ${THIRD_PARTY_DIR}/quickfix/src/C++)

message(STATUS
  "ZMQ Version 3.2")
add_definitions('-DZMQ_3X')


# R Integration
execute_process(
  COMMAND R CMD config --cppflags
  OUTPUT_VARIABLE R_CPP_FLAGS
)
message(STATUS "R cppflags: " ${R_CPP_FLAGS})

execute_process(
  COMMAND R CMD config --ldflags
  OUTPUT_VARIABLE R_LD_FLAGS
)
message(STATUS "R ldflags: " ${R_LD_FLAGS})

execute_process(
  COMMAND Rscript -e "Rcpp:::CxxFlags()"
  OUTPUT_VARIABLE RCPP_CXX_FLAGS
)
message(STATUS "Rcpp cxxflags: " ${RCPP_CXX_FLAGS})

execute_process(
  COMMAND Rscript -e "Rcpp:::LdFlags()"  | sed -e 's/-l/g'
  OUTPUT_VARIABLE RCPP_LD_FLAGS
)
message(STATUS "Rcpp ldflags: " ${RCPP_LD_FLAGS})

execute_process(
  COMMAND Rscript -e "Rcpp:::CxxFlags()" | sed -e 's/-I/g'
  OUTPUT_VARIABLE RCPP_INCLUDE_PATH
)
message(STATUS "Rcpp Include Path: " ${RCPP_INCLUDE_PATH})


# Generate paths for Emacs
list(APPEND emacs_includes
  ${IBAPI_ROOT}
  ${IBAPI_ROOT}/Shared
  ${IBAPI_ROOT}/PosixSocketClient
  ${RCPP_INCLUDE_PATH}
  ${THIRD_PARTY_BUILD_DIR}/include
)

#list(APPEND emacs_sys_includes
#)
emacs_ide_project("${emacs_includes}" "${emacs_sys_includes}")

# Use the api version defined in the top level cmake file:
set(API_ROOT ${COMMON_DIR}/ib/api/${IBAPI_VERSION})

# The macros here will first find any locally installed
# libraries.  If not, it will build them in build/third_party.
# Consider install these on the local host to ensure proper
# function and more efficient builds.
third_party(gflags)
set(glog_deps gflags)
third_party(glog)

# Be sure to update the Makefile to include -fPIC in the 
# compiler options
message(STATUS "Make sure leveldb's Makefile has the -fPIC option set for shared libs")
third_party_plain_make(leveldb)

# Include the subdirectories required for this project:
add_subdirectory(src/cpp)
add_subdirectory(prototype/cpp)
add_subdirectory(test/cpp)
add_subdirectory(R-packages)

# Add this dependency here to make sure all the libs are built before R build
# starts
add_dependencies(all_R_packages
  all_mains
  all_tests
  all_prototypes
)

add_custom_target(world)
add_dependencies(world
  all_mains
  all_tests
  all_prototypes
#  all_R_packages
)
