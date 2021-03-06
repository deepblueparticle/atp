#
# Tests for platform code

message(STATUS "TESTS: Platform ==============================================")

# test_platform_message_processor
set(test_platform_message_processor_incs
  ${GEN_DIR}
  ${SRC_DIR}
  ${TEST_DIR}
)
set(test_platform_message_processor_srcs
  ${TEST_DIR}/AllTests.cpp
  MessageProcessorTest.cpp
)
set(test_platform_message_processor_libs
  atp_platform_all
  gflags
  )
cpp_gtest(test_platform_message_processor)


# test_platform_marketdata_handler
set(test_platform_marketdata_handler_incs
  ${GEN_DIR}
  ${SRC_DIR}
  ${TEST_DIR}
)
set(test_platform_marketdata_handler_srcs
  ${TEST_DIR}/AllTests.cpp
  MarketDataHandlerTest.cpp
)
set(test_platform_marketdata_handler_libs
  atp_platform_all
  gflags
  )
cpp_gtest(test_platform_marketdata_handler)

# test_platform_indicator
set(test_platform_indicator_incs
  ${GEN_DIR}
  ${SRC_DIR}
  ${TEST_DIR}
)
set(test_platform_indicator_srcs
  ${TEST_DIR}/AllTests.cpp
  IndicatorTest.cpp
)
set(test_platform_indicator_libs
  atp_platform_all
  gflags
  )
cpp_gtest(test_platform_indicator)

# test_platform_strategy
set(test_platform_strategy_incs
  ${GEN_DIR}
  ${SRC_DIR}
  ${TEST_DIR}
)
set(test_platform_strategy_srcs
  ${TEST_DIR}/AllTests.cpp
  StrategyTest.cpp
)
set(test_platform_strategy_libs
  atp_platform_all
  gflags
  )
cpp_gtest(test_platform_strategy)

add_custom_target(all_platform_tests)
add_dependencies(all_platform_tests
  test_platform_message_processor
  test_platform_marketdata_handler
  test_platform_indicator
  test_platform_strategy
)
