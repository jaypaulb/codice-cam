#pragma once

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <functional>

namespace CodiceCam {

/**
 * @brief TUIO message validation result
 */
struct TUIOValidationResult {
    bool is_valid = false;
    std::string error_message;
    std::vector<std::string> warnings;
    std::map<std::string, std::string> message_info;
    
    void addWarning(const std::string& warning) {
        warnings.push_back(warning);
    }
    
    void addInfo(const std::string& key, const std::string& value) {
        message_info[key] = value;
    }
};

/**
 * @brief TUIO message validator for MT Showcase compatibility
 */
class TUIOValidator {
public:
    TUIOValidator();
    ~TUIOValidator();
    
    /**
     * @brief Validate TUIO message format
     * @param message Raw TUIO message data
     * @return Validation result with detailed information
     */
    TUIOValidationResult validateMessage(const std::string& message) const;
    
    /**
     * @brief Validate TUIO object data
     * @param symbol_id TUIO symbol ID
     * @param x X coordinate (0.0-1.0)
     * @param y Y coordinate (0.0-1.0)
     * @param angle Rotation angle in radians
     * @return Validation result
     */
    TUIOValidationResult validateObjectData(int symbol_id, float x, float y, float angle) const;
    
    /**
     * @brief Validate TUIO session data
     * @param session_id TUIO session ID
     * @return Validation result
     */
    TUIOValidationResult validateSessionData(int session_id) const;
    
    /**
     * @brief Check MT Showcase compatibility
     * @param message TUIO message
     * @return Compatibility result
     */
    TUIOValidationResult checkMTShowcaseCompatibility(const std::string& message) const;
    
    /**
     * @brief Get TUIO protocol version from message
     * @param message TUIO message
     * @return Protocol version (1.0, 1.1, 2.0, etc.)
     */
    std::string getProtocolVersion(const std::string& message) const;
    
    /**
     * @brief Parse TUIO message components
     * @param message TUIO message
     * @return Map of message components
     */
    std::map<std::string, std::string> parseMessage(const std::string& message) const;
    
    /**
     * @brief Validate message timing
     * @param message TUIO message
     * @param expected_fps Expected FPS
     * @return Validation result
     */
    TUIOValidationResult validateTiming(const std::string& message, int expected_fps) const;
    
    /**
     * @brief Get validation statistics
     * @return Statistics string
     */
    std::string getValidationStatistics() const;
    
    /**
     * @brief Reset validation statistics
     */
    void resetStatistics();
    
    /**
     * @brief Set validation callbacks
     * @param on_valid Callback for valid messages
     * @param on_invalid Callback for invalid messages
     */
    void setValidationCallbacks(
        std::function<void(const TUIOValidationResult&)> on_valid,
        std::function<void(const TUIOValidationResult&)> on_invalid
    );

private:
    // Validation statistics
    mutable int total_messages_validated_;
    mutable int valid_messages_;
    mutable int invalid_messages_;
    mutable std::chrono::steady_clock::time_point last_validation_time_;
    
    // Callbacks
    std::function<void(const TUIOValidationResult&)> on_valid_callback_;
    std::function<void(const TUIOValidationResult&)> on_invalid_callback_;
    
    // Validation helper methods
    bool isValidOSCMessage(const std::string& message) const;
    bool isValidTUIOAddress(const std::string& address) const;
    bool isValidCoordinate(float coord) const;
    bool isValidAngle(float angle) const;
    bool isValidSymbolID(int symbol_id) const;
    bool isValidSessionID(int session_id) const;
    
    // Message parsing helpers
    std::vector<std::string> splitOSCAddress(const std::string& address) const;
    std::vector<std::string> parseOSCArguments(const std::string& message) const;
    std::string extractOSCAddress(const std::string& message) const;
    
    // MT Showcase specific validation
    bool isMTShowcaseCompatible(const std::string& message) const;
    bool hasRequiredTUIOFields(const std::string& message) const;
    bool hasValidMTShowcaseFormat(const std::string& message) const;
};

/**
 * @brief TUIO integration test framework
 */
class TUIOIntegrationTester {
public:
    TUIOIntegrationTester();
    ~TUIOIntegrationTester();
    
    /**
     * @brief Run comprehensive TUIO integration tests
     * @return Test results summary
     */
    std::string runIntegrationTests();
    
    /**
     * @brief Test TUIO message format compliance
     * @return Test results
     */
    TUIOValidationResult testMessageFormat();
    
    /**
     * @brief Test MT Showcase compatibility
     * @return Test results
     */
    TUIOValidationResult testMTShowcaseCompatibility();
    
    /**
     * @brief Test performance requirements
     * @param target_fps Target FPS
     * @return Performance test results
     */
    TUIOValidationResult testPerformance(int target_fps = 30);
    
    /**
     * @brief Test marker data streaming
     * @return Streaming test results
     */
    TUIOValidationResult testMarkerStreaming();
    
    /**
     * @brief Test configuration integration
     * @return Configuration test results
     */
    TUIOValidationResult testConfigurationIntegration();
    
    /**
     * @brief Generate test report
     * @return Comprehensive test report
     */
    std::string generateTestReport() const;
    
    /**
     * @brief Set test configuration
     * @param config Test configuration parameters
     */
    void setTestConfiguration(const std::map<std::string, std::string>& config);

private:
    TUIOValidator validator_;
    std::map<std::string, std::string> test_config_;
    std::vector<TUIOValidationResult> test_results_;
    
    // Test helper methods
    std::string generateTestTUIOMessage(int symbol_id, float x, float y, float angle) const;
    bool simulateMTShowcaseReception(const std::string& message) const;
    double measureMessageLatency(const std::string& message) const;
    int measureThroughput(int duration_seconds) const;
};

} // namespace CodiceCam
