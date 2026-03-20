/*
    macOS / POSIX Compatible Port of OneLoneCoder Console Game Engine
    Includes Input Smoothing & Key-Switching Fix for Mac Terminals.
*/

#pragma once

#include <iostream>
#include <chrono>
#include <vector>
#include <list>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <cstring>
#include <cmath>

#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <fcntl.h>

enum COLOUR
{
    FG_BLACK        = 30, FG_DARK_RED     = 31, FG_DARK_GREEN   = 32, FG_DARK_YELLOW  = 33,
    FG_DARK_BLUE    = 34, FG_DARK_MAGENTA = 35, FG_DARK_CYAN    = 36, FG_GREY         = 37,
    FG_DARK_GREY    = 90, FG_RED          = 91, FG_GREEN        = 92, FG_YELLOW       = 93,
    FG_BLUE         = 94, FG_MAGENTA      = 95, FG_CYAN         = 96, FG_WHITE        = 97,
    
    BG_BLACK        = 40, BG_DARK_RED     = 41, BG_DARK_GREEN   = 42, BG_DARK_YELLOW  = 43,
    BG_DARK_BLUE    = 44, BG_DARK_MAGENTA = 45, BG_DARK_CYAN    = 46, BG_GREY         = 47,
    BG_DARK_GREY    = 100, BG_RED         = 101, BG_GREEN       = 102, BG_YELLOW      = 103,
    BG_BLUE         = 104, BG_MAGENTA     = 105, BG_CYAN        = 106, BG_WHITE       = 107,
};

enum PIXEL_TYPE { PIXEL_SOLID = 0x2588, PIXEL_THREEQUARTERS = 0x2593, PIXEL_HALF = 0x2592, PIXEL_QUARTER = 0x2591 };

struct CHAR_INFO { wchar_t Char; short Attributes; };

class olcConsoleGameEngine
{
public:
    olcConsoleGameEngine()
    {
        m_nScreenWidth = 80;
        m_nScreenHeight = 30;
        std::memset(m_keyNewState, 0, 256 * sizeof(short));
        std::memset(m_keyOldState, 0, 256 * sizeof(short));
        std::memset(m_keys, 0, 256 * sizeof(sKeyState));
        m_sAppName = L"Default";
    }

    int ConstructConsole(int width, int height, int fontw, int fonth)
    {
        m_nScreenWidth = width;
        m_nScreenHeight = height;
        m_bufScreen = new CHAR_INFO[m_nScreenWidth*m_nScreenHeight];
        memset(m_bufScreen, 0, sizeof(CHAR_INFO) * m_nScreenWidth * m_nScreenHeight);

        tcgetattr(STDIN_FILENO, &m_OriginalTermios);
        struct termios raw = m_OriginalTermios;
        raw.c_lflag &= ~(ECHO | ICANON);
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

        std::cout << "\033[?25l\033[2J";
        return 1;
    }

    virtual void Draw(int x, int y, short c = 0x2588, short col = FG_WHITE)
    {
        if (x >= 0 && x < m_nScreenWidth && y >= 0 && y < m_nScreenHeight) {
            m_bufScreen[y * m_nScreenWidth + x].Char = c;
            m_bufScreen[y * m_nScreenWidth + x].Attributes = col;
        }
    }

    void Fill(int x1, int y1, int x2, int y2, short c = 0x2588, short col = FG_WHITE)
    {
        Clip(x1, y1); Clip(x2, y2);
        for (int x = x1; x < x2; x++)
            for (int y = y1; y < y2; y++)
                Draw(x, y, c, col);
    }

    void DrawString(int x, int y, std::wstring c, short col = FG_WHITE)
    {
        for (size_t i = 0; i < c.size(); i++)
            if (x + i < m_nScreenWidth && y < m_nScreenHeight) {
                m_bufScreen[y * m_nScreenWidth + x + i].Char = c[i];
                m_bufScreen[y * m_nScreenWidth + x + i].Attributes = col;
            }
    }

    void Clip(int &x, int &y)
    {
        if (x < 0) x = 0;
        if (x >= m_nScreenWidth) x = m_nScreenWidth;
        if (y < 0) y = 0;
        if (y >= m_nScreenHeight) y = m_nScreenHeight;
    }

    void DrawLine(int x1, int y1, int x2, int y2, short c = 0x2588, short col = FG_WHITE)
    {
        int x, y, dx, dy, dx1, dy1, px, py, xe, ye, i;
        dx = x2 - x1; dy = y2 - y1;
        dx1 = abs(dx); dy1 = abs(dy);
        px = 2 * dy1 - dx1;    py = 2 * dx1 - dy1;
        if (dy1 <= dx1) {
            if (dx >= 0) { x = x1; y = y1; xe = x2; } else { x = x2; y = y2; xe = x1;}
            Draw(x, y, c, col);
            for (i = 0; x<xe; i++) {
                x = x + 1;
                if (px<0) px = px + 2 * dy1;
                else {
                    if ((dx<0 && dy<0) || (dx>0 && dy>0)) y = y + 1; else y = y - 1;
                    px = px + 2 * (dy1 - dx1);
                }
                Draw(x, y, c, col);
            }
        } else {
            if (dy >= 0) { x = x1; y = y1; ye = y2; } else { x = x2; y = y2; ye = y1; }
            Draw(x, y, c, col);
            for (i = 0; y<ye; i++) {
                y = y + 1;
                if (py <= 0) py = py + 2 * dx1;
                else {
                    if ((dx<0 && dy<0) || (dx>0 && dy>0)) x = x + 1; else x = x - 1;
                    py = py + 2 * (dx1 - dy1);
                }
                Draw(x, y, c, col);
            }
        }
    }

    ~olcConsoleGameEngine()
    {
        delete[] m_bufScreen;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &m_OriginalTermios);
        std::cout << "\033[?25h\033[2J\033[H"; 
    }

    void Start()
    {
        m_bAtomActive = true;
        std::thread t = std::thread(&olcConsoleGameEngine::GameThread, this);
        t.join();
    }

    int ScreenWidth() { return m_nScreenWidth; }
    int ScreenHeight() { return m_nScreenHeight; }

private:
    void GameThread()
    {
        if (!OnUserCreate()) m_bAtomActive = false;
        auto tp1 = std::chrono::system_clock::now();
        auto tp2 = std::chrono::system_clock::now();

        while (m_bAtomActive)
        {
            tp2 = std::chrono::system_clock::now();
            std::chrono::duration<float> elapsedTime = tp2 - tp1;
            tp1 = tp2;
            float fElapsedTime = elapsedTime.count();

            char c;
            int nread = read(STDIN_FILENO, &c, 1);
            
            for (int i = 0; i < 256; i++) {
                m_keys[i].bPressed = false;
                m_keys[i].bReleased = false;
            }

            if (nread > 0) {
                char upperC = toupper(c);
                
                // NEW FIX: If we switch keys, instantly release the old one!
                if (m_keyOldState[0] != 0 && m_keyOldState[0] != upperC) {
                    m_keys[m_keyOldState[0]].bReleased = true;
                    m_keys[m_keyOldState[0]].bHeld = false;
                }
                
                m_keys[upperC].bPressed = !m_keys[upperC].bHeld;
                m_keys[upperC].bHeld = true;
                m_keyOldState[0] = upperC; 
                m_fKeyTimer = 0.0f; 
            } else {
                m_fKeyTimer += fElapsedTime;
                if(m_fKeyTimer > 0.1f && m_keyOldState[0] != 0) {
                     m_keys[m_keyOldState[0]].bReleased = true;
                     m_keys[m_keyOldState[0]].bHeld = false;
                     m_keyOldState[0] = 0;
                }
            }

            if (!OnUserUpdate(fElapsedTime)) m_bAtomActive = false;

            std::string frameBuffer = "\033[H"; 
            short lastColor = -1;

            for (int y = 0; y < m_nScreenHeight; y++) {
                for (int x = 0; x < m_nScreenWidth; x++) {
                    short color = m_bufScreen[y * m_nScreenWidth + x].Attributes;
                    if (color != lastColor) {
                        frameBuffer += "\033[" + std::to_string(color) + "m";
                        lastColor = color;
                    }
                    wchar_t glyph = m_bufScreen[y * m_nScreenWidth + x].Char;
                    if(glyph == PIXEL_SOLID) frameBuffer += "█";
                    else if(glyph == PIXEL_THREEQUARTERS) frameBuffer += "▓";
                    else if(glyph == PIXEL_HALF) frameBuffer += "▒";
                    else if(glyph == PIXEL_QUARTER) frameBuffer += "░";
                    else frameBuffer += (char)glyph;
                }
                frameBuffer += "\r\n";
            }
            std::cout << frameBuffer << std::flush;
        }
        if (OnUserDestroy()) m_cvGameFinished.notify_one();
        else m_bAtomActive = true;
    }

public:
    virtual bool OnUserCreate() = 0;
    virtual bool OnUserUpdate(float fElapsedTime) = 0;
    virtual bool OnUserDestroy() { return true; }

protected:
    struct sKeyState { bool bPressed; bool bReleased; bool bHeld; } m_keys[256];
    int m_nScreenWidth; int m_nScreenHeight;
    CHAR_INFO *m_bufScreen; std::wstring m_sAppName;
    short m_keyOldState[256] = { 0 }; short m_keyNewState[256] = { 0 };
    float m_fKeyTimer = 0.0f; 
    struct termios m_OriginalTermios;
    static std::atomic<bool> m_bAtomActive;
    static std::condition_variable m_cvGameFinished;
    static std::mutex m_muxGame;
};

std::atomic<bool> olcConsoleGameEngine::m_bAtomActive(false);
std::condition_variable olcConsoleGameEngine::m_cvGameFinished;
std::mutex olcConsoleGameEngine::m_muxGame;