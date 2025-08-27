# Source files for CodiceCam

# Find required packages
if(USE_SYSTEM_PACKAGES)
    # Use system packages
    find_package(OpenCV REQUIRED)
    find_package(SDL2 REQUIRED)
    set(OPENCV_LIBS ${OpenCV_LIBS})
    set(SDL2_LIBS SDL2::SDL2)
else()
    # Use vcpkg packages
    find_package(OpenCV REQUIRED)
    find_package(SDL2 REQUIRED)
    set(OPENCV_LIBS ${OpenCV_LIBS})
    set(SDL2_LIBS SDL2::SDL2)
endif()

# Source files
set(SOURCES
    main.cpp
    CameraManager.cpp
    ImageProcessor.cpp
    MarkerDetector.cpp
    # CodiceDecoder.cpp
    TUIOBridge.cpp
    TUIOConfig.cpp
    # MainWindow.cpp
)

# Create executable
add_executable(CodiceCam ${SOURCES})

# Link libraries
target_link_libraries(CodiceCam
    ${OPENCV_LIBS}
    ${SDL2_LIBS}
    TUIO
)

# Include directories
target_include_directories(CodiceCam PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/third_party/TUIO11_CPP/TUIO
    ${CMAKE_SOURCE_DIR}/third_party/TUIO11_CPP/oscpack
    ${OpenCV_INCLUDE_DIRS}
)

# Compiler definitions
target_compile_definitions(CodiceCam PRIVATE
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

# Set output directory
set_target_properties(CodiceCam PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
