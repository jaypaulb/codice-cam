# Source files for CodiceCam

# Find required packages
find_package(OpenCV REQUIRED)
find_package(SDL2 REQUIRED)

# Source files (to be added as we develop)
set(SOURCES
    main.cpp
    # CameraManager.cpp
    # MarkerDetector.cpp
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
    ${OpenCV_LIBS}
    SDL2::SDL2
    # TUIO library will be added here
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
