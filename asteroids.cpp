#include <SDL2/SDL.h>
#include <vector>
#include <numeric>
#include <cmath>
#include <cstdlib>
#include <map>
#include <iostream>
#include <fstream>

const int SCREEN_HEIGHT = 360;
const int SCREEN_WIDTH = 640;
const float PI = 3.1415926536;
const float PLAYER_LENGTH = 25;
const float PLAYER_WIDTH = 20;
const float MAX_SPEED = 1.5;
const float RESISTANCE = 0.008;
const int COOLDOWN = 10;
const int MAX_ASTEROID_SIZE = 30;
const int SPAWN_GAP = 200;

int nextSpawn = SPAWN_GAP;
float scoreMultiplier = 1;
int fired = 0;
SDL_KeyCode keys[] = {SDLK_UP, SDLK_DOWN, SDLK_LEFT, SDLK_RIGHT};
std::map<SDL_Keycode, bool> isPressed;

struct Vector2
{
    float x, y;
};

struct Bullet
{
    Vector2 position;
    float angle, speed;
    int timeToLive;
};

struct Asteroid
{
    Vector2 position;
    float faceAngle, speed, size, angularSpeed, moveAngle;
    std::vector<Vector2> points;
};

struct Player
{
    int score;
    Vector2 position;
    float angle, speed;
    Vector2 points[3];
};

struct Screen
{
    SDL_Event e;
    SDL_Window *window;
    SDL_Renderer *renderer;
    std::vector<SDL_FPoint> points;
};

std::vector<Bullet> bullets;
std::vector<Asteroid *> asteroids;
void showStartMenu(Screen *screen, Player *player);

Player *createPlayer()
{
    Player *player = (Player *)malloc(sizeof(Player));
    player->score = 0;
    player->position = {200, 200};
    player->angle = 0;
    player->speed = 0;

    float baseHalf = PLAYER_WIDTH / 2;
    player->points[0] = {2 * PLAYER_LENGTH / 3, 0};
    player->points[1] = {-PLAYER_LENGTH / 3, PLAYER_WIDTH / 2};
    player->points[2] = {-PLAYER_LENGTH / 3, -PLAYER_WIDTH / 2};

    return player;
}

Screen *createScreen()
{
    SDL_Init(SDL_INIT_VIDEO);
    Screen *screen = (Screen *)malloc(sizeof(Screen));
    screen->window = SDL_CreateWindow("Asteroids", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2, 0);
    screen->renderer = SDL_CreateRenderer(screen->window, -1, SDL_RENDERER_ACCELERATED);
    SDL_RenderSetScale(screen->renderer, 2, 2);
    return screen;
}

bool isValidSpawn(Player *player, float x, float y, float size)
{
    if (x < size || x > SCREEN_WIDTH - size || y < size || y > SCREEN_HEIGHT - size)
    {
        return false;
    }

    float dx = player->position.x - x;
    float dy = player->position.y - y;
    if (std::sqrt((dx * dx) + (dy * dy)) < size + 5)
    {
        return false;
    }
    return true;
}

Asteroid *createAsteroid(Player *player, float x = -1, float y = -1, float size = MAX_ASTEROID_SIZE)
{
    Asteroid *asteroid = new Asteroid();
    asteroid->size = size;
    if (x == -1 || y == -1)
    {
        do
        {
            x = rand() % SCREEN_WIDTH;
            y = rand() % SCREEN_HEIGHT;
        } while (!isValidSpawn(player, x, y, size));
    }
    asteroid->position.x = x;
    asteroid->position.y = y;
    asteroid->speed = (rand() % 10 + 1) * 0.1;
    asteroid->faceAngle = PI / 2;
    asteroid->moveAngle = (2 * PI) * ((rand()) % 100) * 0.01;
    float angle = 0;
    for (int i = 0; i < 10; i++)
    {
        Vector2 point;
        float randomFactor = 0.80 + (rand() % 40) / 100.0;
        angle += (2 * PI) * 0.1 * ((randomFactor > 1) ? 1 : randomFactor);
        point.x = std::cos(angle) * asteroid->size * randomFactor;
        point.y = std::sin(angle) * asteroid->size * randomFactor;
        asteroid->points.push_back(point);
    }
    asteroid->angularSpeed = (rand() % 10) * 0.002;
    return asteroid;
}

void reset(Screen *screen, Player *player)
{
    screen->points.clear();
    asteroids.clear();
    bullets.clear();
    player->angle = 0;
    player->score = 0;
    player->speed = 0;
    player->position = {200, 200};
    for (int i = 0; i < 5; i++)
    {
        asteroids.push_back(createAsteroid(player));
    }
    showStartMenu(screen, player);
}

void drawPoint(Screen *screen, float x, float y)
{
    SDL_FPoint point;
    point.x = x;
    point.y = y;
    screen->points.push_back(point);
}

void drawLine(Screen *screen, float x1, float y1, float x2, float y2)
{
    float dx = x2 - x1;
    float dy = y2 - y1;
    float length = std::sqrt((dx * dx) + (dy * dy));
    float angle = std::atan2(dy, dx);

    for (int i = 0; i < length; i++)
    {
        drawPoint(screen, x1 + (std::cos(angle) * i), y1 + (std::sin(angle) * i));
    }
}

void drawPlayer(Screen *screen, Player *player)
{
    Vector2 transformed[3];

    for (int i = 0; i < 3; i++)
    {
        transformed[i].x = player->position.x + (player->points[i].x * std::cos(player->angle) - player->points[i].y * std::sin(player->angle));
        transformed[i].y = player->position.y + (player->points[i].x * std::sin(player->angle) + player->points[i].y * std::cos(player->angle));
    }

    drawLine(screen, transformed[0].x, transformed[0].y, transformed[1].x, transformed[1].y);
    drawLine(screen, transformed[1].x, transformed[1].y, transformed[2].x, transformed[2].y);
    drawLine(screen, transformed[2].x, transformed[2].y, transformed[0].x, transformed[0].y);
}

void drawBullets(Screen *screen, Player *player)
{
    bool destroyed = false;

    for (size_t i = 0; i < bullets.size(); i++)
    {
        if (destroyed)
        {
            destroyed = false;
            continue;
        }

        Bullet &b = bullets[i];
        b.position.x += b.speed * std::cos(b.angle);
        b.position.y += b.speed * std::sin(b.angle);

        if ((b.position.x <= 0 || b.position.x >= SCREEN_WIDTH || b.position.y <= 0 || b.position.y >= SCREEN_HEIGHT))
            b.timeToLive++;
        if (b.position.x > SCREEN_WIDTH)
            b.position.x -= SCREEN_WIDTH;
        else if (b.position.x < 0)
            b.position.x += SCREEN_WIDTH;

        if (b.position.y > SCREEN_HEIGHT)
            b.position.y -= SCREEN_HEIGHT;
        else if (b.position.y < 0)
            b.position.y += SCREEN_HEIGHT;

        if (b.timeToLive > 1)
        {
            bullets.erase(bullets.begin() + i);
            i--;
            continue;
        }

        drawLine(
            screen,
            b.position.x,
            b.position.y,
            b.position.x + 10 * std::cos(b.angle),
            b.position.y + 10 * std::sin(b.angle) //
        );

        for (size_t j = 0; j < asteroids.size(); j++)
        {
            float dx = b.position.x - asteroids[j]->position.x;
            float dy = b.position.y - asteroids[j]->position.y;

            if ((std::sqrt((dx * dx) + (dy * dy))) <= asteroids[j]->size)
            {
                if (asteroids[j]->size > 10)
                {
                    for (int k = 0; k < 2; k++)
                    {
                        asteroids.push_back(createAsteroid(player, asteroids[j]->position.x, asteroids[j]->position.y, asteroids[j]->size * 0.5));
                    }
                }
                player->score += asteroids[j]->size * scoreMultiplier;
                scoreMultiplier += 0.01;

                delete asteroids[j];
                asteroids.erase(asteroids.begin() + j);
                j--;

                bullets.erase(bullets.begin() + i);
                i--;
                destroyed = true;
                break;
            }
        }
    }
}

void drawAsteroids(Screen *screen, Player *player)
{
    std::vector<int> remove;
    for (size_t i = 0; i < asteroids.size(); i++)
    {
        Asteroid *a = asteroids[i];

        float dx = asteroids[i]->position.x - player->position.x;
        float dy = asteroids[i]->position.y - player->position.y;

        if ((std::sqrt((dx * dx) + (dy * dy))) <= (asteroids[i]->size * 1.1))
        {
            std::cout << "SCORE : " << player->score << "\n";
            reset(screen, player);
        }

        std::vector<Vector2> transformed(a->points.size());
        for (size_t j = 0; j < a->points.size(); j++)
        {
            transformed[j].x = a->position.x + (a->points[j].x * std::cos(a->faceAngle) - a->points[j].y * std::sin(a->faceAngle));
            transformed[j].y = a->position.y + (a->points[j].x * std::sin(a->faceAngle) + a->points[j].y * std::cos(a->faceAngle));
        }

        for (size_t j = 0; j < a->points.size(); j++)
        {
            size_t next = (j + 1) % a->points.size();
            drawLine(
                screen,
                transformed[j].x,
                transformed[j].y,
                transformed[next].x,
                transformed[next].y //
            );
        }
        a->faceAngle += a->angularSpeed;
        if (a->faceAngle >= 2 * PI)
        {
            a->faceAngle -= 2 * PI;
        }

        a->position.x += a->speed * std::cos(a->moveAngle);
        a->position.y += a->speed * std::sin(a->moveAngle);
        if (a->position.x < -20)
        {
            a->position.x += SCREEN_WIDTH;
        }
        if (a->position.x >= SCREEN_WIDTH + 20)
        {
            a->position.x -= SCREEN_WIDTH;
        }
        if (a->position.y < -20)
        {
            a->position.y += SCREEN_HEIGHT;
        }
        if (a->position.y >= SCREEN_HEIGHT + 20)
        {
            a->position.y -= SCREEN_HEIGHT;
        }
    }

    for (int i = remove.size() - 1; i >= 0; i--)
    {
        delete asteroids[remove[i]];
        asteroids.erase(asteroids.begin() + remove[i]);
    }
}

void drawImage(Screen *screen, float x, float y)
{
    const std::string filename = "TitleScreen.bin";
    const int width = 862;
    const int height = 303;
    std::ifstream file(filename, std::ios::binary);
    std::vector<unsigned char> pixels(width * height);
    file.read(reinterpret_cast<char *>(pixels.data()), width * height);
    float cury = y;
    for (int i = 0; i < height; i++)
    {
        float curx = x;
        for (int j = 0; j < width; j++)
        {
            if (pixels[i * width + j] > 90)
            {
                drawPoint(screen, curx, cury);
            }
            curx += 0.5;
        }
        cury += 0.5;
    }
}

void drawGame(Screen *screen, Player *player)
{
    drawPlayer(screen, player);
    drawBullets(screen, player);
    drawAsteroids(screen, player);
}

void clear(Screen *screen)
{
    SDL_SetRenderDrawColor(screen->renderer, 0, 0, 0, 255);
    SDL_RenderClear(screen->renderer);
    screen->points.clear();
}

void show(Screen *screen)
{
    SDL_SetRenderDrawColor(screen->renderer, 255, 255, 255, 255);
    for (SDL_FPoint point : screen->points)
    {
        SDL_RenderDrawPointF(screen->renderer, point.x, point.y);
    }
    SDL_RenderPresent(screen->renderer);
}

void freeScreen(Screen *screen)
{
    SDL_DestroyRenderer(screen->renderer);
    SDL_DestroyWindow(screen->window);
    free(screen);
}

void handleInput(Screen *screen, Player *player)
{
    nextSpawn--;
    if (nextSpawn <= 0)
    {
        nextSpawn = SPAWN_GAP;
        asteroids.push_back(createAsteroid(player));
    }
    while (SDL_PollEvent(&screen->e))
    {
        if (screen->e.type == SDL_QUIT)
        {
            freeScreen(screen);
            SDL_Quit();
            exit(0);
        }
        else if (screen->e.type == SDL_KEYDOWN)
        {
            isPressed[screen->e.key.keysym.sym] = true;
        }
        else if (screen->e.type == SDL_KEYUP)
        {
            isPressed[screen->e.key.keysym.sym] = false;
        }
    }

    if (isPressed[SDLK_UP] && player->speed < MAX_SPEED)
    {
        player->speed += 0.05f;
    }
    if (isPressed[SDLK_DOWN] && player->speed > 0)
    {
        player->speed -= 0.1f;
    }
    if (isPressed[SDLK_LEFT])
    {
        player->angle -= 0.03f;
    }
    if (isPressed[SDLK_RIGHT])
    {
        player->angle += 0.03f;
    }
    if (isPressed[SDLK_SPACE] && fired <= 0)
    {
        Bullet bullet;
        bullet.position.x = player->position.x;
        bullet.position.y = player->position.y;
        bullet.angle = player->angle;
        bullet.speed = 7;
        bullet.timeToLive = 0;
        bullets.push_back(bullet);
        fired = COOLDOWN;
    }
    else
    {
        fired--;
    }

    player->position.x += player->speed * std::cos(player->angle);
    player->position.y += player->speed * std::sin(player->angle);

    if (player->position.x < -10)
    {
        player->position.x += SCREEN_WIDTH;
    }
    if (player->position.x >= SCREEN_WIDTH + 10)
    {
        player->position.x -= SCREEN_WIDTH;
    }
    if (player->position.y < -10)
    {
        player->position.y += SCREEN_HEIGHT;
    }
    if (player->position.y >= SCREEN_HEIGHT + 10)
    {
        player->position.y -= SCREEN_HEIGHT;
    }
    if (player->speed > 0)
    {
        player->speed -= RESISTANCE;
    }
}

void showStartMenu(Screen *screen, Player *player)
{
    player->position.x = 1000;
    player->position.y = 1000;
    for (int i = 0; i < 10; i++)
    {
        asteroids.push_back(createAsteroid(player, -1, -1, rand() % 20 + 10));
    }

    while (true)
    {
        clear(screen);
        drawAsteroids(screen, player);
        nextSpawn++;
        drawImage(screen, 100, 100);
        show(screen);

        while (SDL_PollEvent(&screen->e))
        {
            if (screen->e.type == SDL_QUIT)
            {
                freeScreen(screen);
                SDL_Quit();
                exit(0);
            }
            else if (screen->e.type == SDL_KEYDOWN && screen->e.key.keysym.sym == SDLK_SPACE)
            {
                for (Asteroid *a : asteroids)
                {
                    delete a;
                }
                asteroids.clear();
                player->position.x = 200;
                player->position.y = 200;
                return;
            }
        }

        SDL_Delay(10);
    }
}

int main(int argc, char *argv[])
{
    Screen *screen = createScreen();
    Player *player = createPlayer();

    showStartMenu(screen, player);

    for (int i = 0; i < 5; i++)
    {
        asteroids.push_back(createAsteroid(player));
    }

    for (SDL_Keycode code : keys)
    {
        isPressed[code] = false;
    }

    while (true)
    {
        clear(screen);
        drawGame(screen, player);
        show(screen);
        handleInput(screen, player);
        SDL_Delay(5);
    }
    return 0;
}
