# Terminal Asteroids: Custom POSIX C++ Game  

A completely terminal-based implementation of the classic arcade game *Asteroids*, powered by a custom-built, cross-platform (macOS/Linux) rendering and input engine written in pure C++. 

Instead of relying on heavyweight graphical libraries like OpenGL or SFML, this project bypasses standard rendering pipelines to interact directly with the POSIX API and ANSI escape codes. The result is a smooth, 60+ FPS gaming experience rendered entirely within the standard output (`stdout`).

# [Asteroids Gameplay]
![recording](https://github.com/user-attachments/assets/ea8e905d-4e79-4d87-a961-dbcfccdded9b)

## 🛠️ Technical Highlights

This project serves as a practical exploration of low-level systems programming, custom game engine architecture, and vector mathematics.

* **Custom Rendering Engine:** Utilizes batched ANSI escape sequences and a double-buffered `CHAR_INFO` array to draw wireframe vectors, particles, and UI elements directly to the terminal without screen tearing.
* **Low-Level I/O Interception:** Leverages `<termios.h>` and `<unistd.h>` to force the terminal into raw, non-blocking mode. Implemented a custom input-smoothing algorithm to bypass native macOS keyboard repeat delays, enabling fluid, asynchronous multi-key input.
* **Applied 2D Physics:** Engineered a framerate-independent physics system handling trigonometric thrust vectors, inertia, momentum drag (friction), and toroidal (wrap-around) space mapping.
* **Dynamic Memory Management:** Strictly utilizes the C++ Standard Template Library (STL). Employs the **Erase-Remove idiom** to safely and efficiently manage the lifecycle of dynamically spawned entities (bullets, particles, and asteroid fragments) without memory leaks.

## 🎮 How to Play

### Controls
* **[ W ]** - Main Engine Thrust
* **[ A ] / [ D ]** - Rotate Ship Left / Right
* **[ SPACE ]** - Fire Weapon

### Gameplay
Destroy all the asteroids on the screen to advance to the next level. 
* Large asteroids will break into smaller, faster fragments when shot.
* Avoid crashing! The ship features toroidal wrap-around physics, meaning flying off one side of the screen will wrap you to the other.
* The game uses realistic space inertia—your ship will continue to drift after releasing the thrust until friction gradually brings it to a halt.

## 💻 Installation & Compilation

This engine is designed to run on **macOS and Linux** systems. No external libraries are required.

1. **Clone the repository:**
   ```bash
   git clone [https://github.com/YourUsername/Terminal-Asteroids.git](https://github.com/YourUsername/Terminal-Asteroids.git)
   cd Terminal-Asteroids
   ```

This project is a macOS/POSIX port and heavy modification of the original olcConsoleGameEngine created by Javidx9 (OneLoneCoder).


