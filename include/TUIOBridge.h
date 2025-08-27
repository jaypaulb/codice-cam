#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <chrono>

// Include TUIO headers
#include "TuioServer.h"
#include "TuioObject.h"
#include "TuioTime.h"

namespace CodiceCam {

/**
 * @brief Represents a detected Codice marker
 */
struct CodiceMarker {
    int id;                    // Codice marker ID (0-4095)
    double confidence;         // Detection confidence (0.0-1.0)
    float x, y;               // Position in normalized coordinates (0.0-1.0)
    float angle;              // Rotation angle in radians
    std::chrono::steady_clock::time_point last_seen;  // Last detection time
    int session_id;           // TUIO session ID
};

/**
 * @brief TUIO Bridge for streaming Codice marker data
 *
 * This class manages the TUIO server and maps Codice markers to TUIO objects,
 * providing real-time streaming of marker data to MT Showcase software.
 */
class TUIOBridge {
public:
    /**
     * @brief Constructor
     */
    TUIOBridge();

    /**
     * @brief Destructor
     */
    ~TUIOBridge();

    /**
     * @brief Initialize the TUIO server
     * @param host Host address (default: "localhost")
     * @param port Port number (default: 3333)
     * @return true if initialization successful, false otherwise
     */
    bool initialize(const std::string& host = "localhost", int port = 3333);

    /**
     * @brief Start the TUIO server
     * @return true if started successfully, false otherwise
     */
    bool start();

    /**
     * @brief Stop the TUIO server
     */
    void stop();

    /**
     * @brief Update markers and send TUIO messages
     * @param markers Vector of currently detected markers
     */
    void updateMarkers(const std::vector<CodiceMarker>& markers);

    /**
     * @brief Check if the server is running
     * @return true if running, false otherwise
     */
    bool isRunning() const;

    /**
     * @brief Get server configuration
     * @return Configuration string
     */
    std::string getConfiguration() const;

    /**
     * @brief Set marker timeout (in milliseconds)
     * @param timeout_ms Timeout in milliseconds
     */
    void setMarkerTimeout(int timeout_ms);

    /**
     * @brief Get statistics
     * @return Statistics string
     */
    std::string getStatistics() const;

private:
    std::unique_ptr<TUIO::TuioServer> tuio_server_;
    std::map<int, TUIO::TuioObject*> active_objects_;
    std::map<int, CodiceMarker> last_markers_;

    std::string host_;
    int port_;
    bool running_;
    int marker_timeout_ms_;

    // Statistics
    int total_objects_created_;
    int total_objects_updated_;
    int total_objects_removed_;
    std::chrono::steady_clock::time_point start_time_;

    /**
     * @brief Generate unique session ID for a marker
     * @param marker_id Codice marker ID
     * @return Session ID
     */
    int generateSessionId(int marker_id);

    /**
     * @brief Clean up expired markers
     */
    void cleanupExpiredMarkers();


};

} // namespace CodiceCam
