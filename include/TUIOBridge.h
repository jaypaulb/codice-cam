#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <functional>
#include <list>

// Include TUIO headers
#include "TuioServer.h"
#include "TuioObject.h"
#include "TuioTime.h"

namespace CodiceCam {

/**
 * @brief Represents the lifecycle state of a Codice marker
 */
enum class MarkerState {
    DETECTED,    // Marker was just detected
    ACTIVE,      // Marker is active and being tracked
    UPDATED,     // Marker position/angle was updated
    LOST         // Marker was lost/removed
};

/**
 * @brief Represents a detected Codice marker with lifecycle information
 */
struct CodiceMarker {
    int id;                    // Codice marker ID (0-4095)
    double confidence;         // Detection confidence (0.0-1.0)
    float x, y;               // Position in normalized coordinates (0.0-1.0)
    float angle;              // Rotation angle in radians
    std::chrono::steady_clock::time_point last_seen;  // Last detection time
    int session_id;           // TUIO session ID
    MarkerState state;        // Current lifecycle state
    std::chrono::steady_clock::time_point first_detected;  // First detection time
    int update_count;         // Number of updates since detection
};

/**
 * @brief TUIO Bridge for streaming Codice marker data
 * 
 * This class manages the TUIO server and maps Codice markers to TUIO objects,
 * providing real-time streaming of marker data to MT Showcase software.
 * 
 * ## Mapping Strategy
 * - Codice marker IDs (0-4095) map directly to TUIO symbol IDs
 * - Session IDs are generated using TUIO's internal session management
 * - Object lifecycle: ADD -> UPDATE -> REMOVE
 * - Position coordinates are normalized (0.0-1.0) for TUIO compatibility
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
    
    /**
     * @brief Get mapping information for a specific marker
     * @param marker_id Codice marker ID
     * @return Mapping information string
     */
    std::string getMappingInfo(int marker_id) const;
    
    /**
     * @brief Validate marker mapping
     * @param marker Codice marker to validate
     * @return true if mapping is valid, false otherwise
     */
    bool validateMapping(const CodiceMarker& marker) const;
    
    /**
     * @brief Get all active mappings
     * @return Map of marker ID to session ID
     */
    std::map<int, int> getActiveMappings() const;
    
    /**
     * @brief Set callback for marker lifecycle events
     * @param callback Function to call on lifecycle events
     */
    void setLifecycleCallback(std::function<void(int marker_id, MarkerState state, const CodiceMarker& marker)> callback);
    
    /**
     * @brief Get lifecycle statistics
     * @return Lifecycle statistics string
     */
    std::string getLifecycleStatistics() const;
    
    /**
     * @brief Get marker lifecycle history
     * @param marker_id Codice marker ID
     * @return Lifecycle history string
     */
    std::string getMarkerLifecycleHistory(int marker_id) const;
    
    /**
     * @brief Force marker state transition
     * @param marker_id Codice marker ID
     * @param new_state New state to transition to
     * @return true if transition successful, false otherwise
     */
    bool transitionMarkerState(int marker_id, MarkerState new_state);

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
    
    // Lifecycle management
    std::function<void(int marker_id, MarkerState state, const CodiceMarker& marker)> lifecycle_callback_;
    std::map<int, std::list<std::pair<MarkerState, std::chrono::steady_clock::time_point>>> marker_history_;
    std::map<int, MarkerState> marker_states_;
    int total_detected_;
    int total_lost_;
    
    /**
     * @brief Generate unique session ID for a marker
     * @param marker_id Codice marker ID
     * @return Session ID
     */
    int generateSessionId(int marker_id) const;
    
    /**
     * @brief Get next available session ID from TUIO server
     * @return Next available session ID
     */
    int getNextSessionId() const;
    
    /**
     * @brief Validate Codice marker ID range
     * @param marker_id Codice marker ID to validate
     * @return true if valid, false otherwise
     */
    bool isValidCodiceId(int marker_id) const;
    
    /**
     * @brief Validate TUIO coordinates
     * @param x X coordinate
     * @param y Y coordinate
     * @return true if valid, false otherwise
     */
    bool isValidCoordinates(float x, float y) const;
    
    /**
     * @brief Clean up expired markers
     */
    void cleanupExpiredMarkers();
    
    /**
     * @brief Handle marker state transition
     * @param marker_id Codice marker ID
     * @param new_state New state
     * @param marker Marker data
     */
    void handleStateTransition(int marker_id, MarkerState new_state, const CodiceMarker& marker);
    
    /**
     * @brief Add marker to history
     * @param marker_id Codice marker ID
     * @param state Marker state
     */
    void addToHistory(int marker_id, MarkerState state);
    
    /**
     * @brief Get state name as string
     * @param state Marker state
     * @return State name string
     */
    std::string getStateName(MarkerState state) const;
    

};

} // namespace CodiceCam
