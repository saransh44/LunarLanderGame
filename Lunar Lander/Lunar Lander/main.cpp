#define GL_SILENCE_DEPRECATION //For silencing pesky notifications on Mac
#define PLATFORM_COUNT 30

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

#include <ctime> 

SDL_Window* displayWindow;
bool gameIsRunning = true;

ShaderProgram textured;
glm::mat4 viewMatrix, modelMatrix, projectionMatrix;

GLuint textID;
GLuint tileID;

ShaderProgram unTextured;

struct GameState {
    Entity lander;
    Entity platforms[PLATFORM_COUNT];
    bool ended = false;
    //add something more to make GameState struct more unique if need be
};

Entity lander;
Entity platforms[PLATFORM_COUNT];
bool ended = false;
GameState state;

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

int ID = 0;
//borders
float bottom_x = -5;
float left_y = -2.25;
float right_y = -2.25;

void Initialize() {
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("Lunar Lander", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(0, 0, 640, 480);

    //plain.Load("shaders/vertex_textured.glsl", "shaders/fragment_textured.glsl");

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


    //bottom border
    while (bottom_x <= 5) {
        platforms[ID].textureID = tileID;
        platforms[ID].position = glm::vec3(bottom_x, -3.25f, 0);
        ID++;
        bottom_x += 1;
    }

    //left border
    while (left_y <= 3.75) {
        platforms[ID].textureID = tileID;
        platforms[ID].position = glm::vec3(-5, left_y, 0);
        ID++;
        left_y += 1;
    }

    //right border
    while (right_y <= 3.75) {
        platforms[ID].textureID = tileID;
        platforms[ID].position = glm::vec3(5, right_y, 0);
        ID++;
        right_y += 1;
    }

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
        state.lander.Update(FIXED_TIMESTEP, state.platforms, PLATFORM_COUNT);

        deltaTime -= FIXED_TIMESTEP;
    }

    accumulator = deltaTime;

}
glm::vec3 tileSizing = glm::vec3(1.0f, 1.0f, 1.0f);

void Render() {
    glClear(GL_COLOR_BUFFER_BIT);
    for (int i = 0; i < PLATFORM_COUNT; i++) {
        platforms[i].Render(&textured, tileSizing);
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

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_SPACE:
                // Some sort of action
                break;

            }
            break;
        }
    }

    // Check for pressed/held keys below
    const Uint8* keys = SDL_GetKeyboardState(NULL);
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