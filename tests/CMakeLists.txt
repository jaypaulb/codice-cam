# Test configuration for CodiceCam

# Find testing framework (Google Test)
find_package(GTest QUIET)

if(GTest_FOUND)
    # Enable testing
    enable_testing()

    # Test source files (to be added as we develop)
    set(TEST_SOURCES
        # test_main.cpp
        # test_camera_manager.cpp
        # test_marker_detector.cpp
        # test_codice_decoder.cpp
        # test_tuio_bridge.cpp
    )

    # Create test executable
    add_executable(CodiceCamTests ${TEST_SOURCES})

    # Link test libraries
    target_link_libraries(CodiceCamTests
        GTest::gtest
        GTest::gtest_main
        # Add other test dependencies here
    )

    # Include directories
    target_include_directories(CodiceCamTests PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    # Add tests to CTest
    add_test(NAME CodiceCamTests COMMAND CodiceCamTests)

else()
    message(STATUS "Google Test not found. Tests will be skipped.")
    message(STATUS "To enable tests, install Google Test:")
    message(STATUS "  vcpkg install gtest")
endif()
