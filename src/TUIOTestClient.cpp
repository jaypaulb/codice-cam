#include "TUIOTestClient.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace CodiceCam {

TUIOTestClient::TUIOTestClient()
    : window_(nullptr)
    , renderer_(nullptr)
    , running_(false)
    , debug_mode_(false)
    , window_width_(800)
    , window_height_(600)
    , total_objects_received_(0)
    , total_updates_received_(0)
    , total_objects_removed_(0)
    , start_time_(std::chrono::steady_clock::now())
{
    initializeColors();
}

TUIOTestClient::~TUIOTestClient() {
    cleanup();
}

bool TUIOTestClient::initialize(int width, int height, const std::string& title) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "❌ Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return false;
    }
    
    window_width_ = width;
    window_height_ = height;
    
    window_ = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        window_width_,
        window_height_,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    
    if (!window_) {
        std::cerr << "❌ Failed to create window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return false;
    }
    
    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer_) {
        std::cerr << "❌ Failed to create renderer: " << SDL_GetError() << std::endl;
        cleanup();
        return false;
    }
    
    // Set renderer properties
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    
    std::cout << "✅ TUIO Test Client initialized: " << width << "x" << height << std::endl;
    return true;
}

bool TUIOTestClient::start() {
    if (!window_ || !renderer_) {
        std::cerr << "❌ Client not initialized" << std::endl;
        return false;
    }
    
    running_ = true;
    start_time_ = std::chrono::steady_clock::now();
    
    std::cout << "🚀 TUIO Test Client started" << std::endl;
    std::cout << "📋 Controls:" << std::endl;
    std::cout << "  - ESC or Close Window: Exit" << std::endl;
    std::cout << "  - D: Toggle debug mode" << std::endl;
    std::cout << "  - R: Reset statistics" << std::endl;
    
    // Main render loop
    while (running_) {
        handleEvents();
        render();
        SDL_Delay(16); // ~60 FPS
    }
    
    return true;
}

void TUIOTestClient::stop() {
    running_ = false;
    std::cout << "🛑 TUIO Test Client stopped" << std::endl;
}

void TUIOTestClient::updateObject(int session_id, int symbol_id, float x, float y, float angle,
                                 float velocity_x, float velocity_y, float rotation_velocity, float acceleration) {
    auto it = objects_.find(session_id);
    bool is_new_object = (it == objects_.end());
    
    TUIOObject& obj = objects_[session_id];
    obj.session_id = session_id;
    obj.symbol_id = symbol_id;
    obj.x = x;
    obj.y = y;
    obj.angle = angle;
    obj.velocity_x = velocity_x;
    obj.velocity_y = velocity_y;
    obj.rotation_velocity = rotation_velocity;
    obj.acceleration = acceleration;
    obj.last_update = std::chrono::steady_clock::now();
    obj.is_active = true;
    
    if (is_new_object) {
        total_objects_received_++;
        if (debug_mode_) {
            std::cout << "🆕 New object: ID=" << symbol_id << " Session=" << session_id 
                      << " at (" << std::fixed << std::setprecision(2) << x << ", " << y << ")" << std::endl;
        }
    } else {
        total_updates_received_++;
        if (debug_mode_) {
            std::cout << "🔄 Update object: ID=" << symbol_id << " Session=" << session_id 
                      << " at (" << std::fixed << std::setprecision(2) << x << ", " << y << ")" << std::endl;
        }
    }
}

void TUIOTestClient::removeObject(int session_id) {
    auto it = objects_.find(session_id);
    if (it != objects_.end()) {
        objects_.erase(it);
        total_objects_removed_++;
        if (debug_mode_) {
            std::cout << "❌ Removed object: Session=" << session_id << std::endl;
        }
    }
}

bool TUIOTestClient::isRunning() const {
    return running_;
}

std::string TUIOTestClient::getStatistics() const {
    std::ostringstream oss;
    auto now = std::chrono::steady_clock::now();
    auto elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_).count();
    
    oss << "TUIO Test Client Statistics:\n";
    oss << "  Uptime: " << elapsed_seconds << " seconds\n";
    oss << "  Active Objects: " << objects_.size() << "\n";
    oss << "  Objects Received: " << total_objects_received_ << "\n";
    oss << "  Updates Received: " << total_updates_received_ << "\n";
    oss << "  Objects Removed: " << total_objects_removed_ << "\n";
    
    if (elapsed_seconds > 0) {
        double updates_per_second = static_cast<double>(total_updates_received_) / elapsed_seconds;
        oss << "  Updates/Second: " << std::fixed << std::setprecision(1) << updates_per_second << "\n";
    }
    
    return oss.str();
}

void TUIOTestClient::setDebugMode(bool debug) {
    debug_mode_ = debug;
    std::cout << "🐛 Debug mode: " << (debug ? "enabled" : "disabled") << std::endl;
}

void TUIOTestClient::handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                running_ = false;
                break;
                
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        running_ = false;
                        break;
                    case SDLK_d:
                        setDebugMode(!debug_mode_);
                        break;
                    case SDLK_r:
                        total_objects_received_ = 0;
                        total_updates_received_ = 0;
                        total_objects_removed_ = 0;
                        start_time_ = std::chrono::steady_clock::now();
                        std::cout << "🔄 Statistics reset" << std::endl;
                        break;
                }
                break;
                
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    window_width_ = event.window.data1;
                    window_height_ = event.window.data2;
                }
                break;
        }
    }
}

void TUIOTestClient::render() {
    // Clear screen with dark background
    SDL_SetRenderDrawColor(renderer_, 20, 20, 20, 255);
    SDL_RenderClear(renderer_);
    
    // Render grid
    renderGrid();
    
    // Render all active objects
    for (const auto& [session_id, obj] : objects_) {
        if (obj.is_active) {
            renderObject(obj);
        }
    }
    
    // Render statistics
    renderStatistics();
    
    // Present the rendered frame
    SDL_RenderPresent(renderer_);
}

void TUIOTestClient::renderObject(const TUIOObject& obj) {
    int screen_x = screenX(obj.x);
    int screen_y = screenY(obj.y);
    float screen_angle = screenAngle(obj.angle);
    
    // Get marker color
    SDL_Color color = getMarkerColor(obj.symbol_id);
    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, 255);
    
    // Draw marker as a circle with ID
    int radius = 20;
    SDL_Rect marker_rect = {screen_x - radius, screen_y - radius, radius * 2, radius * 2};
    
    // Draw filled circle (approximated with multiple rectangles)
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            if (x * x + y * y <= radius * radius) {
                SDL_RenderDrawPoint(renderer_, screen_x + x, screen_y + y);
            }
        }
    }
    
    // Draw marker border
    SDL_SetRenderDrawColor(renderer_, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer_, &marker_rect);
    
    // Draw orientation line
    int line_length = 30;
    int end_x = screen_x + static_cast<int>(line_length * cos(screen_angle));
    int end_y = screen_y + static_cast<int>(line_length * sin(screen_angle));
    SDL_RenderDrawLine(renderer_, screen_x, screen_y, end_x, end_y);
    
    // Draw marker ID (simplified as a small rectangle)
    if (debug_mode_) {
        SDL_Rect id_rect = {screen_x + radius + 5, screen_y - 10, 30, 20};
        SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 128);
        SDL_RenderFillRect(renderer_, &id_rect);
        SDL_SetRenderDrawColor(renderer_, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer_, &id_rect);
    }
}

void TUIOTestClient::renderStatistics() {
    // Draw statistics background
    SDL_Rect stats_bg = {10, 10, 300, 120};
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 180);
    SDL_RenderFillRect(renderer_, &stats_bg);
    SDL_SetRenderDrawColor(renderer_, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer_, &stats_bg);
    
    // Note: In a real implementation, you'd use SDL_ttf for text rendering
    // For now, we'll just show the object count visually
    std::string stats_text = "Objects: " + std::to_string(objects_.size()) + 
                           " | Updates: " + std::to_string(total_updates_received_);
    
    // Draw simple text representation (rectangles for each character)
    int text_x = 20;
    int text_y = 30;
    int char_width = 8;
    int char_height = 12;
    
    for (char c : stats_text) {
        if (c != ' ') {
            SDL_Rect char_rect = {text_x, text_y, char_width, char_height};
            SDL_SetRenderDrawColor(renderer_, 255, 255, 255, 255);
            SDL_RenderDrawRect(renderer_, &char_rect);
        }
        text_x += char_width + 2;
    }
}

void TUIOTestClient::renderGrid() {
    // Draw a subtle grid
    SDL_SetRenderDrawColor(renderer_, 40, 40, 40, 255);
    
    // Vertical lines
    for (int x = 0; x < window_width_; x += 50) {
        SDL_RenderDrawLine(renderer_, x, 0, x, window_height_);
    }
    
    // Horizontal lines
    for (int y = 0; y < window_height_; y += 50) {
        SDL_RenderDrawLine(renderer_, 0, y, window_width_, y);
    }
}

SDL_Color TUIOTestClient::getMarkerColor(int symbol_id) const {
    if (marker_colors_.empty()) {
        return {255, 255, 255, 255}; // Default white
    }
    
    int color_index = symbol_id % marker_colors_.size();
    return marker_colors_[color_index];
}

void TUIOTestClient::initializeColors() {
    // Initialize a palette of distinct colors for different markers
    marker_colors_ = {
        {255, 100, 100, 255}, // Red
        {100, 255, 100, 255}, // Green
        {100, 100, 255, 255}, // Blue
        {255, 255, 100, 255}, // Yellow
        {255, 100, 255, 255}, // Magenta
        {100, 255, 255, 255}, // Cyan
        {255, 150, 100, 255}, // Orange
        {150, 100, 255, 255}, // Purple
        {100, 255, 150, 255}, // Light Green
        {255, 200, 100, 255}  // Light Orange
    };
}

void TUIOTestClient::cleanup() {
    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
    
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    
    SDL_Quit();
}

int TUIOTestClient::screenX(float tuio_x) const {
    return static_cast<int>(tuio_x * window_width_);
}

int TUIOTestClient::screenY(float tuio_y) const {
    return static_cast<int>(tuio_y * window_height_);
}

float TUIOTestClient::screenAngle(float tuio_angle) const {
    return tuio_angle; // TUIO angles are already in radians
}

} // namespace CodiceCam
