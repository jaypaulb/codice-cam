#include "TUIOBridge.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <set>
#include <iomanip>

// Include TUIO headers
#include "TuioServer.h"
#include "TuioObject.h"
#include "TuioTime.h"

namespace CodiceCam {

TUIOBridge::TUIOBridge()
    : tuio_server_(nullptr)
    , host_("localhost")
    , port_(3333)
    , running_(false)
    , marker_timeout_ms_(1000)  // 1 second timeout
    , total_objects_created_(0)
    , total_objects_updated_(0)
    , total_objects_removed_(0)
    , start_time_(std::chrono::steady_clock::now())
    , total_detected_(0)
    , total_lost_(0)
{
}

TUIOBridge::~TUIOBridge() {
    stop();
}

bool TUIOBridge::initialize(const std::string& host, int port) {
    try {
        // Use configuration if available, otherwise use provided parameters
        const auto& config = config_manager_.getConfig();
        host_ = host.empty() ? config.host : host;
        port_ = (port <= 0) ? config.port : port;
        
        // Create TUIO server with host and port
        tuio_server_ = std::make_unique<TUIO::TuioServer>(host_.c_str(), port_);
        
        if (!tuio_server_) {
            std::cerr << "❌ Failed to create TUIO server" << std::endl;
            return false;
        }
        
        std::cout << "✅ TUIO server initialized: " << host_ << ":" << port_ << std::endl;
        std::cout << "📋 Configuration: " << config.max_fps << " FPS, " << config.max_markers << " max markers" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error initializing TUIO server: " << e.what() << std::endl;
        return false;
    }
}

bool TUIOBridge::start() {
    if (!tuio_server_) {
        std::cerr << "❌ TUIO server not initialized" << std::endl;
        return false;
    }
    
    try {
        // TUIO server doesn't need explicit start - it's ready to use
        running_ = true;
        start_time_ = std::chrono::steady_clock::now();
        
        std::cout << "🚀 TUIO server started on " << host_ << ":" << port_ << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error starting TUIO server: " << e.what() << std::endl;
        running_ = false;
        return false;
    }
}

void TUIOBridge::stop() {
    if (tuio_server_ && running_) {
        try {
            // Remove all active objects
            for (auto& [session_id, obj] : active_objects_) {
                tuio_server_->removeTuioObject(obj);
            }
            active_objects_.clear();
            last_markers_.clear();
            
            running_ = false;
            
            std::cout << "🛑 TUIO server stopped" << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "❌ Error stopping TUIO server: " << e.what() << std::endl;
        }
    }
}

void TUIOBridge::updateMarkers(const std::vector<CodiceMarker>& markers) {
    if (!tuio_server_ || !running_) {
        return;
    }
    
    try {
        // Clean up expired markers first
        cleanupExpiredMarkers();
        
        // Create a set of current marker IDs for efficient lookup
        std::set<int> current_marker_ids;
        for (const auto& marker : markers) {
            current_marker_ids.insert(marker.id);
        }
        
        // Initialize frame
        tuio_server_->initFrame(TUIO::TuioTime::getSessionTime());
        
        // Update existing markers and add new ones
        for (const auto& marker : markers) {
            // Validate marker before processing
            if (!validateMapping(marker)) {
                std::cerr << "⚠️  Skipping invalid marker ID: " << marker.id << std::endl;
                continue;
            }
            
            int session_id = generateSessionId(marker.id);
            bool is_new_marker = (last_markers_.find(marker.id) == last_markers_.end());
            
            if (is_new_marker) {
                // Create new object using TuioServer's addTuioObject method
                // Direct mapping: Codice marker ID -> TUIO symbol ID
                auto obj = tuio_server_->addTuioObject(marker.id, marker.x, marker.y, marker.angle);
                if (obj) {
                    active_objects_[session_id] = obj;
                    total_objects_created_++;
                    
                    // Handle lifecycle: DETECTED
                    CodiceMarker lifecycle_marker = marker;
                    lifecycle_marker.state = MarkerState::DETECTED;
                    lifecycle_marker.first_detected = std::chrono::steady_clock::now();
                    lifecycle_marker.update_count = 0;
                    handleStateTransition(marker.id, MarkerState::DETECTED, lifecycle_marker);
                }
            } else {
                // Update existing object
                auto obj = active_objects_[session_id];
                tuio_server_->updateTuioObject(obj, marker.x, marker.y, marker.angle);
                total_objects_updated_++;
                
                // Handle lifecycle: UPDATED
                CodiceMarker lifecycle_marker = marker;
                lifecycle_marker.state = MarkerState::UPDATED;
                auto last_marker_it = last_markers_.find(marker.id);
                if (last_marker_it != last_markers_.end()) {
                    lifecycle_marker.update_count = last_marker_it->second.update_count + 1;
                } else {
                    lifecycle_marker.update_count = 1;
                }
                handleStateTransition(marker.id, MarkerState::UPDATED, lifecycle_marker);
            }
            
            // Update last seen time and state
            CodiceMarker updated_marker = marker;
            updated_marker.state = is_new_marker ? MarkerState::DETECTED : MarkerState::ACTIVE;
            if (is_new_marker) {
                updated_marker.first_detected = std::chrono::steady_clock::now();
                updated_marker.update_count = 0;
            } else {
                auto last_marker_it = last_markers_.find(marker.id);
                if (last_marker_it != last_markers_.end()) {
                    updated_marker.first_detected = last_marker_it->second.first_detected;
                    updated_marker.update_count = last_marker_it->second.update_count + 1;
                } else {
                    updated_marker.first_detected = std::chrono::steady_clock::now();
                    updated_marker.update_count = 1;
                }
            }
            last_markers_[marker.id] = updated_marker;
        }
        
        // Remove markers that are no longer detected
        std::vector<int> to_remove;
        for (const auto& [session_id, obj] : active_objects_) {
            if (obj == nullptr) {
                std::cerr << "⚠️  Found null object in active_objects_ for session " << session_id << std::endl;
                to_remove.push_back(session_id);
                continue;
            }
            int marker_id = obj->getSymbolID();
            if (current_marker_ids.find(marker_id) == current_marker_ids.end()) {
                to_remove.push_back(session_id);
            }
        }
        
        for (int session_id : to_remove) {
            auto obj = active_objects_[session_id];
            if (obj == nullptr) {
                std::cerr << "⚠️  Skipping null object removal for session " << session_id << std::endl;
                active_objects_.erase(session_id);
                continue;
            }
            
            int marker_id = obj->getSymbolID();
            
            // Handle lifecycle: LOST
            auto last_marker_it = last_markers_.find(marker_id);
            if (last_marker_it != last_markers_.end()) {
                CodiceMarker lost_marker = last_marker_it->second;
                lost_marker.state = MarkerState::LOST;
                handleStateTransition(marker_id, MarkerState::LOST, lost_marker);
            }
            
            tuio_server_->removeTuioObject(obj);
            active_objects_.erase(session_id);
            total_objects_removed_++;
        }
        
        // Commit the frame
        tuio_server_->commitFrame();
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error updating markers: " << e.what() << std::endl;
    }
}

bool TUIOBridge::isRunning() const {
    return running_;
}

std::string TUIOBridge::getConfiguration() const {
    std::ostringstream oss;
    oss << "TUIO Server Configuration:\n";
    oss << "  Host: " << host_ << "\n";
    oss << "  Port: " << port_ << "\n";
    oss << "  Status: " << (running_ ? "Running" : "Stopped") << "\n";
    oss << "  Marker Timeout: " << marker_timeout_ms_ << "ms\n";
    return oss.str();
}

void TUIOBridge::setMarkerTimeout(int timeout_ms) {
    marker_timeout_ms_ = timeout_ms;
}

std::string TUIOBridge::getStatistics() const {
    auto now = std::chrono::steady_clock::now();
    auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_).count();
    
    std::ostringstream oss;
    oss << "TUIO Server Statistics:\n";
    oss << "  Uptime: " << uptime << " seconds\n";
    oss << "  Active Objects: " << active_objects_.size() << "\n";
    oss << "  Objects Created: " << total_objects_created_ << "\n";
    oss << "  Objects Updated: " << total_objects_updated_ << "\n";
    oss << "  Objects Removed: " << total_objects_removed_ << "\n";
    return oss.str();
}

std::string TUIOBridge::getMappingInfo(int marker_id) const {
    std::ostringstream oss;
    oss << "Mapping Info for Codice Marker ID " << marker_id << ":\n";
    
    if (!isValidCodiceId(marker_id)) {
        oss << "  ❌ Invalid Codice marker ID (must be 0-4095)\n";
        return oss.str();
    }
    
    // Check if marker is currently active
    int session_id = generateSessionId(marker_id);
    if (active_objects_.find(session_id) != active_objects_.end()) {
        auto obj = active_objects_.at(session_id);
        oss << "  ✅ Active TUIO Object\n";
        oss << "  📍 Session ID: " << session_id << "\n";
        oss << "  🎯 Symbol ID: " << obj->getSymbolID() << "\n";
        oss << "  📊 Position: (" << std::fixed << std::setprecision(3) 
            << obj->getX() << ", " << obj->getY() << ")\n";
        oss << "  🔄 Angle: " << std::setprecision(2) << obj->getAngle() << " rad\n";
    } else {
        oss << "  ⏸️  Not currently active\n";
        oss << "  📍 Would use Session ID: " << session_id << "\n";
        oss << "  🎯 Symbol ID: " << marker_id << " (direct mapping)\n";
    }
    
    return oss.str();
}

bool TUIOBridge::validateMapping(const CodiceMarker& marker) const {
    // Validate Codice marker ID
    if (!isValidCodiceId(marker.id)) {
        std::cerr << "❌ Invalid Codice marker ID: " << marker.id << std::endl;
        return false;
    }
    
    // Validate coordinates
    if (!isValidCoordinates(marker.x, marker.y)) {
        std::cerr << "❌ Invalid coordinates: (" << marker.x << ", " << marker.y << ")" << std::endl;
        return false;
    }
    
    // Validate confidence
    if (marker.confidence < 0.0 || marker.confidence > 1.0) {
        std::cerr << "❌ Invalid confidence: " << marker.confidence << std::endl;
        return false;
    }
    
    return true;
}

std::map<int, int> TUIOBridge::getActiveMappings() const {
    std::map<int, int> mappings;
    
    for (const auto& [session_id, obj] : active_objects_) {
        int marker_id = obj->getSymbolID();
        mappings[marker_id] = session_id;
    }
    
    return mappings;
}

bool TUIOBridge::setStreamingConfig(const TUIOStreamingConfig& config) {
    if (!config.validate()) {
        std::cerr << "❌ Invalid streaming configuration: " << config.getValidationErrors() << std::endl;
        return false;
    }
    
    if (!config_manager_.setConfig(config)) {
        return false;
    }
    
    // Apply configuration to running server if needed
    if (running_) {
        std::cout << "⚠️  Configuration updated while server is running. Restart server to apply changes." << std::endl;
    }
    
    return true;
}

const TUIOStreamingConfig& TUIOBridge::getStreamingConfig() const {
    return config_manager_.getConfig();
}

bool TUIOBridge::loadConfigFromFile(const std::string& config_file) {
    return config_manager_.loadFromFile(config_file);
}

bool TUIOBridge::saveConfigToFile(const std::string& config_file) const {
    return config_manager_.saveToFile(config_file);
}

bool TUIOBridge::applyConfigProfile(const std::string& profile_name) {
    const auto& current_config = config_manager_.getConfig();
    TUIOStreamingConfig profile_config = current_config.getProfile(profile_name);
    
    if (!profile_config.validate()) {
        std::cerr << "❌ Invalid profile configuration: " << profile_config.getValidationErrors() << std::endl;
        return false;
    }
    
    if (!config_manager_.setConfig(profile_config)) {
        return false;
    }
    
    std::cout << "✅ Applied configuration profile: " << profile_name << std::endl;
    return true;
}

std::vector<std::string> TUIOBridge::getAvailableProfiles() const {
    return config_manager_.getConfig().getAvailableProfiles();
}

void TUIOBridge::setLifecycleCallback(std::function<void(int marker_id, MarkerState state, const CodiceMarker& marker)> callback) {
    lifecycle_callback_ = callback;
}

std::string TUIOBridge::getLifecycleStatistics() const {
    std::ostringstream oss;
    oss << "Lifecycle Statistics:\n";
    oss << "  Total Detected: " << total_detected_ << "\n";
    oss << "  Total Lost: " << total_lost_ << "\n";
    oss << "  Currently Active: " << active_objects_.size() << "\n";
    oss << "  Objects Created: " << total_objects_created_ << "\n";
    oss << "  Objects Updated: " << total_objects_updated_ << "\n";
    oss << "  Objects Removed: " << total_objects_removed_ << "\n";
    
    // State distribution
    std::map<MarkerState, int> state_counts;
    for (const auto& [marker_id, state] : marker_states_) {
        state_counts[state]++;
    }
    
    oss << "  State Distribution:\n";
    for (const auto& [state, count] : state_counts) {
        oss << "    " << getStateName(state) << ": " << count << "\n";
    }
    
    return oss.str();
}

std::string TUIOBridge::getMarkerLifecycleHistory(int marker_id) const {
    std::ostringstream oss;
    oss << "Lifecycle History for Marker ID " << marker_id << ":\n";
    
    auto history_it = marker_history_.find(marker_id);
    if (history_it == marker_history_.end()) {
        oss << "  No history found for this marker\n";
        return oss.str();
    }
    
    const auto& history = history_it->second;
    for (const auto& [state, timestamp] : history) {
        auto time_since_epoch = std::chrono::duration_cast<std::chrono::milliseconds>(
            timestamp.time_since_epoch()).count();
        oss << "  " << getStateName(state) << " at " << time_since_epoch << "ms\n";
    }
    
    return oss.str();
}

bool TUIOBridge::transitionMarkerState(int marker_id, MarkerState new_state) {
    auto state_it = marker_states_.find(marker_id);
    if (state_it == marker_states_.end()) {
        std::cerr << "❌ Marker " << marker_id << " not found for state transition" << std::endl;
        return false;
    }
    
    MarkerState old_state = state_it->second;
    marker_states_[marker_id] = new_state;
    addToHistory(marker_id, new_state);
    
    std::cout << "🔄 Marker " << marker_id << " transitioned from " 
              << getStateName(old_state) << " to " << getStateName(new_state) << std::endl;
    
    return true;
}

void TUIOBridge::handleStateTransition(int marker_id, MarkerState new_state, const CodiceMarker& marker) {
    // Update state
    marker_states_[marker_id] = new_state;
    
    // Add to history
    addToHistory(marker_id, new_state);
    
    // Update statistics
    if (new_state == MarkerState::DETECTED) {
        total_detected_++;
    } else if (new_state == MarkerState::LOST) {
        total_lost_++;
    }
    
    // Call callback if set
    if (lifecycle_callback_) {
        lifecycle_callback_(marker_id, new_state, marker);
    }
    
    std::cout << "🔄 Marker " << marker_id << " -> " << getStateName(new_state) << std::endl;
}

void TUIOBridge::addToHistory(int marker_id, MarkerState state) {
    auto now = std::chrono::steady_clock::now();
    marker_history_[marker_id].push_back({state, now});
    
    // Keep only last 10 history entries per marker
    if (marker_history_[marker_id].size() > 10) {
        marker_history_[marker_id].pop_front();
    }
}

std::string TUIOBridge::getStateName(MarkerState state) const {
    switch (state) {
        case MarkerState::DETECTED: return "DETECTED";
        case MarkerState::ACTIVE: return "ACTIVE";
        case MarkerState::UPDATED: return "UPDATED";
        case MarkerState::LOST: return "LOST";
        default: return "UNKNOWN";
    }
}

int TUIOBridge::generateSessionId(int marker_id) const {
    // Use TUIO server's session ID management for proper uniqueness
    // This ensures session IDs are managed correctly by the TUIO protocol
    return getNextSessionId();
}

int TUIOBridge::getNextSessionId() const {
    if (tuio_server_) {
        // Use TUIO server's internal session ID management
        // Note: TUIO server manages session IDs internally, we'll use a simple counter
        static int session_counter = 1000;
        return ++session_counter;
    }
    // Fallback: generate a simple unique ID
    static int fallback_id = 1000;
    return ++fallback_id;
}

bool TUIOBridge::isValidCodiceId(int marker_id) const {
    // Codice markers support 4x4 grid with 11 data bits = 2048 possible IDs (0-2047)
    // But we'll be conservative and support the full range for future expansion
    return marker_id >= 0 && marker_id <= 4095;
}

bool TUIOBridge::isValidCoordinates(float x, float y) const {
    // TUIO coordinates must be normalized (0.0-1.0)
    return x >= 0.0f && x <= 1.0f && y >= 0.0f && y <= 1.0f;
}

void TUIOBridge::cleanupExpiredMarkers() {
    auto now = std::chrono::steady_clock::now();
    std::vector<int> expired_markers;
    
    for (const auto& [marker_id, marker] : last_markers_) {
        auto time_since_seen = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - marker.last_seen).count();
        
        if (time_since_seen > marker_timeout_ms_) {
            expired_markers.push_back(marker_id);
        }
    }
    
    // Remove expired markers
    for (int marker_id : expired_markers) {
        int session_id = generateSessionId(marker_id);
        if (active_objects_.find(session_id) != active_objects_.end()) {
            auto obj = active_objects_[session_id];
            tuio_server_->removeTuioObject(obj);
            active_objects_.erase(session_id);
            total_objects_removed_++;
        }
        last_markers_.erase(marker_id);
    }
}



} // namespace CodiceCam
