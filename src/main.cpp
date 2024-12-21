// clang-format off
#include "common.h"

#include <iostream>
#include <map>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <glad/glad.h>
#include <stb_image/stb_image.cpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H

// OpenGL Utilities
#include "Shader.h"
// clang-format on

bool32 TEXT_DEBUGGING_ENABLED = 0;
bool32 VSYNC_ENABLED = 1;
real32 TARGET_SCREEN_FPS = 58.9f;

int32 LOGICAL_WIDTH = 1280;
int32 LOGICAL_HEIGHT = 720;
real32 ABSOLUTE_ASPECT_RATIO = (real32)LOGICAL_WIDTH / (real32)LOGICAL_HEIGHT;
/*
Here are a few integer grid sizes that would fit a 16:9 aspect ratio perfectly:

16x9 Grid - Each cell would be 80x80 pixels.
32x18 Grid - Each cell would be 40x40 pixels.
64x36 Grid - Each cell would be 20x20 pixels.
80x45 Grid - Each cell would be 16x16 pixels.
160x90 Grid - Each cell would be 8x8 pixels.
320x180 Grid - Each cell would be 4x4 pixels.
640x360 Grid - Each cell would be 2x2 pixels.
*/
uint32 GRID_BLOCK_SIZE = 20;
uint32 X_GRIDS = LOGICAL_WIDTH / GRID_BLOCK_SIZE;  // Should exactly divide into logical width
uint32 Y_GRIDS = (int32)(X_GRIDS / ABSOLUTE_ASPECT_RATIO);

int32 global_window_width = LOGICAL_WIDTH;
int32 global_window_height = LOGICAL_HEIGHT;
int32 global_pixel_width, global_pixel_height;

real32 SIMULATION_FPS = 100;
real32 SIMULATION_DELTA_TIME_S = 1.f / SIMULATION_FPS;

real32 TARGET_TIME_PER_FRAME_S = 1.f / (real32)TARGET_SCREEN_FPS;
real32 TARGET_TIME_PER_FRAME_MS = TARGET_TIME_PER_FRAME_S * 1000.0f;

int32 global_running = 1;
SDL_Window* global_window;
SDL_Renderer* global_renderer;

real32 global_text_dpi_scale_factor = 1.f;

bool32 global_display_debug_info;

real32 global_debug_counter;

struct Master_Timer
{
    // High-res timer stuff
    Uint64 last_frame_counter;
    Uint64 COUNTER_FREQUENCY;

    real32 time_elapsed_for_work__seconds;            // How much time was needed for simulation stuff
    real32 time_elapsed_for_writing_buffer__seconds;  // How much time was needed for writing to render buffer?
    real32 time_elapsed_for_render__seconds;          // How much time was needed for rendering?
    real32 time_elapsed_for_sleep__seconds;           // How much time was needed for sleeping?
    real32 total_frame_time_elapsed__seconds;         // How long did the whole dang frame take?
    real64 physics_simulation_elapsed_time__seconds;  // This is the main counter for time. Everything will rely on what
                                                      // the physics sees
};

void set_dpi()
{
    real32 dpi = (real32)global_pixel_width / (real32)global_window_width;
    global_text_dpi_scale_factor = dpi;
    // printf("%.4f\n", global_text_dpi_scale_factor);
}

#ifdef _WIN32
uint64 global_last_cycle_count;
uint64 global_cycles_elapsed_before_render;
uint64 global_cycles_elapsed_after_render;
uint64 global_total_cycles_elapsed;

uint64 global_LAST_total_cycles_elapsed = global_total_cycles_elapsed;
uint64 global_LAST_cycles_elapsed_before_render = global_cycles_elapsed_before_render;
uint64 global_LAST_cycles_elapsed_after_render = global_cycles_elapsed_after_render;
#endif

global_variable const int32 FONT_SCALE_FACTOR = 4;
Shader* global_text_shader;
Shader* global_basic_shader;
Shader* global_grid_shader;

// Function to configure OpenGL for font rendering
void setupTextRenderingState()
{
    glEnable(GL_BLEND);                                 // Enable blending for alpha transparency
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // Set blend function for alpha blending
    glDisable(GL_CULL_FACE);                            // Disable face culling (not needed for flat quads)
    glDisable(GL_DEPTH_TEST);                           // Disable depth testing for text
}

// Function to restore OpenGL state for geometry rendering
void setupGeometryRenderingState()
{
    glDisable(GL_BLEND);  // Disable blending to avoid transparency issues in 3D models
    // glEnable(GL_CULL_FACE); // Enable face culling for proper back-face removal
    // glCullFace(GL_BACK); // Cull back faces
    // glFrontFace(GL_CCW); // Counter-clockwise winding is front-facing
    // glEnable(GL_DEPTH_TEST);
}

GLuint quadVAO, quadVBO, quadEBO;

void setupQuad()
{
    float vertices[] = {
        // clang-format off
        // Positions       // Texture Coords
        -0.5f,  0.5f,      0.0f, 1.0f,  // Top-left
         0.5f,  0.5f,      1.0f, 1.0f,  // Top-right
         0.5f, -0.5f,      1.0f, 0.0f,  // Bottom-right
        -0.5f, -0.5f,      0.0f, 0.0f   // Bottom-left
        // clang-format on
    };

    unsigned int indices[] = {0, 1, 2, 0, 2, 3};

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glGenBuffers(1, &quadEBO);

    glBindVertexArray(quadVAO);

    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Texture Coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

unsigned int global_square_VAO, global_square_VBO, global_square_EBO;

void setup_square_buffers()
{
    real32 vertices[] = {
        // Positions
        -0.5f, -0.5f,  // Bottom-left
         0.5f, -0.5f,  // Bottom-right
         0.5f,  0.5f,  // Top-right
        -0.5f,  0.5f   // Top-left
    };
    
    // Indices for two triangles to form a rectangle
    uint32 indices[] = {
        // clang-format off
        0, 1, 2, // First triangle
        2, 3, 0  // Second triangle
        // clang-format on
    };

    glGenVertexArrays(1, &global_square_VAO);
    glGenBuffers(1, &global_square_VBO);
    glGenBuffers(1, &global_square_EBO);

    glBindVertexArray(global_square_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, global_square_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, global_square_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0); // Unbind for safety
}

/// Holds all state information relevant to a character as loaded using FreeType
struct Character
{
    unsigned int TextureID;  // ID handle of the glyph texture
    glm::ivec2 Size;         // Size of glyph
    glm::ivec2 Bearing;      // Offset from baseline to left/top of glyph
    unsigned int Advance;    // Horizontal offset to advance to next glyph
};

std::map<GLchar, Character> Characters;
unsigned int FONT_VAO, FONT_VBO;

void RenderText(Shader& shader, std::string text, float x, float y, float scale, glm::vec3 color)
{
    setupTextRenderingState();
    shader.use();
    
    glm::mat4 projection = glm::ortho(0.0f, (real32)LOGICAL_WIDTH, 0.0f, (real32)LOGICAL_HEIGHT);
    glUniformMatrix4fv(glGetUniformLocation(global_text_shader->ID, "projection"),
                       1,
                       GL_FALSE,
                       glm::value_ptr(projection));
    // activate corresponding render state
    glUniform3f(glGetUniformLocation(shader.ID, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(FONT_VAO);

    // iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++)
    {
        Character ch = Characters[*c];

        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;
        // update VBO for each character
        float vertices[6][4] = {
            // clang-format off
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }
            // clang-format on
        };
        // render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        // update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, FONT_VBO);
        glBufferSubData(GL_ARRAY_BUFFER,
                        0,
                        sizeof(vertices),
                        vertices);  // be sure to use glBufferSubData and not glBufferData

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (ch.Advance >> 6) * scale;  // bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th
                                         // pixels by 64 to get amount of pixels))
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void adjust_viewport_to_window()
{
    SDL_GetWindowSize(global_window, &global_window_width, &global_window_height);
    SDL_GetWindowSizeInPixels(global_window, &global_pixel_width, &global_pixel_height);
    set_dpi();

    // Calculate aspect ratio scaling
    float aspectRatio = (real32)(global_pixel_width) / (real32)(global_pixel_height);
    float logicalAspectRatio = (real32)(LOGICAL_WIDTH) / LOGICAL_HEIGHT;

    if (aspectRatio > logicalAspectRatio)
    {
        // Window is wider than logical resolution
        int viewportWidth = (int32)(global_pixel_height * logicalAspectRatio);
        int xOffset = (global_pixel_width - viewportWidth) / 2;
        glViewport(xOffset, 0, viewportWidth, global_pixel_height);
    }
    else
    {
        // Window is taller than logical resolution
        int viewportHeight = (int32)(global_pixel_width / logicalAspectRatio);
        int yOffset = (global_pixel_height - viewportHeight) / 2;
        glViewport(0, yOffset, global_pixel_width, viewportHeight);
    }
}

// clang-format off
#include "debug_utils.cpp"
#include "sdl_events.cpp"
#include "audio.cpp"

typedef struct Scene
{
    void (*reset_state)(struct Scene* scene);
    void (*handle_input)(struct Scene* scene, Input* input);
    void (*update)(struct Scene* scene, real64 simulation_time_elapsed, real32 dt_s);
    void (*render)(struct Scene* scene);
    void* state;  // Pointer to the scene-specific state
} Scene;

Scene* global_next_scene;
Scene* global_current_scene;
Scene global_start_screen_scene;
Scene global_gameplay_scene;

Audio_Context global_audio_context;

uint32 global_snake_face_texture;
uint32 global_snake_body_texture;
uint32 global_snake_tail_texture;
uint32 global_egg_texture;

#include "scenes/start_screen.cpp"
#include "scenes/gameplay.cpp"
// clang-format on

bool filterEvent(void* userdata, SDL_Event* event)
{
    Input* input = (Input*)(userdata);

    if (event->type == SDL_EVENT_WINDOW_RESIZED)
    {
        adjust_viewport_to_window();
    }

    return true;  // Allow other events
}

// TODO: do something with this
GLenum glCheckError_(const char* file, int line)
{
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR)
    {
        std::string error;
        switch (errorCode)
        {
            case GL_INVALID_ENUM:
                error = "INVALID_ENUM";
                break;
            case GL_INVALID_VALUE:
                error = "INVALID_VALUE";
                break;
            case GL_INVALID_OPERATION:
                error = "INVALID_OPERATION";
                break;
            case GL_OUT_OF_MEMORY:
                error = "OUT_OF_MEMORY";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                error = "INVALID_FRAMEBUFFER_OPERATION";
                break;
        }
        std::cout << error << " | " << file << " (" << line << ")" << std::endl;
    }
    return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

int32 main(int32 argc, char* argv[])
{
    if (!SDL_SetAppMetadata("Snake Game", "1.0", "com.choo.Snake_Game"))
    {
        return SDL_APP_FAILURE;
    }

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        return SDL_APP_FAILURE;
    }

    {  // Window and Renderer
        // Set OpenGL attributes
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);  // OpenGL 3.x
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);  // OpenGL 3.3
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);  // Enable multisampling
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);  // Set 4x MSAA

        global_window = SDL_CreateWindow("Snake Game",
                                         LOGICAL_WIDTH,
                                         LOGICAL_HEIGHT,
                                         SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);

        if (!global_window)
        {
            SDL_Log("SDL_CreateWindow Error:: %s", SDL_GetError());
            return SDL_APP_FAILURE;
        }

        // Capture the mouse
        if (!SDL_CaptureMouse(true))
        {
            std::cerr << "Failed to capture the mouse: " << SDL_GetError() << std::endl;
        }
        // Set relative mouse mode
        if (!SDL_SetWindowRelativeMouseMode(global_window, true))
        {
            std::cerr << "Failed to set relative mouse mode: " << SDL_GetError() << std::endl;
        }

        // Hide the mouse cursor
        if (!SDL_HideCursor())
        {
            std::cout << "Failed to hide cursor" << std::endl;
        }

        SDL_GLContext gl_context = SDL_GL_CreateContext(global_window);
        if (!gl_context)
        {
            SDL_Log("Could not create OpenGL context: %s", SDL_GetError());
            return SDL_APP_FAILURE;
        }

        // Initialize GLAD
        if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
        {
            SDL_Log("Failed to initialize GLAD: %s", SDL_GetError());
            return SDL_APP_FAILURE;
        }

        SDL_GL_SetSwapInterval(VSYNC_ENABLED);
        // Enable multisampling in OpenGL
        glEnable(GL_MULTISAMPLE);
    }

    if (!TTF_Init())
    {
        std::cerr << "Failed to initialize SDL_ttf: " << SDL_GetError() << std::endl;
        return -1;
    }

    if (!audio_init(&global_audio_context))
    {
        fprintf(stderr, "Failed to initialize audio.\n");
        SDL_Quit();
        return -1;
    }

    // const char* platform = SDL_GetPlatform();
    // std::cout << "Platform " << platform << std::endl;

#ifdef __WINDOWS__
    timeBeginPeriod(1);
#endif

// Set macOS-specific hint
#ifdef __APPLE__
    SDL_SetHint(SDL_HINT_VIDEO_MAC_FULLSCREEN_SPACES, "1");
    // SDL_SetHint(SDL_HINT_RENDER_DRIVER, "metal");
#endif

    set_dpi();

    SDL_Event event;
    Input input = {};

    SDL_SetEventFilter(filterEvent, &input);

    Master_Timer master_timer = {};
    master_timer.COUNTER_FREQUENCY = SDL_GetPerformanceFrequency();
    master_timer.last_frame_counter = SDL_GetPerformanceCounter();

    global_debug_counter = 0;

    real32 accumulator_s = 0.0f;

    global_display_debug_info = 0;

#ifdef _WIN32
    // This looks like a function call but it's actually an intrinsic that
    // runs the actual assembly instruction directly
    global_last_cycle_count = __rdtsc();
#endif

    {  // Start Screen Scene
        global_start_screen_scene = Scene();
        Start_Screen__State start_screen_state = {};
        global_start_screen_scene.state = (void*)&start_screen_state;
        start_screen__reset_state(&global_start_screen_scene);
        global_start_screen_scene.reset_state = &start_screen__reset_state;
        global_start_screen_scene.handle_input = &start_screen__handle_input;
        global_start_screen_scene.update = &start_screen__update;
        global_start_screen_scene.render = &start_screen__render;
    }

    {  // Gameplay Scene
        global_gameplay_scene = Scene();
        Gameplay__State gameplay_state = {};
        global_gameplay_scene.state = (void*)&gameplay_state;
        gameplay__reset_state(&global_gameplay_scene);
        global_gameplay_scene.reset_state = &gameplay__reset_state;
        global_gameplay_scene.handle_input = &gameplay__handle_input;
        global_gameplay_scene.update = &gameplay__update;
        global_gameplay_scene.render = &gameplay__render;
    }

    global_current_scene = &global_start_screen_scene;

    bool32 success;
#define INFO_LOG_LENGTH 512
    char info_log[INFO_LOG_LENGTH];

    // load and create a texture
    // -------------------------
    stbi_set_flip_vertically_on_load(true);  // tell stb_image.h to flip loaded texture's on the y-axis.
    float borderColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };

    // texture 1
    // ---------
    glGenTextures(1, &global_snake_face_texture);
    glBindTexture(GL_TEXTURE_2D, global_snake_face_texture);
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // load image, create texture and generate mipmaps
    int width, height, nrChannels;
    unsigned char* data = stbi_load("assets/images/snake/head.png", &width, &height, &nrChannels, 0);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        // glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);

    // texture 2
    // ---------
    glGenTextures(1, &global_snake_body_texture);
    glBindTexture(GL_TEXTURE_2D, global_snake_body_texture);
    // set the texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    data = stbi_load("assets/images/snake/body.png", &width, &height, &nrChannels, 0);
    if (data)
    {
        // note that the awesomeface.png has transparency and thus an alpha channel, so make sure to tell OpenGL the
        // data type is of GL_RGBA
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        // glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);

    // texture 3
    // ---------
    glGenTextures(1, &global_egg_texture);
    glBindTexture(GL_TEXTURE_2D, global_egg_texture);
    // set the texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    data = stbi_load("assets/images/snake/egg.png", &width, &height, &nrChannels, 0);
    if (data)
    {
        // note that the awesomeface.png has transparency and thus an alpha channel, so make sure to tell OpenGL the
        // data type is of GL_RGBA
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        // glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);

    // texture 4
    // ---------
    glGenTextures(1, &global_snake_tail_texture);
    glBindTexture(GL_TEXTURE_2D, global_snake_tail_texture);
    // set the texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    data = stbi_load("assets/images/snake/tail.png", &width, &height, &nrChannels, 0);
    if (data)
    {
        // note that the awesomeface.png has transparency and thus an alpha channel, so make sure to tell OpenGL the
        // data type is of GL_RGBA
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        // glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);
    /*------------------------------------------------------------*/
    // compile and setup the shader
    // ----------------------------
    Shader text_shader("src/shaders/text.vs.glsl", "src/shaders/text.fs.glsl");
    global_text_shader = &text_shader;

    // FreeType
    // --------
    FT_Library ft;
    // All functions return a value different than 0 whenever an error occurred
    if (FT_Init_FreeType(&ft))
    {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        return -1;
    }

    // find path to font
    // std::string font_name = FileSystem::getPath("assets/fonts/Share_Tech_Mono/ShareTechMono-Regular.ttf");
    // if (font_name.empty())
    // {
    //     std::cout << "ERROR::FREETYPE: Failed to load font_name" << std::endl;
    //     return -1;
    // }

    // load font as face
    FT_Face face;
    if (FT_New_Face(ft, "assets/fonts/PixelHigh.ttf", 0, &face))
    {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
        return -1;
    }
    else
    {
        // set size to load glyphs as
        FT_Set_Pixel_Sizes(face, 0, 48 * FONT_SCALE_FACTOR);

        // disable byte-alignment restriction
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        // load first 128 characters of ASCII set
        for (unsigned char c = 0; c < 128; c++)
        {
            // We're using signed distance fields!
            FT_Int32 load_flags = FT_LOAD_RENDER | FT_LOAD_TARGET_(FT_RENDER_MODE_SDF);
            // Load character glyph
            if (FT_Load_Char(face, c, load_flags))
            {
                std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
                continue;
            }
            // generate texture
            unsigned int texture;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(GL_TEXTURE_2D,
                         0,
                         GL_RED,
                         face->glyph->bitmap.width,
                         face->glyph->bitmap.rows,
                         0,
                         GL_RED,
                         GL_UNSIGNED_BYTE,
                         face->glyph->bitmap.buffer);
            // set texture options
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            // now store character for later use
            Character character = {texture,
                                   glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                                   glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                                   static_cast<unsigned int>(face->glyph->advance.x)};
            Characters.insert(std::pair<char, Character>(c, character));
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    // destroy FreeType once we're finished
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    // configure VAO/VBO for texture quads
    // -----------------------------------
    glGenVertexArrays(1, &FONT_VAO);
    glGenBuffers(1, &FONT_VBO);
    glBindVertexArray(FONT_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, FONT_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    /*------------------------------------------------------------*/

    Shader basic_shader("src/shaders/basic_shader.vs.glsl", "src/shaders/basic_shader.fs.glsl");
    global_basic_shader = &basic_shader;

    Shader grid_shader("src/shaders/2d_texture.vs.glsl", "src/shaders/2d_texture.fs.glsl");
    global_grid_shader = &grid_shader;

    setupQuad();
    setup_square_buffers();
    adjust_viewport_to_window();

    while (global_running)
    {
//==============================
// TIMING
#ifdef _WIN32
        global_LAST_total_cycles_elapsed = global_total_cycles_elapsed;
        global_LAST_cycles_elapsed_before_render = global_cycles_elapsed_before_render;
        global_LAST_cycles_elapsed_after_render = global_cycles_elapsed_after_render;

        uint64 global_cycle_count_now = __rdtsc();
        global_cycles_elapsed_before_render = global_cycle_count_now - global_last_cycle_count;
#endif
        Uint64 counter_now = SDL_GetPerformanceCounter();
//==============================

        Master_Timer last_master_timer = master_timer;

        if (TEXT_DEBUGGING_ENABLED && global_debug_counter == 0) // Displays Debug info in the console
        {
            print_debug_info_text_to_console(last_master_timer);
        }

        {  // Input and event handling
            handle_events(&event, &input);
            global_current_scene->handle_input(global_current_scene, &input);
        }

        {  // Scene Manager
            if (global_next_scene) {
                global_current_scene = global_next_scene;
                global_current_scene->reset_state(global_current_scene);
                global_next_scene = 0;
            }
        }

        { // Update Scene
            // Gameplay_State state_to_render;
            // https://gafferongames.com/post/fix_your_timestep/
            real32 frame_time_s = master_timer.total_frame_time_elapsed__seconds;

            if (frame_time_s > 0.25f)
            {
                // Prevent "spiraling" (excessive frame accumulation) in case of a big lag spike.
                frame_time_s = 0.25f;
            }

            accumulator_s += frame_time_s;

            while (accumulator_s >= SIMULATION_DELTA_TIME_S)
            {  // Simulation 'consumes' whatever time is given to it based on the render rate
                // previous_state = current_state;
                global_current_scene->update(global_current_scene,
                                             master_timer.physics_simulation_elapsed_time__seconds,
                                             SIMULATION_DELTA_TIME_S);
                master_timer.physics_simulation_elapsed_time__seconds += SIMULATION_DELTA_TIME_S;
                accumulator_s -= SIMULATION_DELTA_TIME_S;
            }

            // TODO: we can do some interpolation here if we ever need to make the rendering a bit smoother
            // real32 alpha = accumulator_s / SIMULATION_DELTA_TIME_S;
            // Interpolate between the current state and previous state
            // NOTE: the render always lags by about a frame

            // NOTE: I've commented this out because I don't think we need linear interpolation for new grid-based approach?
            // state_to_render = current_state * alpha + previous_state * (1.0f - alpha);
            // state_to_render = current_state;
        }

//==============================
// TIMING
#ifdef _WIN32
        uint64 global_cycle_count_before_render = __rdtsc();
        global_cycles_elapsed_before_render = global_cycle_count_before_render - global_cycle_count_now;
#endif
        Uint64 counter_after_work = SDL_GetPerformanceCounter();
        master_timer.time_elapsed_for_work__seconds =
            ((real32)(counter_after_work - counter_now) / (real32)master_timer.COUNTER_FREQUENCY);
//==============================

        { // Write to render buffer
            // Clear the screen
            glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            global_current_scene->render(global_current_scene);

            if (global_display_debug_info)
            {
                display_debug_info_text(last_master_timer);
            }

            display_timer_in_window_name(last_master_timer);
        }

//==============================
// TIMING
        Uint64 counter_after_writing_buffer = SDL_GetPerformanceCounter();
        master_timer.time_elapsed_for_writing_buffer__seconds =
            ((real32)(counter_after_writing_buffer - counter_after_work) / (real32)master_timer.COUNTER_FREQUENCY);
//==============================

        {
            GLenum error = glGetError();
            if (error != GL_NO_ERROR)
            {
                fprintf(stderr, "OpenGL Error: %d\n", error);
            }
            // Swap buffers
            SDL_GL_SwapWindow(global_window);
        }

//==============================
// TIMING

#ifdef _WIN32
        uint64 global_cycle_count_after_render = __rdtsc();
        global_cycles_elapsed_after_render = global_cycle_count_after_render - global_cycle_count_before_render;
#endif
        Uint64 counter_after_render = SDL_GetPerformanceCounter();
        master_timer.time_elapsed_for_render__seconds =
            ((real32)(counter_after_render - counter_after_writing_buffer) / (real32)master_timer.COUNTER_FREQUENCY);
//==============================

        {  // Sleep with busy-wait for precise timings
            real64 TARGET_FRAME_DURATION__Millis = 1000 / TARGET_SCREEN_FPS;
            real64 target_duration_ticks =
                (TARGET_FRAME_DURATION__Millis * master_timer.COUNTER_FREQUENCY) / 1000;  // Convert to ticks
            Uint64 elapsed_ticks = counter_after_render - counter_now;

            if (elapsed_ticks < target_duration_ticks)
            {
                real64 remaining_ticks = target_duration_ticks - (real64)elapsed_ticks;
                real64 remaining_millis = 1000 * (remaining_ticks / master_timer.COUNTER_FREQUENCY);

                if (remaining_millis > 2.f)
                {
                    // Precision can be ~1ms on sleep these days. I'm minusing a ms because i want to spin wait
                    // for the remainder ms.
                    SDL_Delay((uint32)(remaining_millis - 1.0f));
                }

                // Spin-wait for the remaining time (microsecond precision)
                while (SDL_GetPerformanceCounter() - counter_now < target_duration_ticks)
                {
                    // Busy-wait for precise timing
                }
            }
        }

        tick_debug_counter(master_timer);

//==============================
// TIMING

#ifdef _WIN32
        uint64 global_end_cycle_count_after_delay = __rdtsc();
        global_total_cycles_elapsed = global_end_cycle_count_after_delay - global_last_cycle_count;
#endif
        Uint64 counter_after_sleep = SDL_GetPerformanceCounter();
        master_timer.time_elapsed_for_sleep__seconds =
            ((real32)(counter_after_sleep - counter_after_render) / (real32)master_timer.COUNTER_FREQUENCY);
        master_timer.total_frame_time_elapsed__seconds =
            ((real32)(counter_after_sleep - counter_now) / (real32)master_timer.COUNTER_FREQUENCY);

        // Next iteration
        master_timer.last_frame_counter = counter_after_sleep;
#ifdef _WIN32
        global_last_cycle_count = global_end_cycle_count_after_delay;
#endif
//==============================
    } // end while (global_running)

    TTF_Quit();
    SDL_DestroyWindow(global_window);
    SDL_Quit();

    return EXIT_SUCCESS;
}