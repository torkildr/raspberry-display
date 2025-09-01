#pragma once

#include <array>
#include <memory>
#include <atomic>
#include <functional>
#include <cstdint>

#include "timer.hpp"
#include "display.hpp"

#define PONG_PADDLE_HEIGHT 2
#define PONG_PADDLE_WIDTH 1
#define PONG_BALL_SIZE 1
#define PONG_FIELD_WIDTH X_MAX
#define PONG_FIELD_HEIGHT 8
#define PONG_AI_SPEED 0.4f
#define PONG_WINNING_SCORE 3


namespace pong
{

enum class Direction
{
    UP = -1,
    DOWN = 1
};

enum class PaddleControl
{
    NONE,
    UP,
    DOWN
};

struct Ball
{
    float x, y;
    float dx, dy;
    
    Ball() : x(PONG_FIELD_WIDTH / 2.0f), y(PONG_FIELD_HEIGHT / 2.0f), 
             dx(-1.0f), dy(0.5f) {}
};

struct Paddle
{
    float y;
    int score;
    
    Paddle() : y(PONG_FIELD_HEIGHT / 2.0f - PONG_PADDLE_HEIGHT / 2.0f), score(0) {}
};

class PongGame
{
public:
    PongGame();
    ~PongGame();
    
    // Game control
    void start();
    void stop();
    bool isRunning() const;
    
    // User input
    void setPlayerControl(PaddleControl control);
    
    // Display integration
    void renderToBuffer(std::array<uint8_t, X_MAX>& buffer);
    
    // Game state
    void reset();
    int getPlayerScore() const;
    int getAIScore() const;
    bool shouldExit() const; // Check if game should auto-exit after game over
    
private:
    void update();
    void updateBall();
    void updateAI();
    void checkCollisions();
    void checkScore();
    void resetBall();
    
    // Rendering helpers
    void renderPaddle(std::array<uint8_t, X_MAX>& buffer, int x, const Paddle& paddle);
    void renderBall(std::array<uint8_t, X_MAX>& buffer, const Ball& ball);
    void renderScore(std::array<uint8_t, X_MAX>& buffer);
    void setPixel(std::array<uint8_t, X_MAX>& buffer, int x, int y, bool on = true);
    
    Ball m_ball;
    Paddle m_playerPaddle;    // Left paddle (user controlled)
    Paddle m_aiPaddle;        // Right paddle (AI controlled)
    
    std::atomic<bool> m_running{false};
    std::atomic<PaddleControl> m_playerControl{PaddleControl::NONE};
    int m_controlTimeout{0};
    
    std::unique_ptr<timer::Timer> m_gameTimer;
    static constexpr std::chrono::milliseconds GAME_UPDATE_INTERVAL{50}; // 20 FPS
    
    // Game state
    bool m_gameOver = false;
    int m_gameOverTimer = 0;
    static constexpr int GAME_OVER_DISPLAY_TIME = 60; // 3 seconds at 20fps
};

// Helper function to create pong display state
std::string createPongDisplayString();

} // namespace pong
