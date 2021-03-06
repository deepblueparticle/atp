#
# Tests for common code

message(STATUS "TESTS: Common ================================================")

# Use standard string library instead of mvc foundation classes:
option(IB_USE_STD_STRING "Use standard c++ string (see Shared/IBString.h)." ON)
if (IB_USE_STD_STRING)
  add_definitions('-DIB_USE_STD_STRING')
endif (IB_USE_STD_STRING)

# CommonTests
set(test_common_incs
  ${GEN_DIR}
  ${SRC_DIR}
  ${TEST_DIR}
  ${IBAPI_ROOT}
  ${IBAPI_ROOT}/Shared
)
set(test_common_srcs
  ${TEST_DIR}/AllTests.cpp
  ExecutorTest.cpp
  FactoryTest.cpp
)
set(test_common_libs
  api_base
  ib_api_versioned_base
  atp_common
  atp_proto
  atp_zmq
  gflags
  glog
  )
cpp_gtest(test_common)

# MovingWindow
set(test_common_moving_window_incs
  ${GEN_DIR}
  ${SRC_DIR}
  ${TEST_DIR}
)
set(test_common_moving_window_srcs
  ${TEST_DIR}/AllTests.cpp
  MovingWindowTest.cpp
 )
set(test_common_moving_window_libs
  atp_common
  atp_proto
  atp_zmq
  gflags
  glog
  )
cpp_gtest(test_common_moving_window)

# TimeSeries
set(test_common_time_series_incs
  ${GEN_DIR}
  ${SRC_DIR}
  ${TEST_DIR}
)
set(test_common_time_series_srcs
  ${TEST_DIR}/AllTests.cpp
  TimeSeriesTest.cpp
 )
set(test_common_time_series_libs
  atp_common
  atp_proto
  atp_zmq
  gflags
  glog
  )
cpp_gtest(test_common_time_series)

add_custom_target(all_common_tests)
add_dependencies(all_common_tests
  test_common
  test_commom_moving_window
  test_commom_time_series
)
