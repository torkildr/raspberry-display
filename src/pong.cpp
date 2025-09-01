#include "pong.hpp"
#include "log_util.hpp"
#include "font.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>

namespace pong
{

PongGame::PongGame()
{
    // Initialize random seed for ball movement
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    
    // Create game update timer
    m_gameTimer = timer::createTimer(GAME_UPDATE_INTERVAL, [this]() {
        if (m_running) {
            update();
        }
    });
    
    reset();
}

PongGame::~PongGame()
{
    stop();
}

void PongGame::start()
{
    if (!m_running) {
        m_running = true;
        m_gameOver = false;
        DEBUG_LOG("Pong game started");
    }
}

void PongGame::stop()
{
    if (m_running) {
        m_running = false;
        DEBUG_LOG("Pong game stopped");
    }
}

bool PongGame::isRunning() const
{
    return m_running;
}

void PongGame::setPlayerControl(PaddleControl control)
{
    m_playerControl = control;
}

void PongGame::reset()
{
    m_ball = Ball();
    m_playerPaddle = Paddle();
    m_aiPaddle = Paddle();
    m_gameOver = false;
    m_controlTimeout = 0;
    m_gameOverTimer = 0;
    
    // Randomize initial ball direction
    m_ball.dx = (std::rand() % 2 == 0) ? -1.0f : 1.0f;
    m_ball.dy = ((static_cast<float>(std::rand() % 100)) / 100.0f - 0.5f) * 2.0f; // Random Y velocity between -1 and 1
    
    DEBUG_LOG("Pong game reset");
}

int PongGame::getPlayerScore() const
{
    return m_playerPaddle.score;
}

int PongGame::getAIScore() const
{
    return m_aiPaddle.score;
}

bool PongGame::shouldExit() const
{
    return m_gameOver && m_gameOverTimer >= GAME_OVER_DISPLAY_TIME;
}

void PongGame::update()
{
    if (m_gameOver) {
        // Increment game over timer
        m_gameOverTimer++;
        return;
    }
    
    // Update player paddle based on input (discrete movement with timeout)
    PaddleControl control = m_playerControl.load();
    
    // Handle new control input
    if (control != PaddleControl::NONE && m_controlTimeout <= 0) {
        if (control == PaddleControl::UP && m_playerPaddle.y > 0) {
            m_playerPaddle.y -= 1.0f; // Move 1 pixel per key press
            m_controlTimeout = 6; // 6 updates (~300ms at 20fps) before allowing next move
        } else if (control == PaddleControl::DOWN && m_playerPaddle.y < PONG_FIELD_HEIGHT - PONG_PADDLE_HEIGHT) {
            m_playerPaddle.y += 1.0f; // Move 1 pixel per key press
            m_controlTimeout = 6; // 6 updates (~300ms at 20fps) before allowing next move
        }
        // Reset control to prevent continuous movement
        m_playerControl = PaddleControl::NONE;
    }
    
    // Decrease timeout counter
    if (m_controlTimeout > 0) {
        m_controlTimeout--;
    }
    
    updateAI();
    updateBall();
    checkCollisions();
    checkScore();
}

void PongGame::updateBall()
{
    m_ball.x += m_ball.dx;
    m_ball.y += m_ball.dy;
    
    // Bounce off top and bottom walls
    if (m_ball.y <= 0 || m_ball.y >= PONG_FIELD_HEIGHT - 1) {
        m_ball.dy = -m_ball.dy;
        m_ball.y = std::max(0.0f, std::min(static_cast<float>(PONG_FIELD_HEIGHT - 1), m_ball.y));
    }
}

void PongGame::updateAI()
{
    // Improved AI: slower, with occasional mistakes and reaction delay
    float paddleCenter = m_aiPaddle.y + PONG_PADDLE_HEIGHT / 2.0f;
    float ballY = m_ball.y;
    
    // Add some randomness - 15% chance to make no move (reaction delay)
    if ((std::rand() % 100) < 15) {
        return;
    }
    
    // Add some randomness - 10% chance to move in wrong direction (mistake)
    bool makeWrongMove = (std::rand() % 100) < 10;
    
    // Increase dead zone to make AI less precise
    float deadZone = 1.2f;
    
    if (ballY < paddleCenter - deadZone && m_aiPaddle.y > 0) {
        if (makeWrongMove) {
            m_aiPaddle.y += PONG_AI_SPEED; // Wrong direction
        } else {
            m_aiPaddle.y -= PONG_AI_SPEED; // Correct direction
        }
    } else if (ballY > paddleCenter + deadZone && m_aiPaddle.y < PONG_FIELD_HEIGHT - PONG_PADDLE_HEIGHT) {
        if (makeWrongMove) {
            m_aiPaddle.y -= PONG_AI_SPEED; // Wrong direction
        } else {
            m_aiPaddle.y += PONG_AI_SPEED; // Correct direction
        }
    }
    
    // Keep AI paddle within bounds
    m_aiPaddle.y = std::max(0.0f, std::min(static_cast<float>(PONG_FIELD_HEIGHT - PONG_PADDLE_HEIGHT), m_aiPaddle.y));
}

void PongGame::checkCollisions()
{
    // Left paddle collision (player)
    if (m_ball.x <= 2 && m_ball.dx < 0) {
        if (m_ball.y >= m_playerPaddle.y && m_ball.y <= m_playerPaddle.y + PONG_PADDLE_HEIGHT) {
            m_ball.dx = -m_ball.dx;
            m_ball.x = 2;
            // Add some spin based on where the ball hits the paddle (reduced spin)
            float hitPos = (m_ball.y - m_playerPaddle.y) / PONG_PADDLE_HEIGHT;
            m_ball.dy += (hitPos - 0.5f) * 0.3f;
        }
    }
    
    // Right paddle collision (AI)
    if (m_ball.x >= PONG_FIELD_WIDTH - 3 && m_ball.dx > 0) {
        if (m_ball.y >= m_aiPaddle.y && m_ball.y <= m_aiPaddle.y + PONG_PADDLE_HEIGHT) {
            m_ball.dx = -m_ball.dx;
            m_ball.x = PONG_FIELD_WIDTH - 3;
            // Add some spin based on where the ball hits the paddle (reduced spin)
            float hitPos = (m_ball.y - m_aiPaddle.y) / PONG_PADDLE_HEIGHT;
            m_ball.dy += (hitPos - 0.5f) * 0.3f;
        }
    }
    
    // Apply velocity dampening to prevent excessive acceleration
    m_ball.dy *= 0.95f;
    
    // Keep ball speed reasonable with tighter constraints
    m_ball.dy = std::max(-1.5f, std::min(1.5f, m_ball.dy));
    
    // Ensure minimum vertical speed isn't too small (prevents horizontal-only movement)
    if (std::abs(m_ball.dy) < 0.1f) {
        m_ball.dy = (m_ball.dy >= 0) ? 0.1f : -0.1f;
    }
}

void PongGame::checkScore()
{
    // Check if ball went off the left or right side
    if (m_ball.x < 0) {
        m_aiPaddle.score++;
        resetBall();
    } else if (m_ball.x >= PONG_FIELD_WIDTH) {
        m_playerPaddle.score++;
        resetBall();
    }
    
    // Check for game over
    if (m_playerPaddle.score >= PONG_WINNING_SCORE || m_aiPaddle.score >= PONG_WINNING_SCORE) {
        m_gameOver = true;
        DEBUG_LOG("Pong game over! Player: " << m_playerPaddle.score << " AI: " << m_aiPaddle.score);
    }
}

void PongGame::resetBall()
{
    m_ball.x = PONG_FIELD_WIDTH / 2.0f;
    m_ball.y = PONG_FIELD_HEIGHT / 2.0f;
    
    // Random direction
    m_ball.dx = (std::rand() % 2 == 0) ? -1.0f : 1.0f;
    m_ball.dy = ((static_cast<float>(std::rand() % 100)) / 100.0f - 0.5f) * 1.0f;
}

void PongGame::renderToBuffer(std::array<uint8_t, X_MAX>& buffer)
{
    // Clear buffer
    buffer.fill(0);
    
    if (m_gameOver) {
        // Show game over text using the font system
        std::string winText;
        if (m_playerPaddle.score >= PONG_WINNING_SCORE) {
            winText = "You win!";
        } else {
            winText = "Computer wins!";
        }
        
        // Render the text using the font system
        std::vector<uint8_t> renderedText = font::FontCache::renderStringOptimized(winText);
        
        // Center the text horizontally on the display
        int textWidth = static_cast<int>(renderedText.size());
        int startX = (PONG_FIELD_WIDTH - textWidth) / 2;
        
        // Ensure we don't go outside buffer bounds
        startX = std::max(0, startX);
        int endX = std::min(PONG_FIELD_WIDTH, startX + textWidth);
        
        // Copy the rendered text to the buffer
        for (int x = startX; x < endX; x++) {
            int textIndex = x - startX;
            if (textIndex < static_cast<int>(renderedText.size())) {
                buffer[static_cast<size_t>(x)] = renderedText[static_cast<size_t>(textIndex)];
            }
        }
        return;
    }
    
    // Render game elements
    renderPaddle(buffer, 1, m_playerPaddle);                    // Left paddle
    renderPaddle(buffer, PONG_FIELD_WIDTH - 2, m_aiPaddle);     // Right paddle
    renderBall(buffer, m_ball);
    renderScore(buffer);
}

void PongGame::renderPaddle(std::array<uint8_t, X_MAX>& buffer, int x, const Paddle& paddle)
{
    for (int i = 0; i < PONG_PADDLE_HEIGHT; i++) {
        setPixel(buffer, x, static_cast<int>(paddle.y) + i, true);
    }
}

void PongGame::renderBall(std::array<uint8_t, X_MAX>& buffer, const Ball& ball)
{
    setPixel(buffer, static_cast<int>(ball.x), static_cast<int>(ball.y), true);
}

void PongGame::renderScore(std::array<uint8_t, X_MAX>& buffer)
{
    // Simple score display using pixels at the top
    // Player score on the left side (pixels 20-30)
    for (int i = 0; i < std::min(m_playerPaddle.score, 10); i++) {
        setPixel(buffer, 20 + i * 2, 0, true);
    }
    
    // AI score on the right side (pixels 98-108)
    for (int i = 0; i < std::min(m_aiPaddle.score, 10); i++) {
        setPixel(buffer, 98 + i * 2, 0, true);
    }
    
    // Center line
    for (int y = 1; y < PONG_FIELD_HEIGHT - 1; y += 2) {
        setPixel(buffer, PONG_FIELD_WIDTH / 2, y, true);
    }
}

void PongGame::setPixel(std::array<uint8_t, X_MAX>& buffer, int x, int y, bool on)
{
    if (x < 0 || x >= PONG_FIELD_WIDTH || y < 0 || y >= PONG_FIELD_HEIGHT) {
        return;
    }
    
    if (on) {
        buffer[static_cast<size_t>(x)] |= static_cast<uint8_t>(1 << y);
    } else {
        buffer[static_cast<size_t>(x)] &= static_cast<uint8_t>(~(1 << y));
    }
}

std::string createPongDisplayString()
{
    // Legacy function - no longer used since pong is independent of sequence system
    // Kept for backwards compatibility
    return "PONG_GAME_ACTIVE";
}

} // namespace pong
