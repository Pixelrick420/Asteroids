# SDL2 Asteroids Game

A classic Asteroids arcade game implemented in C++ using SDL2.

## Preview
![image](https://github.com/user-attachments/assets/6e5c5842-555a-4388-a456-a5da09e7653e) ![image](https://github.com/user-attachments/assets/ee5cb740-0165-4877-a116-c20c29dec564)

## Features

- Fully functional Asteroids gameplay
- Infinite wraparound screen with bounds checking.
- Start menu with animated background.
- Collision detection logic to determine game over condition.
- Transformation and rotating matrices to handle player input and rotation.

## Prerequisites

- SDL2 library
- C++ compiler with C++11 support

## Build Instructions
### Windows
Clone the repository using :
```
git clone https://github.com/Pixelrick420/Asteroids.git
```
Then move into the cloned repository :
```
cd Asteroids
```
Then compile using the command :
```
g++ -I <path to SDL include> -L <path to SDL lib> -o asteroids asteroids.cpp -l mingw32 -l SDL2main -l SDL2
```
If you are using another compiler like clang, use appropriate command to link the files.
Finally, run the program using:
```
./asteroids
```

Alternatively, you can download and unzip asteroids.zip and compile using :
```g++ -Isrc/Include -Lsrc/lib -o asteroids asteroids.cpp -lmingw32 -lSDL2main -lSDL2```
Then run the program using:
```
./asteroids
```

## Controls
- **Up Arrow**: Accelerate
- **Down Arrow**: Decelerate
- **Left/Right Arrows**: Rotate ship
- **Space**: Fire bullet / Start game

## Gameplay
- Destroy asteroids to increase score.
- Collision with asteroid ends the game and resets to start menu.
- Destroyed asteroid may split into smaller asteroids.
- Score multiplier increases with each asteroid destroyed.

## Dependencies
- SDL2 library

## License
Open source. Feel free to modify and distribute.
