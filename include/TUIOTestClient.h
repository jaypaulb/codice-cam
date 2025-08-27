#pragma once

#include <SDL2/SDL.h>
#include <vector>
#include <map>
#include <chrono>
#include <string>
#include <memory>

namespace CodiceCam {

/**
 * @brief Represents a TUIO object for visualization
 */
struct TUIOObject {
    int session_id;
    int symbol_id;
    float x, y;
    float angle;
    float velocity_x, velocity_y;
    float rotation_velocity;
    float acceleration;
    std::chrono::steady_clock::time_point last_update;
    bool is_active;
    
    TUIOObject() : session_id(0), symbol_id(0), x(0), y(0), angle(0),
                   velocity_x(0), velocity_y(0), rotation_velocity(0), acceleration(0),
                   is_active(false) {}
};

/**
 * @brief Visual TUIO test client for displaying received markers
 */
class TUIOTestClient {
public:
    TUIOTestClient();
    ~TUIOTestClient();
    
    /**
     * @brief Initialize the test client
     * @param width Window width
     * @param height Window height
     * @param title Window title
     * @return true if initialization successful, false otherwise
     */
    bool initialize(int width = 800, int height = 600, const std::string& title = "TUIO Test Client");
    
    /**
     * @brief Start the test client main loop
     * @return true if started successfully, false otherwise
     */
    bool start();
    
    /**
     * @brief Stop the test client
     */
    void stop();
    
    /**
     * @brief Update TUIO object data
     * @param session_id TUIO session ID
     * @param symbol_id TUIO symbol ID
     * @param x X coordinate (0.0-1.0)
     * @param y Y coordinate (0.0-1.0)
     * @param angle Rotation angle in radians
     * @param velocity_x X velocity
     * @param velocity_y Y velocity
     * @param rotation_velocity Angular velocity
     * @param acceleration Acceleration
     */
    void updateObject(int session_id, int symbol_id, float x, float y, float angle,
                     float velocity_x = 0.0f, float velocity_y = 0.0f, 
                     float rotation_velocity = 0.0f, float acceleration = 0.0f);
    
    /**
     * @brief Remove TUIO object
     * @param session_id TUIO session ID
     */
    void removeObject(int session_id);
    
    /**
     * @brief Check if client is running
     * @return true if running, false otherwise
     */
    bool isRunning() const;
    
    /**
     * @brief Get statistics
     * @return Statistics string
     */
    std::string getStatistics() const;
    
    /**
     * @brief Set debug mode
     * @param debug Enable debug mode
     */
    void setDebugMode(bool debug);

private:
    SDL_Window* window_;
    SDL_Renderer* renderer_;
    bool running_;
    bool debug_mode_;
    int window_width_;
    int window_height_;
    
    // TUIO objects
    std::map<int, TUIOObject> objects_;
    
    // Statistics
    int total_objects_received_;
    int total_updates_received_;
    int total_objects_removed_;
    std::chrono::steady_clock::time_point start_time_;
    
    // Colors for different markers
    std::vector<SDL_Color> marker_colors_;
    
    // Event handling
    void handleEvents();
    void render();
    void renderObject(const TUIOObject& obj);
    void renderStatistics();
    void renderGrid();
    
    // Utility functions
    SDL_Color getMarkerColor(int symbol_id) const;
    void initializeColors();
    void cleanup();
    
    // Coordinate conversion
    int screenX(float tuio_x) const;
    int screenY(float tuio_y) const;
    float screenAngle(float tuio_angle) const;
};

} // namespace CodiceCam
