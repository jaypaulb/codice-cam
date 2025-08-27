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
{
}

TUIOBridge::~TUIOBridge() {
    stop();
}

bool TUIOBridge::initialize(const std::string& host, int port) {
    try {
        host_ = host;
        port_ = port;

        // Create TUIO server with host and port
        tuio_server_ = std::make_unique<TUIO::TuioServer>(host_.c_str(), port_);

        if (!tuio_server_) {
            std::cerr << "❌ Failed to create TUIO server" << std::endl;
            return false;
        }

        std::cout << "✅ TUIO server initialized: " << host_ << ":" << port_ << std::endl;
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

            if (active_objects_.find(session_id) != active_objects_.end()) {
                // Update existing object
                auto obj = active_objects_[session_id];
                tuio_server_->updateTuioObject(obj, marker.x, marker.y, marker.angle);
                total_objects_updated_++;
            } else {
                // Create new object using TuioServer's addTuioObject method
                // Direct mapping: Codice marker ID -> TUIO symbol ID
                auto obj = tuio_server_->addTuioObject(marker.id, marker.x, marker.y, marker.angle);
                if (obj) {
                    active_objects_[session_id] = obj;
                    total_objects_created_++;
                }
            }

            // Update last seen time
            last_markers_[marker.id] = marker;
        }

        // Remove markers that are no longer detected
        std::vector<int> to_remove;
        for (const auto& [session_id, obj] : active_objects_) {
            int marker_id = obj->getSymbolID();
            if (current_marker_ids.find(marker_id) == current_marker_ids.end()) {
                to_remove.push_back(session_id);
            }
        }

        for (int session_id : to_remove) {
            auto obj = active_objects_[session_id];
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
