#include "TUIOConfig.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <regex>

namespace CodiceCam {

// TUIOStreamingConfig implementation

bool TUIOStreamingConfig::validate() const {
    return getValidationErrors().empty();
}

std::string TUIOStreamingConfig::getValidationErrors() const {
    std::vector<std::string> errors;

    // Network validation
    if (host.empty()) {
        errors.push_back("Host cannot be empty");
    }
    if (port < 1 || port > 65535) {
        errors.push_back("Port must be between 1 and 65535");
    }
    if (timeout_ms < 100) {
        errors.push_back("Timeout must be at least 100ms");
    }

    // Streaming validation
    if (max_fps < 1 || max_fps > 120) {
        errors.push_back("Max FPS must be between 1 and 120");
    }
    if (buffer_size < 256) {
        errors.push_back("Buffer size must be at least 256 bytes");
    }

    // Marker validation
    if (marker_timeout_ms < 100) {
        errors.push_back("Marker timeout must be at least 100ms");
    }
    if (min_confidence < 0.0 || min_confidence > 1.0) {
        errors.push_back("Min confidence must be between 0.0 and 1.0");
    }
    if (max_markers < 1 || max_markers > 100) {
        errors.push_back("Max markers must be between 1 and 100");
    }

    // Performance validation
    if (motion_smoothing_factor < 0.0 || motion_smoothing_factor > 1.0) {
        errors.push_back("Motion smoothing factor must be between 0.0 and 1.0");
    }
    if (prediction_frames < 0 || prediction_frames > 10) {
        errors.push_back("Prediction frames must be between 0 and 10");
    }

    std::ostringstream oss;
    for (size_t i = 0; i < errors.size(); ++i) {
        if (i > 0) oss << "; ";
        oss << errors[i];
    }
    return oss.str();
}

void TUIOStreamingConfig::setDefaults() {
    host = "localhost";
    port = 3333;
    timeout_ms = 1000;
    max_fps = 30;
    enable_compression = false;
    buffer_size = 1024;
    marker_timeout_ms = 1000;
    min_confidence = 0.5;
    max_markers = 10;
    enable_tuio_1_1 = true;
    enable_tuio_2_0 = false;
    tuio_profile = "default";
    enable_debug_logging = false;
    enable_statistics = true;
    statistics_interval_ms = 5000;
    enable_motion_prediction = false;
    motion_smoothing_factor = 0.1;
    prediction_frames = 3;
}

std::string TUIOStreamingConfig::toJson() const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"network\": {\n";
    oss << "    \"host\": \"" << host << "\",\n";
    oss << "    \"port\": " << port << ",\n";
    oss << "    \"timeout_ms\": " << timeout_ms << "\n";
    oss << "  },\n";
    oss << "  \"streaming\": {\n";
    oss << "    \"max_fps\": " << max_fps << ",\n";
    oss << "    \"enable_compression\": " << (enable_compression ? "true" : "false") << ",\n";
    oss << "    \"buffer_size\": " << buffer_size << "\n";
    oss << "  },\n";
    oss << "  \"markers\": {\n";
    oss << "    \"timeout_ms\": " << marker_timeout_ms << ",\n";
    oss << "    \"min_confidence\": " << min_confidence << ",\n";
    oss << "    \"max_markers\": " << max_markers << "\n";
    oss << "  },\n";
    oss << "  \"tuio\": {\n";
    oss << "    \"enable_tuio_1_1\": " << (enable_tuio_1_1 ? "true" : "false") << ",\n";
    oss << "    \"enable_tuio_2_0\": " << (enable_tuio_2_0 ? "true" : "false") << ",\n";
    oss << "    \"profile\": \"" << tuio_profile << "\"\n";
    oss << "  },\n";
    oss << "  \"debug\": {\n";
    oss << "    \"enable_debug_logging\": " << (enable_debug_logging ? "true" : "false") << ",\n";
    oss << "    \"enable_statistics\": " << (enable_statistics ? "true" : "false") << ",\n";
    oss << "    \"statistics_interval_ms\": " << statistics_interval_ms << "\n";
    oss << "  },\n";
    oss << "  \"performance\": {\n";
    oss << "    \"enable_motion_prediction\": " << (enable_motion_prediction ? "true" : "false") << ",\n";
    oss << "    \"motion_smoothing_factor\": " << motion_smoothing_factor << ",\n";
    oss << "    \"prediction_frames\": " << prediction_frames << "\n";
    oss << "  }\n";
    oss << "}";
    return oss.str();
}

bool TUIOStreamingConfig::fromJson(const std::string& json) {
    // Simple JSON parsing - in a real implementation, you'd use a proper JSON library
    // For now, we'll implement basic key-value extraction
    std::regex host_regex("\"host\":\\s*\"([^\"]+)\"");
    std::regex port_regex("\"port\":\\s*(\\d+)");
    std::regex timeout_regex("\"timeout_ms\":\\s*(\\d+)");
    std::regex max_fps_regex("\"max_fps\":\\s*(\\d+)");
    std::regex buffer_size_regex("\"buffer_size\":\\s*(\\d+)");
    std::regex marker_timeout_regex("\"marker_timeout_ms\":\\s*(\\d+)");
    std::regex min_confidence_regex("\"min_confidence\":\\s*([\\d.]+)");
    std::regex max_markers_regex("\"max_markers\":\\s*(\\d+)");

    std::smatch match;

    if (std::regex_search(json, match, host_regex)) {
        host = match[1].str();
    }
    if (std::regex_search(json, match, port_regex)) {
        port = std::stoi(match[1].str());
    }
    if (std::regex_search(json, match, timeout_regex)) {
        timeout_ms = std::stoi(match[1].str());
    }
    if (std::regex_search(json, match, max_fps_regex)) {
        max_fps = std::stoi(match[1].str());
    }
    if (std::regex_search(json, match, buffer_size_regex)) {
        buffer_size = std::stoi(match[1].str());
    }
    if (std::regex_search(json, match, marker_timeout_regex)) {
        marker_timeout_ms = std::stoi(match[1].str());
    }
    if (std::regex_search(json, match, min_confidence_regex)) {
        min_confidence = std::stod(match[1].str());
    }
    if (std::regex_search(json, match, max_markers_regex)) {
        max_markers = std::stoi(match[1].str());
    }

    return validate();
}

void TUIOStreamingConfig::merge(const TUIOStreamingConfig& other) {
    if (!other.host.empty()) host = other.host;
    if (other.port > 0) port = other.port;
    if (other.timeout_ms > 0) timeout_ms = other.timeout_ms;
    if (other.max_fps > 0) max_fps = other.max_fps;
    if (other.buffer_size > 0) buffer_size = other.buffer_size;
    if (other.marker_timeout_ms > 0) marker_timeout_ms = other.marker_timeout_ms;
    if (other.min_confidence >= 0.0) min_confidence = other.min_confidence;
    if (other.max_markers > 0) max_markers = other.max_markers;
    enable_compression = other.enable_compression;
    enable_tuio_1_1 = other.enable_tuio_1_1;
    enable_tuio_2_0 = other.enable_tuio_2_0;
    if (!other.tuio_profile.empty()) tuio_profile = other.tuio_profile;
    enable_debug_logging = other.enable_debug_logging;
    enable_statistics = other.enable_statistics;
    if (other.statistics_interval_ms > 0) statistics_interval_ms = other.statistics_interval_ms;
    enable_motion_prediction = other.enable_motion_prediction;
    if (other.motion_smoothing_factor >= 0.0) motion_smoothing_factor = other.motion_smoothing_factor;
    if (other.prediction_frames >= 0) prediction_frames = other.prediction_frames;
}

TUIOStreamingConfig TUIOStreamingConfig::getProfile(const std::string& profile_name) const {
    // Return a copy with profile-specific settings
    TUIOStreamingConfig profile = *this;

    if (profile_name == "high_performance") {
        profile.max_fps = 60;
        profile.enable_compression = true;
        profile.enable_motion_prediction = true;
        profile.motion_smoothing_factor = 0.05;
        profile.prediction_frames = 5;
    } else if (profile_name == "low_latency") {
        profile.max_fps = 30;
        profile.enable_compression = false;
        profile.enable_motion_prediction = false;
        profile.timeout_ms = 500;
        profile.marker_timeout_ms = 500;
    } else if (profile_name == "debug") {
        profile.enable_debug_logging = true;
        profile.enable_statistics = true;
        profile.statistics_interval_ms = 1000;
        profile.max_fps = 15;
    }

    return profile;
}

std::vector<std::string> TUIOStreamingConfig::getAvailableProfiles() const {
    return {"default", "high_performance", "low_latency", "debug"};
}

// TUIOConfigManager implementation

TUIOConfigManager::TUIOConfigManager() {
    config_.setDefaults();
    initializeDefaultProfiles();
}

TUIOConfigManager::~TUIOConfigManager() {
}

bool TUIOConfigManager::loadFromFile(const std::string& config_file) {
    std::ifstream file(config_file);
    if (!file.is_open()) {
        std::cerr << "❌ Failed to open configuration file: " << config_file << std::endl;
        return false;
    }

    std::string json_content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
    file.close();

    TUIOStreamingConfig new_config;
    if (!new_config.fromJson(json_content)) {
        std::cerr << "❌ Invalid configuration file: " << new_config.getValidationErrors() << std::endl;
        return false;
    }

    config_ = new_config;
    std::cout << "✅ Configuration loaded from: " << config_file << std::endl;
    return true;
}

bool TUIOConfigManager::saveToFile(const std::string& config_file) const {
    std::ofstream file(config_file);
    if (!file.is_open()) {
        std::cerr << "❌ Failed to create configuration file: " << config_file << std::endl;
        return false;
    }

    file << config_.toJson();
    file.close();

    std::cout << "✅ Configuration saved to: " << config_file << std::endl;
    return true;
}

const TUIOStreamingConfig& TUIOConfigManager::getConfig() const {
    return config_;
}

bool TUIOConfigManager::setConfig(const TUIOStreamingConfig& config) {
    if (!config.validate()) {
        std::cerr << "❌ Invalid configuration: " << config.getValidationErrors() << std::endl;
        return false;
    }

    config_ = config;
    std::cout << "✅ Configuration updated successfully" << std::endl;
    return true;
}

bool TUIOConfigManager::updateParameter(const std::string& key, const std::string& value) {
    return parseJsonValue(key, value);
}

std::string TUIOConfigManager::getParameter(const std::string& key) const {
    return serializeJsonValue(key);
}

void TUIOConfigManager::resetToDefaults() {
    config_.setDefaults();
    std::cout << "✅ Configuration reset to defaults" << std::endl;
}

std::string TUIOConfigManager::getConfigSummary() const {
    std::ostringstream oss;
    oss << "TUIO Streaming Configuration Summary:\n";
    oss << "  Network: " << config_.host << ":" << config_.port << "\n";
    oss << "  Streaming: " << config_.max_fps << " FPS, " << config_.buffer_size << " bytes\n";
    oss << "  Markers: " << config_.max_markers << " max, " << config_.min_confidence << " min confidence\n";
    oss << "  TUIO: v1.1=" << (config_.enable_tuio_1_1 ? "enabled" : "disabled");
    if (config_.enable_tuio_2_0) oss << ", v2.0=enabled";
    oss << "\n";
    oss << "  Performance: " << (config_.enable_motion_prediction ? "prediction enabled" : "prediction disabled") << "\n";
    oss << "  Debug: " << (config_.enable_debug_logging ? "enabled" : "disabled") << "\n";
    return oss.str();
}

bool TUIOConfigManager::validateConfig() const {
    return config_.validate();
}

std::vector<std::string> TUIOConfigManager::getValidationErrors() const {
    std::vector<std::string> errors;
    std::string error_str = config_.getValidationErrors();
    if (!error_str.empty()) {
        std::istringstream iss(error_str);
        std::string error;
        while (std::getline(iss, error, ';')) {
            // Trim whitespace
            error.erase(0, error.find_first_not_of(" \t"));
            error.erase(error.find_last_not_of(" \t") + 1);
            if (!error.empty()) {
                errors.push_back(error);
            }
        }
    }
    return errors;
}

void TUIOConfigManager::initializeDefaultProfiles() {
    // Initialize default profiles
    TUIOStreamingConfig default_profile;
    default_profile.setDefaults();
    profiles_["default"] = default_profile;

    TUIOStreamingConfig high_perf = default_profile.getProfile("high_performance");
    profiles_["high_performance"] = high_perf;

    TUIOStreamingConfig low_latency = default_profile.getProfile("low_latency");
    profiles_["low_latency"] = low_latency;

    TUIOStreamingConfig debug = default_profile.getProfile("debug");
    profiles_["debug"] = debug;
}

bool TUIOConfigManager::parseJsonValue(const std::string& key, const std::string& value) {
    // Simple parameter parsing - in a real implementation, you'd use proper type conversion
    if (key == "host") {
        config_.host = value;
    } else if (key == "port") {
        config_.port = std::stoi(value);
    } else if (key == "timeout_ms") {
        config_.timeout_ms = std::stoi(value);
    } else if (key == "max_fps") {
        config_.max_fps = std::stoi(value);
    } else if (key == "buffer_size") {
        config_.buffer_size = std::stoi(value);
    } else if (key == "marker_timeout_ms") {
        config_.marker_timeout_ms = std::stoi(value);
    } else if (key == "min_confidence") {
        config_.min_confidence = std::stod(value);
    } else if (key == "max_markers") {
        config_.max_markers = std::stoi(value);
    } else if (key == "enable_compression") {
        config_.enable_compression = (value == "true" || value == "1");
    } else if (key == "enable_tuio_1_1") {
        config_.enable_tuio_1_1 = (value == "true" || value == "1");
    } else if (key == "enable_tuio_2_0") {
        config_.enable_tuio_2_0 = (value == "true" || value == "1");
    } else if (key == "enable_debug_logging") {
        config_.enable_debug_logging = (value == "true" || value == "1");
    } else if (key == "enable_statistics") {
        config_.enable_statistics = (value == "true" || value == "1");
    } else if (key == "enable_motion_prediction") {
        config_.enable_motion_prediction = (value == "true" || value == "1");
    } else if (key == "motion_smoothing_factor") {
        config_.motion_smoothing_factor = std::stod(value);
    } else if (key == "prediction_frames") {
        config_.prediction_frames = std::stoi(value);
    } else {
        return false;
    }

    return config_.validate();
}

std::string TUIOConfigManager::serializeJsonValue(const std::string& key) const {
    if (key == "host") return config_.host;
    if (key == "port") return std::to_string(config_.port);
    if (key == "timeout_ms") return std::to_string(config_.timeout_ms);
    if (key == "max_fps") return std::to_string(config_.max_fps);
    if (key == "buffer_size") return std::to_string(config_.buffer_size);
    if (key == "marker_timeout_ms") return std::to_string(config_.marker_timeout_ms);
    if (key == "min_confidence") return std::to_string(config_.min_confidence);
    if (key == "max_markers") return std::to_string(config_.max_markers);
    if (key == "enable_compression") return config_.enable_compression ? "true" : "false";
    if (key == "enable_tuio_1_1") return config_.enable_tuio_1_1 ? "true" : "false";
    if (key == "enable_tuio_2_0") return config_.enable_tuio_2_0 ? "true" : "false";
    if (key == "enable_debug_logging") return config_.enable_debug_logging ? "true" : "false";
    if (key == "enable_statistics") return config_.enable_statistics ? "true" : "false";
    if (key == "enable_motion_prediction") return config_.enable_motion_prediction ? "true" : "false";
    if (key == "motion_smoothing_factor") return std::to_string(config_.motion_smoothing_factor);
    if (key == "prediction_frames") return std::to_string(config_.prediction_frames);
    return "";
}

} // namespace CodiceCam
