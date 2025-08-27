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
    # TUIOBridge.cpp
    # MainWindow.cpp
)

# Header files
set(HEADERS
    # CameraManager.h
    # MarkerDetector.h
    # CodiceDecoder.h
    # TUIOBridge.h
    # MainWindow.h
)

# Create executable
add_executable(CodiceCam ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(CodiceCam
    ${OPENCV_LIBS}
    ${SDL2_LIBS}
    TUIO
)

# Include directories
target_include_directories(CodiceCam PRIVATE
    ${CMAKE_SOURCE_DIR}/include
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
