# Third-party dependencies

# TUIO11_CPP library - Create CMake interface
set(TUIO_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/TUIO11_CPP)

# Create TUIO library
add_library(TUIO STATIC
    # TUIO core library files
    ${TUIO_SOURCE_DIR}/TUIO/TuioBlob.cpp
    ${TUIO_SOURCE_DIR}/TUIO/TuioClient.cpp
    ${TUIO_SOURCE_DIR}/TUIO/TuioContainer.cpp
    ${TUIO_SOURCE_DIR}/TUIO/TuioCursor.cpp
    ${TUIO_SOURCE_DIR}/TUIO/TuioDispatcher.cpp
    ${TUIO_SOURCE_DIR}/TUIO/TuioManager.cpp
    ${TUIO_SOURCE_DIR}/TUIO/TuioObject.cpp
    ${TUIO_SOURCE_DIR}/TUIO/TuioPoint.cpp
    ${TUIO_SOURCE_DIR}/TUIO/TuioServer.cpp
    ${TUIO_SOURCE_DIR}/TUIO/TuioTime.cpp
    ${TUIO_SOURCE_DIR}/TUIO/OneEuroFilter.cpp

    # OSC communication files
    ${TUIO_SOURCE_DIR}/TUIO/OscReceiver.cpp
    ${TUIO_SOURCE_DIR}/TUIO/TcpReceiver.cpp
    ${TUIO_SOURCE_DIR}/TUIO/TcpSender.cpp
    ${TUIO_SOURCE_DIR}/TUIO/UdpReceiver.cpp
    ${TUIO_SOURCE_DIR}/TUIO/UdpSender.cpp
    ${TUIO_SOURCE_DIR}/TUIO/WebSockSender.cpp
    ${TUIO_SOURCE_DIR}/TUIO/FlashSender.cpp

    # OSC library files
    ${TUIO_SOURCE_DIR}/oscpack/osc/OscOutboundPacketStream.cpp
    ${TUIO_SOURCE_DIR}/oscpack/osc/OscReceivedElements.cpp
    ${TUIO_SOURCE_DIR}/oscpack/osc/OscTypes.cpp
    ${TUIO_SOURCE_DIR}/oscpack/ip/IpEndpointName.cpp
    ${TUIO_SOURCE_DIR}/oscpack/ip/posix/NetworkingUtils.cpp
    ${TUIO_SOURCE_DIR}/oscpack/ip/posix/UdpSocket.cpp
)

# Add Windows-specific files only on Windows
if(WIN32)
    target_sources(TUIO PRIVATE
        ${TUIO_SOURCE_DIR}/oscpack/ip/win32/NetworkingUtils.cpp
        ${TUIO_SOURCE_DIR}/oscpack/ip/win32/UdpSocket.cpp
    )
endif()

# Include directories
target_include_directories(TUIO PUBLIC
    ${TUIO_SOURCE_DIR}/TUIO
    ${TUIO_SOURCE_DIR}/oscpack
)

# Compiler definitions
target_compile_definitions(TUIO PRIVATE
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

# Set C++ standard
target_compile_features(TUIO PUBLIC cxx_std_11)

# Link system libraries
if(WIN32)
    target_link_libraries(TUIO ws2_32 wsock32)
else()
    target_link_libraries(TUIO pthread)
endif()

# Export the library
set_target_properties(TUIO PROPERTIES
    VERSION 1.1.6
    SOVERSION 1
)
