#pragma once

#include <string>
#include <map>
#include <vector>
#include <chrono>

namespace CodiceCam {

/**
 * @brief TUIO streaming configuration options
 */
struct TUIOStreamingConfig {
    // Network configuration
    std::string host = "localhost";
    int port = 3333;
    int timeout_ms = 1000;
    
    // Streaming parameters
    int max_fps = 30;
    bool enable_compression = false;
    int buffer_size = 1024;
    
    // Marker configuration
    int marker_timeout_ms = 1000;
    double min_confidence = 0.5;
    int max_markers = 10;
    
    // TUIO protocol settings
    bool enable_tuio_1_1 = true;
    bool enable_tuio_2_0 = false;
    std::string tuio_profile = "default";
    
    // Debug and logging
    bool enable_debug_logging = false;
    bool enable_statistics = true;
    int statistics_interval_ms = 5000;
    
    // Performance tuning
    bool enable_motion_prediction = false;
    double motion_smoothing_factor = 0.1;
    int prediction_frames = 3;
    
    // Validation
    bool validate() const;
    std::string getValidationErrors() const;
    void setDefaults();
    
    // Serialization
    std::string toJson() const;
    bool fromJson(const std::string& json);
    
    // Configuration management
    void merge(const TUIOStreamingConfig& other);
    TUIOStreamingConfig getProfile(const std::string& profile_name) const;
    std::vector<std::string> getAvailableProfiles() const;
};

/**
 * @brief TUIO configuration manager
 */
class TUIOConfigManager {
public:
    TUIOConfigManager();
    ~TUIOConfigManager();
    
    /**
     * @brief Load configuration from file
     * @param config_file Path to configuration file
     * @return true if loaded successfully, false otherwise
     */
    bool loadFromFile(const std::string& config_file);
    
    /**
     * @brief Save configuration to file
     * @param config_file Path to configuration file
     * @return true if saved successfully, false otherwise
     */
    bool saveToFile(const std::string& config_file) const;
    
    /**
     * @brief Get current configuration
     * @return Current TUIO streaming configuration
     */
    const TUIOStreamingConfig& getConfig() const;
    
    /**
     * @brief Set configuration
     * @param config New configuration
     * @return true if configuration is valid, false otherwise
     */
    bool setConfig(const TUIOStreamingConfig& config);
    
    /**
     * @brief Update specific configuration parameter
     * @param key Configuration parameter key
     * @param value Configuration parameter value
     * @return true if update successful, false otherwise
     */
    bool updateParameter(const std::string& key, const std::string& value);
    
    /**
     * @brief Get configuration parameter
     * @param key Configuration parameter key
     * @return Configuration parameter value, empty if not found
     */
    std::string getParameter(const std::string& key) const;
    
    /**
     * @brief Reset to default configuration
     */
    void resetToDefaults();
    
    /**
     * @brief Get configuration summary
     * @return Configuration summary string
     */
    std::string getConfigSummary() const;
    
    /**
     * @brief Validate current configuration
     * @return true if valid, false otherwise
     */
    bool validateConfig() const;
    
    /**
     * @brief Get validation errors
     * @return List of validation errors
     */
    std::vector<std::string> getValidationErrors() const;

private:
    TUIOStreamingConfig config_;
    std::map<std::string, TUIOStreamingConfig> profiles_;
    
    void initializeDefaultProfiles();
    bool parseJsonValue(const std::string& key, const std::string& value);
    std::string serializeJsonValue(const std::string& key) const;
};

} // namespace CodiceCam
