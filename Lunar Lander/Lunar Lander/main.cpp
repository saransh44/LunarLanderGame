#define GL_SILENCE_DEPRECATION //For silencing pesky notifications on Mac
#define TILE_COUNT 34

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <vector>
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "Entity.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

SDL_Window* displayWindow;
bool gameIsRunning = true;

ShaderProgram textured;
glm::mat4 viewMatrix, modelMatrix, projectionMatrix;

GLuint textID;
GLuint tileID;
GLuint pikachuID; 
GLuint pokeballID;

ShaderProgram unTextured;

Entity tiles[TILE_COUNT];
Entity pikachu(PLAYER);
Entity goal(COIN);

bool ended = false;

using namespace std;

GLuint LoadTexture(const char* filePath) {
    int w, h, n;
    unsigned char* image = stbi_load(filePath, &w, &h, &n, STBI_rgb_alpha);

    if (image == NULL) {
        std::cout << "Unable to load image. Make sure the path is correct\n";
        assert(false);
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    stbi_image_free(image);
    return textureID;
}

void DrawText(ShaderProgram* program, GLuint fontTextureID, std::string text, float size, float spacing, glm::vec3 position)
{
    float width = 1.0f / 16.0f;
    float height = 1.0f / 16.0f;

    std::vector<float> vertices;
    std::vector<float> texCoords;

    for (int i = 0; i < text.size(); i++) {
        int index = (int)text[i];

        float u = (float)(index % 16) / 16.0f;
        float v = (float)(index / 16) / 16.0f;


        texCoords.insert(texCoords.end(), { u, v + height, u + width, v + height, u + width, v, u, v + height, u + width, v, u, v });

        float offset = (size + spacing) * i;
        vertices.insert(vertices.end(), { offset + (-0.5f * size), (-0.5f * size),
                                        offset + (0.5f * size), (-0.5f * size),
                                        offset + (0.5f * size), (0.5f * size),
                                        offset + (-0.5f * size), (-0.5f * size),
                                        offset + (0.5f * size), (0.5f * size),
                                        offset + (-0.5f * size), (0.5f * size) });
    }

    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, position);
    program->SetModelMatrix(modelMatrix);
    glBindTexture(GL_TEXTURE_2D, fontTextureID);


    glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices.data());
    glEnableVertexAttribArray(program->positionAttribute);

    glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords.data());
    glEnableVertexAttribArray(program->texCoordAttribute);

    glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 2.0f);

    glDisableVertexAttribArray(program->positionAttribute);
    glDisableVertexAttribArray(program->texCoordAttribute);
}

int borderTileCount = -1;

//borders
float bottomBorder;
float leftBorder;
float rightBorder;

void Initialize() {
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("Lunar Lander", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(0, 0, 640, 480);


    viewMatrix = glm::mat4(1.0f);
    //modelMatrix = glm::mat4(1.0f); //modelMatrix is used elegantly in Entity.cpp
    projectionMatrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    unTextured.Load("shaders/vertex.glsl", "shaders/fragment.glsl");

    unTextured.SetProjectionMatrix(projectionMatrix);
    unTextured.SetViewMatrix(viewMatrix);
    unTextured.SetColor(1.0f, 1.0f, 1.0f, 1.0f);
    glUseProgram(unTextured.programID);

    //textured.SetModelMatrix(modelMatrix);

    textured.Load("shaders/vertex_textured.glsl", "shaders/fragment_textured.glsl");
    textID = LoadTexture("font1.png");
    tileID = LoadTexture("tile.png");
    pikachu.textureID = LoadTexture("pikachu.png");
    goal.textureID = LoadTexture("pokeball.png");

    goal.position = glm::vec3(2.0f, -2.0f, 0.0);

    pikachu.position = glm::vec3(0, 4, 0);
    pikachu.acceleration = glm::vec3(0, -0.1, 0); 
    pikachu.width = 0.5;
    pikachu.height = 0.5;
    //as much as we would like to use -9.81 its too much movement in a short time for this game
    
    for (bottomBorder = -5; bottomBorder < 6; bottomBorder++) {
        borderTileCount++;

        tiles[borderTileCount].textureID = tileID;
        tiles[borderTileCount].position = glm::vec3(bottomBorder, -3.2f, 0);
        tiles[borderTileCount].entityType = TILE;
    }
    cout << borderTileCount << endl; //debugging

    for (leftBorder = -2.2; leftBorder < 3.2; leftBorder++) {
        borderTileCount++;

        tiles[borderTileCount].textureID = tileID;
        tiles[borderTileCount].position = glm::vec3(-4.5, leftBorder, 0);
        tiles[borderTileCount].entityType = TILE;
    }
    cout << borderTileCount << endl; //debugging

    for (rightBorder = -2.2; rightBorder < 3.2; rightBorder++) {
        borderTileCount++;

        tiles[borderTileCount].textureID = tileID;
        tiles[borderTileCount].position = glm::vec3(4.5, rightBorder, 0);
        tiles[borderTileCount].entityType = TILE;
    }
    cout << borderTileCount << endl; //debugging

    tiles[23].textureID = tileID;
    tiles[23].position = glm::vec3(-3.5, 1.8, 0);
    tiles[23].entityType = TILE;

    tiles[24].textureID = tileID;
    tiles[24].position = glm::vec3(-1.2, 1.8, 0);
    tiles[24].entityType = TILE;

    tiles[25].textureID = tileID;
    tiles[25].position = glm::vec3(-0.2, 1.8, 0);
    tiles[25].entityType = TILE;

    tiles[26].textureID = tileID;
    tiles[26].position = glm::vec3(2.4, 1.8, 0);
    tiles[26].entityType = TILE;

    int moreTiles = 26;
    for (int firstBarrierX = 4; firstBarrierX >= -1; firstBarrierX--) {
        moreTiles++;

        tiles[moreTiles].textureID = tileID;
        tiles[moreTiles].position = glm::vec3(firstBarrierX, -0.2, 0);
        tiles[moreTiles].entityType = TILE;
    }

    cout << moreTiles << endl; //debugging

    tiles[33] = goal;

    textured.SetProjectionMatrix(projectionMatrix);
    textured.SetViewMatrix(viewMatrix);
    textured.SetColor(1.0f, 1.0f, 1.0f, 1.0f);

    glUseProgram(textured.programID);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
}


#define FIXED_TIMESTEP 0.0166666f
float lastTicks = 0;
float accumulator = 0.0f;

void Update() {

    float ticks = (float)SDL_GetTicks() / 1000.0f;
    float deltaTime = ticks - lastTicks;
    lastTicks = ticks;

    deltaTime += accumulator;
    if (deltaTime < FIXED_TIMESTEP) {
        accumulator = deltaTime;
        return;
    }

    while (deltaTime >= FIXED_TIMESTEP) {
        // Update. Notice it's FIXED_TIMESTEP. Not deltaTime

        if (!pikachu.collisionDetected && !pikachu.winnerWinnerChickenDinner)
            pikachu.Update(FIXED_TIMESTEP, tiles, TILE_COUNT);

        deltaTime -= FIXED_TIMESTEP;
    }

    accumulator = deltaTime;    
}


glm::vec3 tileSizing = glm::vec3(1.0f, 1.0f, 0.0f);
glm::vec3 pokeballSizing = glm::vec3(1.0f, 1.0f, 0.0f); 
glm::vec3 pikSizing = glm::vec3(0.5f, 0.5f, 0.0f);

void Render() {
    glClear(GL_COLOR_BUFFER_BIT);
    
    pikachu.Render(&textured, pikSizing);
    goal.Render(&textured, pokeballSizing);

    for (int i = 0; i <= 33; i++) {
        tiles[i].Render(&textured, tileSizing);
    }
    if (pikachu.collisionDetected) {
        if (pikachu.winnerWinnerChickenDinner) {
            DrawText(&textured, textID, "Mission Successful!", 0.75, -0.5, glm::vec3(-1.5, 0, 0));
            //gameIsRunning = false; //commented out because if not, it immediatly goes to quit function so you can't see end screen.

        }
        else {
            DrawText(&textured, textID, "Mission Failed!", 0.75, -0.5, glm::vec3(-1.0, 0, 0));
            //gameIsRunning = false;
        }
    }
    SDL_GL_SwapWindow(displayWindow);
}

void ProcessInput() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            gameIsRunning = false;
            break;
        }
        if (!pikachu.boostUsed) {
            switch (event.key.keysym.sym) {
            case SDLK_RIGHT:
                pikachu.acceleration.x = 3.0;
                pikachu.boostUsed = true;

                break;
            case SDLK_LEFT:
                pikachu.acceleration.x = -3.0;
                pikachu.boostUsed = true;

                break;
            }

        }
    }
    
    // Check for pressed/held keys below
    const Uint8* keys = SDL_GetKeyboardState(NULL);

    if (keys[SDL_SCANCODE_A])
    {
        pikachu.acceleration.x = -0.5f;
    }
    else if (keys[SDL_SCANCODE_D])
    {
        pikachu.acceleration.x = 0.5f;
    }
    //if (pikachu.boost) {}
}

void Shutdown() {
    SDL_Quit();
}

int main(int argc, char* argv[]) {
    Initialize();
    
    while (gameIsRunning) {
        ProcessInput();
        Update();
        Render();
        }
    
    Shutdown();
    return 0;
}