/*
    macOS Compatible Asteroids
    Added Wrap-Around Collision, Grey Asteroids, Yellow Bullets, and Red Ship Nose
*/

#include <iostream>
#include <string>
#include <algorithm>
#include <vector>
#include <cmath>
#include <cstdlib>
using namespace std;

// Custom POSIX-compatible terminal rendering and input engine
#include "olcConsoleGameEngine_mac.h"

class OneLoneCoder_Asteroids : public olcConsoleGameEngine
{
public:
    OneLoneCoder_Asteroids() { m_sAppName = L"Asteroids"; }

private:
    // Core data structures representing physical entities in the game world
    struct sSpaceObject { int nSize; float x; float y; float dx; float dy; float angle; };
    struct sStar { float x; float y; };
    struct sParticle { float x; float y; float dx; float dy; float life; };

    // Object pools containing all active entities
    vector<sSpaceObject> vecAsteroids;
    vector<sSpaceObject> vecBullets;
    vector<sStar> vecStars;
    vector<sParticle> vecParticles;
    
    // Player state variables
    sSpaceObject player;
    bool bDead = false;
    int nScore = 0;

    // Vector models (geometry) for the ship and asteroids
    vector<pair<float, float>> vecModelShip;
    vector<pair<float, float>> vecModelAsteroid;

protected:
    // Called once at the start of the application. Used to initialize geometry and backgrounds.
    virtual bool OnUserCreate()
    {
        // Define the vertices of the player's ship (an arrowhead pointing "up")
        // Index 0 is the nose of the ship.
        vecModelShip = { 
            { 0.0f, -2.5f }, // Nose 
            {-1.5f, +1.5f }, // Left Wing
            { 0.0f, +0.5f }, // Engine Indent (Back)
            {+1.5f, +1.5f }  // Right Wing
        };
        
        // Procedurally generate a jagged, circular shape for the asteroids
        // Uses sine and cosine with added noise to create a rocky, irregular polygon
        int verts = 20;
        for (int i = 0; i < verts; i++) {
            float noise = (float)rand() / (float)RAND_MAX * 0.4f + 0.8f;
            vecModelAsteroid.push_back(make_pair(noise * sinf(((float)i / (float)verts) * 6.28318f), 
                                                 noise * cosf(((float)i / (float)verts) * 6.28318f)));
        }

        // Generate Deep Space Starfield Background with random coordinates
        for (int i = 0; i < 100; i++) {
            vecStars.push_back({ (float)(rand() % ScreenWidth()), (float)(rand() % ScreenHeight()) });
        }

        ResetGame();
        return true;
    }

    // Clears the board and sets the player and asteroids back to their starting positions
    void ResetGame()
    {
        // Center the player and reset velocity/rotation
        player.x = ScreenWidth() / 2.0f; player.y = ScreenHeight() / 2.0f;
        player.dx = 0.0f; player.dy = 0.0f; player.angle = 0.0f;
        
        // Wipe all dynamic entities
        vecBullets.clear(); 
        vecAsteroids.clear();
        vecParticles.clear();

        // Spawn initial asteroids in safe locations away from the center
        vecAsteroids.push_back({ 4, 10.0f, 10.0f, 8.0f, -6.0f, 0.0f });
        vecAsteroids.push_back({ 4, 50.0f, 25.0f, -5.0f, 3.0f, 0.0f });

        bDead = false; 
        nScore = 0; 
    }

    // Enforces toroidal space mapping (if an object flies off one edge, it wraps to the opposite edge)
    void WrapCoordinates(float ix, float iy, float &ox, float &oy)
    {
        ox = ix; oy = iy;
        if (ix < 0.0f)  ox = ix + (float)ScreenWidth();
        if (ix >= (float)ScreenWidth()) ox = ix - (float)ScreenWidth();
        if (iy < 0.0f)  oy = iy + (float)ScreenHeight();
        if (iy >= (float)ScreenHeight()) oy = iy - (float)ScreenHeight();
    }

    // Overrides the base Draw function to apply coordinate wrapping to all drawn pixels
    virtual void Draw(int x, int y, short c = 0x2588, short col = FG_WHITE)
    {
        float fx, fy; WrapCoordinates((float)x, (float)y, fx, fy);      
        olcConsoleGameEngine::Draw((int)fx, (int)fy, c, col);
    }

    // Circular Collision Detection: Accounts for screen wrap-around distances
    bool IsPointInsideCircle(float cx, float cy, float radius, float x, float y)
    {
        float dx = fabs(x - cx);
        float dy = fabs(y - cy);

        // If absolute distance is greater than half the screen, the shortest path is wrapping around the edge
        if (dx > ScreenWidth() / 2.0f) dx = ScreenWidth() - dx;
        if (dy > ScreenHeight() / 2.0f) dy = ScreenHeight() - dy;

        return sqrt(dx*dx + dy*dy) < radius;
    }

    // Main Game Loop: Executed once per frame
    virtual bool OnUserUpdate(float fElapsedTime)
    {
        // --- 1. CLEAR SCREEN & DRAW BACKGROUND ---
        Fill(0, 0, ScreenWidth(), ScreenHeight(), ' ', FG_BLACK);

        for (auto &star : vecStars) {
            Draw((int)star.x, (int)star.y, L'.', FG_DARK_GREY);
        }

        // --- 2. HANDLE GAME OVER STATE ---
        if (bDead)
        {
            DrawString(ScreenWidth() / 2 - 4, ScreenHeight() / 2 - 2, L"GAME OVER", FG_RED);
            DrawString(ScreenWidth() / 2 - 11, ScreenHeight() / 2 + 1, L"PRESS SPACE TO RESTART", FG_WHITE);
            DrawString(ScreenWidth() / 2 - 5, ScreenHeight() / 2 + 3, L"SCORE: " + to_wstring(nScore), FG_GREEN);

            // Wait for input to restart, halting all other game logic
            if (m_keys[' '].bPressed) ResetGame();
            return true; 
        }

        // --- 3. PLAYER INPUT & MOVEMENT ---
        // Rotate the ship (framerate-independent using fElapsedTime)
        if (m_keys['A'].bHeld) player.angle -= 4.5f * fElapsedTime;
        if (m_keys['D'].bHeld) player.angle += 4.5f * fElapsedTime;

        // Apply forward thrust using trigonometry
        if (m_keys['W'].bHeld) {
            player.dx += sin(player.angle) * 120.0f * fElapsedTime;
            player.dy += -cos(player.angle) * 120.0f * fElapsedTime;

            // Spawn Thruster Particles in the opposite direction of the ship's heading
            vecParticles.push_back({
                player.x - sin(player.angle) * 3.0f, 
                player.y + cos(player.angle) * 3.0f,
                player.dx * -0.2f + (float)((rand() % 20) - 10), // Add random scatter to exhaust
                player.dy * -0.2f + (float)((rand() % 20) - 10),
                0.3f // Particle lifespan in seconds
            });
        }

        // --- 4. PLAYER PHYSICS & COLLISION ---
        // Update position based on current velocity
        player.x += player.dx * fElapsedTime;
        player.y += player.dy * fElapsedTime;
        
        // Apply friction/drag so the ship doesn't drift infinitely
        player.dx -= player.dx * 3.5f * fElapsedTime; 
        player.dy -= player.dy * 3.5f * fElapsedTime; 
        
        WrapCoordinates(player.x, player.y, player.x, player.y);

        // Check for collisions between the player and any active asteroid
        for (auto &a : vecAsteroids)
            if (IsPointInsideCircle(a.x, a.y, a.nSize + 1.5f, player.x, player.y)) 
                bDead = true;

        // --- 5. WEAPON FIRING ---
        if (m_keys[' '].bPressed)
            vecBullets.push_back({ 0, player.x, player.y, 80.0f * sinf(player.angle), -80.0f * cosf(player.angle), 100.0f });

        // --- 6. UPDATE ASTEROIDS ---
        for (auto &a : vecAsteroids) {
            a.x += a.dx * fElapsedTime;
            a.y += a.dy * fElapsedTime;
            a.angle += 0.5f * fElapsedTime; // Constant spin
            WrapCoordinates(a.x, a.y, a.x, a.y);
            
            // Render smaller asteroids as a darker color
            short astCol = FG_GREY;
            if (a.nSize <= 2) astCol = FG_DARK_GREY; 

            DrawWireFrameModel(vecModelAsteroid, a.x, a.y, a.angle, (float)a.nSize, astCol); 
        }

        // --- 7. UPDATE PARTICLES ---
        for (auto &p : vecParticles) {
            p.x += p.dx * fElapsedTime;
            p.y += p.dy * fElapsedTime;
            p.life -= fElapsedTime; // Degrade lifespan
            WrapCoordinates(p.x, p.y, p.x, p.y);
            
            // Change color as the particle cools down
            if (p.life > 0.15f) Draw((int)p.x, (int)p.y, PIXEL_QUARTER, FG_YELLOW);
            else if (p.life > 0.0f) Draw((int)p.x, (int)p.y, PIXEL_QUARTER, FG_RED); 
        }
        
        // Clean up dead particles using the Erase-Remove idiom
        vecParticles.erase(remove_if(vecParticles.begin(), vecParticles.end(), [&](sParticle p) { return p.life <= 0.0f; }), vecParticles.end());

        // --- 8. UPDATE BULLETS & HANDLE ASTEROID DESTRUCTION ---
        vector<sSpaceObject> newAsteroids; // Temporary buffer to hold new asteroid fragments

        for (auto &b : vecBullets) {
            // Update bullet physics
            b.x += b.dx * fElapsedTime; b.y += b.dy * fElapsedTime;
            WrapCoordinates(b.x, b.y, b.x, b.y);
            b.angle -= 1.0f * fElapsedTime;

            // Check collision against all asteroids
            for (auto &a : vecAsteroids) {
                if(IsPointInsideCircle(a.x, a.y, a.nSize, b.x, b.y)) {
                    b.x = -100; // Flag bullet for deletion by moving it out of bounds
                    
                    // If the asteroid is large enough, break it into two smaller fragments
                    if (a.nSize > 1) {
                        float angle1 = ((float)rand() / (float)RAND_MAX) * 6.283185f;
                        float angle2 = ((float)rand() / (float)RAND_MAX) * 6.283185f;
                        // Spawn fragments at half the size (bit shifted) with randomized velocity
                        newAsteroids.push_back({ a.nSize >> 1 ,a.x, a.y, 10.0f * sinf(angle1), 10.0f * cosf(angle1), 0.0f });
                        newAsteroids.push_back({ a.nSize >> 1 ,a.x, a.y, 10.0f * sinf(angle2), 10.0f * cosf(angle2), 0.0f });
                    }
                    a.x = -100; // Flag parent asteroid for deletion
                    nScore += 100; 
                }
            }
        }

        // Append generated child fragments to the main asteroid vector
        for(auto a : newAsteroids) vecAsteroids.push_back(a);

        // --- 9. ENTITY CLEANUP & LEVEL PROGRESSION ---
        // Remove flagged (destroyed) asteroids
        if (vecAsteroids.size() > 0) {
            auto i = remove_if(vecAsteroids.begin(), vecAsteroids.end(), [&](sSpaceObject o) { return (o.x < 0); });
            if (i != vecAsteroids.end()) vecAsteroids.erase(i, vecAsteroids.end());
        }

        // If the board is clear, level up and spawn new targets
        if (vecAsteroids.empty()) {
            nScore += 1000; 
            vecAsteroids.clear(); vecBullets.clear();
            
            vecAsteroids.push_back({ 4, 10.0f, 10.0f, 8.0f, -6.0f, 0.0f });
            vecAsteroids.push_back({ 4, 50.0f, 25.0f, -5.0f, 3.0f, 0.0f });
        }

        // Remove bullets that have traveled out of the visible screen bounds
        if (vecBullets.size() > 0) {
            auto i = remove_if(vecBullets.begin(), vecBullets.end(), [&](sSpaceObject o) { return (o.x < 1 || o.y < 1 || o.x >= ScreenWidth() - 1 || o.y >= ScreenHeight() - 1); });
            if (i != vecBullets.end()) vecBullets.erase(i, vecBullets.end());
        }

        // --- 10. FINAL RENDERING ---
        // Draw Bullets 
        for (auto b : vecBullets) Draw((int)b.x, (int)b.y, PIXEL_SOLID, FG_YELLOW);

        // Draw Player Ship (Base White Frame)
        DrawWireFrameModel(vecModelShip, player.x, player.y, player.angle, 1.0f, FG_WHITE);
        
        // Calculate the exact spatial coordinate of the ship's nose (Index 0) and draw it Red
        float noseX = player.x + vecModelShip[0].first * cosf(player.angle) - vecModelShip[0].second * sinf(player.angle);
        float noseY = player.y + vecModelShip[0].first * sinf(player.angle) + vecModelShip[0].second * cosf(player.angle);
        WrapCoordinates(noseX, noseY, noseX, noseY);
        Draw((int)noseX, (int)noseY, PIXEL_SOLID, FG_RED);
        
        // Render Score UI
        if (!bDead) {
            DrawString(2, 2, L"SCORE: " + to_wstring(nScore), FG_GREEN);
        }
        
        return true;
    }

    // Handles the transformation pipeline for vector graphics
    void DrawWireFrameModel(const vector<pair<float, float>> &vecModelCoordinates, float x, float y, float r = 0.0f, float s = 1.0f, short col = FG_WHITE)
    {
        vector<pair<float, float>> vecTransformedCoordinates;
        int verts = vecModelCoordinates.size();
        vecTransformedCoordinates.resize(verts);

        // 1. Rotate the geometry
        for (int i = 0; i < verts; i++) {
            vecTransformedCoordinates[i].first = vecModelCoordinates[i].first * cosf(r) - vecModelCoordinates[i].second * sinf(r);
            vecTransformedCoordinates[i].second = vecModelCoordinates[i].first * sinf(r) + vecModelCoordinates[i].second * cosf(r);
        }
        
        // 2. Scale the geometry
        for (int i = 0; i < verts; i++) {
            vecTransformedCoordinates[i].first = vecTransformedCoordinates[i].first * s;
            vecTransformedCoordinates[i].second = vecTransformedCoordinates[i].second * s;
        }
        
        // 3. Translate the geometry to its world-space position
        for (int i = 0; i < verts; i++) {
            vecTransformedCoordinates[i].first = vecTransformedCoordinates[i].first + x;
            vecTransformedCoordinates[i].second = vecTransformedCoordinates[i].second + y;
        }
        
        // 4. Draw the closed polygon by connecting the transformed vertices
        for (int i = 0; i < verts; i++) {
            int j = (i + 1) % verts; 
            DrawLine((int)vecTransformedCoordinates[i].first, (int)vecTransformedCoordinates[i].second, 
                (int)vecTransformedCoordinates[j].first, (int)vecTransformedCoordinates[j].second, PIXEL_SOLID, col);
        }
    }
};

int main()
{
    // Initialize and start the engine loop
    OneLoneCoder_Asteroids game;
    game.ConstructConsole(60, 30, 8, 8); 
    game.Start();
    return 0;
}
