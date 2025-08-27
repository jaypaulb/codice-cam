#include "TUIOBridge.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <set>

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
            int session_id = generateSessionId(marker.id);
            
            if (active_objects_.find(session_id) != active_objects_.end()) {
                // Update existing object
                auto obj = active_objects_[session_id];
                tuio_server_->updateTuioObject(obj, marker.x, marker.y, marker.angle);
                total_objects_updated_++;
            } else {
                // Create new object using TuioServer's addTuioObject method
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

int TUIOBridge::generateSessionId(int marker_id) {
    // Use marker ID as base for session ID, but ensure uniqueness
    // For now, we'll use a simple mapping: session_id = marker_id + 1000
    // This ensures session IDs don't conflict with TUIO's internal IDs
    return marker_id + 1000;
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
