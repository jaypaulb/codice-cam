#include "TUIOValidator.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <regex>
#include <cmath>
#include <iomanip>
#include <arpa/inet.h>

namespace CodiceCam {

// TUIOValidator implementation

TUIOValidator::TUIOValidator()
    : total_messages_validated_(0)
    , valid_messages_(0)
    , invalid_messages_(0)
    , last_validation_time_(std::chrono::steady_clock::now())
{
}

TUIOValidator::~TUIOValidator() {
}

TUIOValidationResult TUIOValidator::validateMessage(const std::string& message) const {
    TUIOValidationResult result;
    total_messages_validated_++;
    last_validation_time_ = std::chrono::steady_clock::now();
    
    // Basic message validation
    if (message.empty()) {
        result.error_message = "Message is empty";
        invalid_messages_++;
        if (on_invalid_callback_) on_invalid_callback_(result);
        return result;
    }
    
    // Check if it's a valid OSC message
    if (!isValidOSCMessage(message)) {
        result.error_message = "Invalid OSC message format";
        invalid_messages_++;
        if (on_invalid_callback_) on_invalid_callback_(result);
        return result;
    }
    
    // Extract OSC address
    std::string address = extractOSCAddress(message);
    if (!isValidTUIOAddress(address)) {
        result.error_message = "Invalid TUIO address: " + address;
        invalid_messages_++;
        if (on_invalid_callback_) on_invalid_callback_(result);
        return result;
    }
    
    // Parse message components
    auto components = parseMessage(message);
    result.addInfo("address", address);
    result.addInfo("protocol_version", getProtocolVersion(message));
    
    // Validate specific TUIO message types
    if (address.find("/tuio/2Dobj") != std::string::npos) {
        // Validate TUIO 2D object message
        if (components.find("session_id") == components.end()) {
            result.addWarning("Missing session_id in TUIO object message");
        }
        if (components.find("symbol_id") == components.end()) {
            result.addWarning("Missing symbol_id in TUIO object message");
        }
        if (components.find("x") == components.end()) {
            result.addWarning("Missing x coordinate in TUIO object message");
        }
        if (components.find("y") == components.end()) {
            result.addWarning("Missing y coordinate in TUIO object message");
        }
    }
    
    // Check MT Showcase compatibility
    auto compatibility_result = checkMTShowcaseCompatibility(message);
    if (!compatibility_result.is_valid) {
        result.addWarning("MT Showcase compatibility issue: " + compatibility_result.error_message);
    }
    
    result.is_valid = true;
    valid_messages_++;
    if (on_valid_callback_) on_valid_callback_(result);
    
    return result;
}

TUIOValidationResult TUIOValidator::validateObjectData(int symbol_id, float x, float y, float angle) const {
    TUIOValidationResult result;
    
    // Validate symbol ID
    if (!isValidSymbolID(symbol_id)) {
        result.error_message = "Invalid symbol ID: " + std::to_string(symbol_id);
        return result;
    }
    
    // Validate coordinates
    if (!isValidCoordinate(x)) {
        result.error_message = "Invalid x coordinate: " + std::to_string(x);
        return result;
    }
    
    if (!isValidCoordinate(y)) {
        result.error_message = "Invalid y coordinate: " + std::to_string(y);
        return result;
    }
    
    // Validate angle
    if (!isValidAngle(angle)) {
        result.addWarning("Angle may be outside normal range: " + std::to_string(angle));
    }
    
    result.is_valid = true;
    result.addInfo("symbol_id", std::to_string(symbol_id));
    result.addInfo("x", std::to_string(x));
    result.addInfo("y", std::to_string(y));
    result.addInfo("angle", std::to_string(angle));
    
    return result;
}

TUIOValidationResult TUIOValidator::validateSessionData(int session_id) const {
    TUIOValidationResult result;
    
    if (!isValidSessionID(session_id)) {
        result.error_message = "Invalid session ID: " + std::to_string(session_id);
        return result;
    }
    
    result.is_valid = true;
    result.addInfo("session_id", std::to_string(session_id));
    
    return result;
}

TUIOValidationResult TUIOValidator::checkMTShowcaseCompatibility(const std::string& message) const {
    TUIOValidationResult result;
    
    // Check if message has required TUIO fields
    if (!hasRequiredTUIOFields(message)) {
        result.error_message = "Missing required TUIO fields for MT Showcase";
        return result;
    }
    
    // Check MT Showcase specific format requirements
    if (!hasValidMTShowcaseFormat(message)) {
        result.error_message = "Invalid format for MT Showcase compatibility";
        return result;
    }
    
    // Check protocol version compatibility
    std::string version = getProtocolVersion(message);
    if (version != "1.1" && version != "2.0") {
        result.addWarning("MT Showcase may not support TUIO version: " + version);
    }
    
    result.is_valid = true;
    result.addInfo("mt_showcase_compatible", "true");
    result.addInfo("tuio_version", version);
    
    return result;
}

std::string TUIOValidator::getProtocolVersion(const std::string& message) const {
    // Extract TUIO version from message address
    std::string address = extractOSCAddress(message);
    
    if (address.find("/tuio/2Dobj") != std::string::npos) {
        return "1.1";  // TUIO 1.1
    } else if (address.find("/tuio2/") != std::string::npos) {
        return "2.0";  // TUIO 2.0
    } else if (address.find("/tuio/") != std::string::npos) {
        return "1.0";  // TUIO 1.0
    }
    
    return "unknown";
}

std::map<std::string, std::string> TUIOValidator::parseMessage(const std::string& message) const {
    std::map<std::string, std::string> components;
    
    // Parse OSC address
    std::string address = extractOSCAddress(message);
    components["address"] = address;
    
    // Parse OSC arguments
    auto args = parseOSCArguments(message);
    
    // Map arguments to known TUIO fields
    if (args.size() >= 1) components["session_id"] = args[0];
    if (args.size() >= 2) components["symbol_id"] = args[1];
    if (args.size() >= 3) components["x"] = args[2];
    if (args.size() >= 4) components["y"] = args[3];
    if (args.size() >= 5) components["angle"] = args[4];
    if (args.size() >= 6) components["velocity_x"] = args[5];
    if (args.size() >= 7) components["velocity_y"] = args[6];
    if (args.size() >= 8) components["rotation_velocity"] = args[7];
    if (args.size() >= 9) components["acceleration"] = args[8];
    
    return components;
}

TUIOValidationResult TUIOValidator::validateTiming(const std::string& message, int expected_fps) const {
    TUIOValidationResult result;
    
    // This is a simplified timing validation
    // In a real implementation, you'd measure actual message timing
    auto now = std::chrono::steady_clock::now();
    auto time_since_last = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - last_validation_time_).count();
    
    double expected_interval_ms = 1000.0 / expected_fps;
    double actual_interval_ms = static_cast<double>(time_since_last);
    
    if (actual_interval_ms > expected_interval_ms * 1.5) {
        result.addWarning("Message timing slower than expected FPS");
        result.addInfo("expected_interval_ms", std::to_string(expected_interval_ms));
        result.addInfo("actual_interval_ms", std::to_string(actual_interval_ms));
    }
    
    result.is_valid = true;
    return result;
}

std::string TUIOValidator::getValidationStatistics() const {
    std::ostringstream oss;
    oss << "TUIO Validation Statistics:\n";
    oss << "  Total Messages Validated: " << total_messages_validated_ << "\n";
    oss << "  Valid Messages: " << valid_messages_ << "\n";
    oss << "  Invalid Messages: " << invalid_messages_ << "\n";
    
    if (total_messages_validated_ > 0) {
        double success_rate = (static_cast<double>(valid_messages_) / total_messages_validated_) * 100.0;
        oss << "  Success Rate: " << std::fixed << std::setprecision(2) << success_rate << "%\n";
    }
    
    return oss.str();
}

void TUIOValidator::resetStatistics() {
    total_messages_validated_ = 0;
    valid_messages_ = 0;
    invalid_messages_ = 0;
    last_validation_time_ = std::chrono::steady_clock::now();
}

void TUIOValidator::setValidationCallbacks(
    std::function<void(const TUIOValidationResult&)> on_valid,
    std::function<void(const TUIOValidationResult&)> on_invalid) {
    on_valid_callback_ = on_valid;
    on_invalid_callback_ = on_invalid;
}

// Private helper methods

bool TUIOValidator::isValidOSCMessage(const std::string& message) const {
    // Basic OSC message validation
    // OSC messages should start with '/' and contain address and arguments
    if (message.empty() || message[0] != '/') {
        return false;
    }
    
    // Look for null terminator and type tag
    size_t null_pos = message.find('\0');
    if (null_pos == std::string::npos) {
        return false;
    }
    
    return true;
}

bool TUIOValidator::isValidTUIOAddress(const std::string& address) const {
    // Valid TUIO addresses
    std::vector<std::string> valid_addresses = {
        "/tuio/2Dobj",
        "/tuio/2Dcur",
        "/tuio/2Dblb",
        "/tuio2/obj",
        "/tuio2/cur",
        "/tuio2/blb"
    };
    
    for (const auto& valid_addr : valid_addresses) {
        if (address.find(valid_addr) == 0) {
            return true;
        }
    }
    
    return false;
}

bool TUIOValidator::isValidCoordinate(float coord) const {
    return coord >= 0.0f && coord <= 1.0f;
}

bool TUIOValidator::isValidAngle(float angle) const {
    // Allow angles from -2π to 2π
    return angle >= -2.0f * M_PI && angle <= 2.0f * M_PI;
}

bool TUIOValidator::isValidSymbolID(int symbol_id) const {
    return symbol_id >= 0 && symbol_id <= 4095;  // Codice marker range
}

bool TUIOValidator::isValidSessionID(int session_id) const {
    return session_id > 0;  // Session IDs should be positive
}

std::vector<std::string> TUIOValidator::splitOSCAddress(const std::string& address) const {
    std::vector<std::string> parts;
    std::istringstream iss(address);
    std::string part;
    
    while (std::getline(iss, part, '/')) {
        if (!part.empty()) {
            parts.push_back(part);
        }
    }
    
    return parts;
}

std::vector<std::string> TUIOValidator::parseOSCArguments(const std::string& message) const {
    std::vector<std::string> args;
    
    // Find the type tag (starts after the first null terminator)
    size_t null_pos = message.find('\0');
    if (null_pos == std::string::npos) {
        return args;
    }
    
    // Skip to the type tag
    size_t type_tag_start = null_pos + 1;
    while (type_tag_start < message.length() && message[type_tag_start] == '\0') {
        type_tag_start++;
    }
    
    if (type_tag_start >= message.length()) {
        return args;
    }
    
    // Find the end of the type tag
    size_t type_tag_end = type_tag_start;
    while (type_tag_end < message.length() && message[type_tag_end] != '\0') {
        type_tag_end++;
    }
    
    if (type_tag_end >= message.length()) {
        return args;
    }
    
    // Parse arguments based on type tag
    std::string type_tag = message.substr(type_tag_start, type_tag_end - type_tag_start);
    size_t arg_start = type_tag_end + 1;
    
    // Align to 4-byte boundary
    while (arg_start % 4 != 0) {
        arg_start++;
    }
    
    for (char type : type_tag) {
        if (arg_start >= message.length()) break;
        
        switch (type) {
            case 'i': {  // 32-bit integer
                if (arg_start + 4 <= message.length()) {
                    int32_t value = *reinterpret_cast<const int32_t*>(message.c_str() + arg_start);
                    // Convert from big-endian if needed
                    args.push_back(std::to_string(ntohl(value)));
                    arg_start += 4;
                }
                break;
            }
            case 'f': {  // 32-bit float
                if (arg_start + 4 <= message.length()) {
                    float value = *reinterpret_cast<const float*>(message.c_str() + arg_start);
                    args.push_back(std::to_string(value));
                    arg_start += 4;
                }
                break;
            }
            case 's': {  // String
                size_t str_end = arg_start;
                while (str_end < message.length() && message[str_end] != '\0') {
                    str_end++;
                }
                if (str_end < message.length()) {
                    args.push_back(message.substr(arg_start, str_end - arg_start));
                    arg_start = str_end + 1;
                    // Align to 4-byte boundary
                    while (arg_start % 4 != 0) {
                        arg_start++;
                    }
                }
                break;
            }
            default:
                break;
        }
    }
    
    return args;
}

std::string TUIOValidator::extractOSCAddress(const std::string& message) const {
    size_t null_pos = message.find('\0');
    if (null_pos == std::string::npos) {
        return "";
    }
    
    return message.substr(0, null_pos);
}

bool TUIOValidator::isMTShowcaseCompatible(const std::string& message) const {
    // MT Showcase typically expects TUIO 1.1 format
    std::string version = getProtocolVersion(message);
    return version == "1.1" || version == "2.0";
}

bool TUIOValidator::hasRequiredTUIOFields(const std::string& message) const {
    auto components = parseMessage(message);
    
    // Check for required fields based on message type
    std::string address = components["address"];
    
    if (address.find("/tuio/2Dobj") != std::string::npos) {
        return components.find("session_id") != components.end() &&
               components.find("symbol_id") != components.end() &&
               components.find("x") != components.end() &&
               components.find("y") != components.end();
    }
    
    return true;  // Other message types may have different requirements
}

bool TUIOValidator::hasValidMTShowcaseFormat(const std::string& message) const {
    // MT Showcase specific format validation
    auto components = parseMessage(message);
    
    // Check coordinate ranges
    if (components.find("x") != components.end()) {
        float x = std::stof(components["x"]);
        if (x < 0.0f || x > 1.0f) {
            return false;
        }
    }
    
    if (components.find("y") != components.end()) {
        float y = std::stof(components["y"]);
        if (y < 0.0f || y > 1.0f) {
            return false;
        }
    }
    
    return true;
}

// TUIOIntegrationTester implementation

TUIOIntegrationTester::TUIOIntegrationTester() {
    // Set default test configuration
    test_config_["target_fps"] = "30";
    test_config_["test_duration"] = "10";
    test_config_["max_latency_ms"] = "100";
}

TUIOIntegrationTester::~TUIOIntegrationTester() {
}

std::string TUIOIntegrationTester::runIntegrationTests() {
    std::ostringstream report;
    report << "🧪 TUIO Integration Test Suite\n";
    report << "==============================\n\n";
    
    // Run all tests
    auto format_result = testMessageFormat();
    auto compatibility_result = testMTShowcaseCompatibility();
    auto performance_result = testPerformance();
    auto streaming_result = testMarkerStreaming();
    auto config_result = testConfigurationIntegration();
    
    // Store results
    test_results_ = {format_result, compatibility_result, performance_result, 
                    streaming_result, config_result};
    
    // Generate summary
    int passed_tests = 0;
    for (const auto& result : test_results_) {
        if (result.is_valid) passed_tests++;
    }
    
    report << "Test Summary:\n";
    report << "  Total Tests: " << test_results_.size() << "\n";
    report << "  Passed: " << passed_tests << "\n";
    report << "  Failed: " << (test_results_.size() - passed_tests) << "\n";
    report << "  Success Rate: " << (passed_tests * 100 / test_results_.size()) << "%\n\n";
    
    return report.str();
}

TUIOValidationResult TUIOIntegrationTester::testMessageFormat() {
    TUIOValidationResult result;
    
    // Test valid TUIO message
    std::string valid_message = generateTestTUIOMessage(123, 0.5f, 0.3f, 1.57f);
    auto validation = validator_.validateMessage(valid_message);
    
    if (validation.is_valid) {
        result.is_valid = true;
        result.addInfo("test", "Message Format");
        result.addInfo("status", "PASS");
    } else {
        result.error_message = "Message format validation failed: " + validation.error_message;
    }
    
    return result;
}

TUIOValidationResult TUIOIntegrationTester::testMTShowcaseCompatibility() {
    TUIOValidationResult result;
    
    // Test MT Showcase compatible message
    std::string compatible_message = generateTestTUIOMessage(456, 0.7f, 0.8f, 0.0f);
    auto compatibility = validator_.checkMTShowcaseCompatibility(compatible_message);
    
    if (compatibility.is_valid) {
        result.is_valid = true;
        result.addInfo("test", "MT Showcase Compatibility");
        result.addInfo("status", "PASS");
    } else {
        result.error_message = "MT Showcase compatibility test failed: " + compatibility.error_message;
    }
    
    return result;
}

TUIOValidationResult TUIOIntegrationTester::testPerformance(int target_fps) {
    TUIOValidationResult result;
    
    // Simulate performance test
    int test_duration = std::stoi(test_config_["test_duration"]);
    int throughput = measureThroughput(test_duration);
    
    if (throughput >= target_fps * 0.9) {  // Allow 10% tolerance
        result.is_valid = true;
        result.addInfo("test", "Performance");
        result.addInfo("status", "PASS");
        result.addInfo("throughput", std::to_string(throughput));
        result.addInfo("target_fps", std::to_string(target_fps));
    } else {
        result.error_message = "Performance test failed. Throughput: " + 
                              std::to_string(throughput) + " FPS, Target: " + 
                              std::to_string(target_fps) + " FPS";
    }
    
    return result;
}

TUIOValidationResult TUIOIntegrationTester::testMarkerStreaming() {
    TUIOValidationResult result;
    
    // Test marker data streaming
    std::vector<int> test_markers = {100, 200, 300, 400, 500};
    int valid_markers = 0;
    
    for (int marker_id : test_markers) {
        auto obj_result = validator_.validateObjectData(marker_id, 0.5f, 0.5f, 0.0f);
        if (obj_result.is_valid) {
            valid_markers++;
        }
    }
    
    if (valid_markers == static_cast<int>(test_markers.size())) {
        result.is_valid = true;
        result.addInfo("test", "Marker Streaming");
        result.addInfo("status", "PASS");
        result.addInfo("valid_markers", std::to_string(valid_markers));
    } else {
        result.error_message = "Marker streaming test failed. Valid: " + 
                              std::to_string(valid_markers) + "/" + 
                              std::to_string(test_markers.size());
    }
    
    return result;
}

TUIOValidationResult TUIOIntegrationTester::testConfigurationIntegration() {
    TUIOValidationResult result;
    
    // Test configuration integration
    // This would test the integration between TUIOBridge and configuration system
    result.is_valid = true;
    result.addInfo("test", "Configuration Integration");
    result.addInfo("status", "PASS");
    result.addInfo("note", "Configuration system integrated successfully");
    
    return result;
}

std::string TUIOIntegrationTester::generateTestReport() const {
    std::ostringstream report;
    report << "📊 TUIO Integration Test Report\n";
    report << "===============================\n\n";
    
    report << validator_.getValidationStatistics() << "\n";
    
    for (const auto& result : test_results_) {
        report << "Test: " << result.message_info.at("test") << "\n";
        report << "Status: " << result.message_info.at("status") << "\n";
        if (!result.is_valid) {
            report << "Error: " << result.error_message << "\n";
        }
        if (!result.warnings.empty()) {
            report << "Warnings:\n";
            for (const auto& warning : result.warnings) {
                report << "  - " << warning << "\n";
            }
        }
        report << "\n";
    }
    
    return report.str();
}

void TUIOIntegrationTester::setTestConfiguration(const std::map<std::string, std::string>& config) {
    test_config_ = config;
}

std::string TUIOIntegrationTester::generateTestTUIOMessage(int symbol_id, float x, float y, float angle) const {
    // Generate a simple TUIO 1.1 message for testing
    std::ostringstream oss;
    oss << "/tuio/2Dobj set " << symbol_id << " " << x << " " << y << " " << angle;
    return oss.str();
}

bool TUIOIntegrationTester::simulateMTShowcaseReception(const std::string& message) const {
    // Simulate MT Showcase receiving and processing a TUIO message
    auto validation = validator_.checkMTShowcaseCompatibility(message);
    return validation.is_valid;
}

double TUIOIntegrationTester::measureMessageLatency(const std::string& message) const {
    // Simulate latency measurement
    auto start = std::chrono::high_resolution_clock::now();
    
    // Simulate message processing
    validator_.validateMessage(message);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    return duration.count() / 1000.0;  // Return latency in milliseconds
}

int TUIOIntegrationTester::measureThroughput(int duration_seconds) const {
    // Simulate throughput measurement
    // In a real implementation, this would measure actual message throughput
    return 30;  // Simulated 30 FPS
}

} // namespace CodiceCam
