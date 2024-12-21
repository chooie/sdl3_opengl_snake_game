#include <SDL3/SDL.h>

#include "../audio.h"
#include "../common.h"

// TODO: move to util or some re-usable place when necessary
struct Screen_Space_Position
{
    real32 x;
    real32 y;
};
Screen_Space_Position map_world_space_position_to_screen_space_position(real32 world_x, real32 world_y)
{
    Screen_Space_Position result = {};
    real32 actual_x = (world_x + 1) * GRID_BLOCK_SIZE;
    // Flip Y so positive y is up and negative y is down
    real32 actual_y = ((world_y + 1) * GRID_BLOCK_SIZE);

    // printf("x: %.2f, y: %.2f\n", actual_x, actual_y);

    result.x = actual_x;
    result.y = actual_y;
    return result;
}

typedef enum
{
    DIRECTION_NONE,  // Represents no movement
    DIRECTION_NORTH,
    DIRECTION_EAST,
    DIRECTION_SOUTH,
    DIRECTION_WEST
} Direction;

struct Snake_Part
{
    int32 pos_x;
    int32 pos_y;
    Direction direction;
};

#define MAX_TAIL_LENGTH 1000

struct Gameplay__State
{
    bool32 is_starting;
    bool32 game_over;
    bool32 is_paused;

    int32 pos_x;
    int32 pos_y;

    Snake_Part snake_parts[MAX_TAIL_LENGTH];
    uint32 next_snake_part_index;

    Direction current_direction;
    Direction proposed_direction;

    real32 time_until_grid_jump__seconds;
    real32 set_time_until_grid_jump__seconds;

    int32 blip_pos_x;
    int32 blip_pos_y;

    // Overload * operator for scalar multiplication
    Gameplay__State operator*(real32 scalar) const
    {
        Gameplay__State result;
        // result.pos_x = pos_x * scalar;
        // result.pos_y = pos_y * scalar;
        // result.velocity_x = velocity_x * scalar;
        // result.velocity_y = velocity_y * scalar;
        return result;
    }

    // Overload + operator for adding two States
    Gameplay__State operator+(const Gameplay__State& other) const
    {
        Gameplay__State result;
        // NOTE: I've commented this out because I don't think we need linear interpolation for new grid-based approach?
        // result.pos_x = pos_x + other.pos_x;
        // result.pos_y = pos_y + other.pos_y;
        // result.velocity_x = velocity_x + other.velocity_x;
        // result.velocity_y = velocity_y + other.velocity_y;
        return result;
    }
};

// TODO move this somewhere
// TODO should this be part of Gameplay__State?
#define MAX_INPUTS 10
Direction input_queue[MAX_INPUTS];
int32 head = 0;  // Points to the current input to be processed
int32 tail = 0;  // Points to the next free spot for adding input

void add_input(Direction dir)
{
    int32 next_tail = (tail + 1) % MAX_INPUTS;
    if (next_tail != head)  // Only add if there's space in the queue
    {
        input_queue[tail] = dir;
        tail = next_tail;
    }
}

Direction get_next_input()
{
    if (head == tail)
    {
        // No inputs available, return current direction
        return DIRECTION_NONE;
    }
    Direction dir = input_queue[head];
    head = (head + 1) % MAX_INPUTS;
    return dir;
}

void gameplay__reset_state(Scene* scene)
{
    Gameplay__State* state = (Gameplay__State*)scene->state;

    head = 0;
    tail = 0;

    state->is_starting = 1;
    state->is_paused = 1;
    state->game_over = 0;

    state->pos_x = X_GRIDS / 2;
    state->pos_y = Y_GRIDS / 4;
    state->current_direction = DIRECTION_NORTH;
    state->next_snake_part_index = 0;

    state->set_time_until_grid_jump__seconds = .1f;
    state->time_until_grid_jump__seconds = state->set_time_until_grid_jump__seconds;

    state->blip_pos_x = X_GRIDS / 2;
    state->blip_pos_y = Y_GRIDS / 2;
}

#define DYNAMIC_SCORE_LENGTH 5 + 7 // TODO: 7 is accounting for "Score: "
global_variable char global_dynamic_score_text[DYNAMIC_SCORE_LENGTH]; // Make sure the buffer is large enough

void gameplay__handle_input(Scene* scene, Input* input)
{
    Gameplay__State* state = (Gameplay__State*)scene->state;

    if (pressed(BUTTON_ESCAPE))
    {
        global_next_scene = &global_start_screen_scene;
    }

    if (pressed(BUTTON_SPACE) && !state->game_over)
    {
        state->is_paused = !state->is_paused;
    }

    if (!state->is_paused)
    {
        if (pressed(BUTTON_W) || pressed(BUTTON_UP))
        {
            add_input(DIRECTION_NORTH);
        }

        if (pressed(BUTTON_A) || pressed(BUTTON_LEFT))
        {
            add_input(DIRECTION_WEST);
        }

        if (pressed(BUTTON_S) || pressed(BUTTON_DOWN))
        {
            add_input(DIRECTION_SOUTH);
        }

        if (pressed(BUTTON_D) || pressed(BUTTON_RIGHT))
        {
            add_input(DIRECTION_EAST);
        }
    }

    if (state->game_over && pressed(BUTTON_ENTER))
    {
        gameplay__reset_state(scene);
        state->is_paused = 0;
    }
}

//=======================================================
// UPDATE
//=======================================================

// Define parameters for the LCG
uint32 seed = 12345;  // Initial seed value
#define LCG_A 1664525u
#define LCG_C 1013904223u

// Function to generate a pseudorandom number using LCG
uint32 custom_rand()
{
    seed = LCG_A * seed + LCG_C;
    return seed;
}

// Function to generate a number between 0 and max - 1 (inclusive)
uint32 custom_rand_range(uint32 max)
{
    return custom_rand() % (max);
}

void gameplay__update(struct Scene* scene, real64 simulation_time_elapsed, real32 dt_s)
{
    Gameplay__State* state = (Gameplay__State*)scene->state;

    if (state->is_starting)
    {
        state->is_starting = 0;
        play_music(global_audio_context.gameplay_background_music);
        set_music_volume(100.f);
    }

    if (state->game_over || state->is_paused)
    {
        return;
    }

    state->time_until_grid_jump__seconds -= dt_s;

    if (state->time_until_grid_jump__seconds <= 0)
    {
        Direction proposed_direction = get_next_input();

        switch (proposed_direction)
        {
            case DIRECTION_NORTH:
            {
                if (state->current_direction != DIRECTION_SOUTH)
                {
                    state->current_direction = DIRECTION_NORTH;
                }
            }
            break;
            case DIRECTION_EAST:
            {
                if (state->current_direction != DIRECTION_WEST)
                {
                    state->current_direction = DIRECTION_EAST;
                }
            }
            break;
            case DIRECTION_SOUTH:
            {
                if (state->current_direction != DIRECTION_NORTH)
                {
                    state->current_direction = DIRECTION_SOUTH;
                }
            }
            break;
            case DIRECTION_WEST:
            {
                if (state->current_direction != DIRECTION_EAST)
                {
                    state->current_direction = DIRECTION_WEST;
                }
            }
            break;
        }

        {  // Blip collision
            if (state->pos_x == state->blip_pos_x && state->pos_y == state->blip_pos_y)
            {
                play_sound_effect(global_audio_context.effect_beep_2);

                {  // Grow snake part
                    Snake_Part new_snake_part = {};
                    Snake_Part* last_snake_part = &state->snake_parts[state->next_snake_part_index];
                    new_snake_part.pos_x = last_snake_part->pos_x;
                    new_snake_part.pos_y = last_snake_part->pos_y;
                    new_snake_part.direction = last_snake_part->direction;
                    state->next_snake_part_index++;

                    if (!(state->next_snake_part_index < MAX_TAIL_LENGTH))
                    {
                        SDL_SetError("Snake tail must never get this long! %d", state->next_snake_part_index);
                        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", SDL_GetError());
                        SDL_assert_release(state->next_snake_part_index < MAX_TAIL_LENGTH);
                    }
                }

                {  // Randomly spawn blip somewhere else
                    uint32 random_x = custom_rand_range(X_GRIDS);
                    state->blip_pos_x = random_x;

                    uint32 random_y = custom_rand_range(Y_GRIDS);
                    state->blip_pos_y = random_y;
                }

                state->set_time_until_grid_jump__seconds -= 0.0005f;
            }
        }

        for (int32 i = state->next_snake_part_index - 1; i >= 0; i--)
        {
            Snake_Part* current_snake_part = &state->snake_parts[i];

            if (i == 0)
            {
                current_snake_part->pos_x = state->pos_x;
                current_snake_part->pos_y = state->pos_y;
                current_snake_part->direction = state->current_direction;
            }
            else
            {
                Snake_Part* previous_snake_part = &state->snake_parts[i - 1];
                current_snake_part->pos_x = previous_snake_part->pos_x;
                current_snake_part->pos_y = previous_snake_part->pos_y;
                current_snake_part->direction = previous_snake_part->direction;
            }
        }

        switch (state->current_direction)
        {
            case DIRECTION_NORTH:
            {
                if (state->current_direction != DIRECTION_SOUTH)
                {
                    state->pos_y++;
                }
            }
            break;
            case DIRECTION_EAST:
            {
                if (state->current_direction != DIRECTION_WEST)
                {
                    state->pos_x++;
                }
            }
            break;
            case DIRECTION_SOUTH:
            {
                if (state->current_direction != DIRECTION_NORTH)
                {
                    state->pos_y--;
                }
            }
            break;
            case DIRECTION_WEST:
            {
                if (state->current_direction != DIRECTION_EAST)
                {
                    state->pos_x--;
                }
            }
            break;
        }

        {  // End Game if player crashes
            for (uint32 i = 0; i < state->next_snake_part_index; i++)
            {
                Snake_Part* current_snake_part = &state->snake_parts[i];
                if (state->pos_x == current_snake_part->pos_x && state->pos_y == current_snake_part->pos_y)
                {
                    state->game_over = 1;
                    break;
                }
            }

            if (state->pos_x < 0 || state->pos_x >= (int32)X_GRIDS || state->pos_y < 0 || state->pos_y >= (int32)Y_GRIDS)
            {
                state->game_over = 1;
            }

            if (state->game_over)
            {
                play_sound_effect(global_audio_context.effect_boom);
            }
        }

        state->time_until_grid_jump__seconds = state->set_time_until_grid_jump__seconds;
        // printf("x: %d, y: %d, %d, %d\n", state->pos_x, state->pos_y, state->current_direction,
        // state->proposed_direction);
    }
}

//=======================================================
// RENDER
//=======================================================

void drawSquare(Shader& shader, float x, float y, float size, glm::vec3 color)
{
    setupGeometryRenderingState();

    // Draw the square
    shader.use();

    glm::mat4 projection = glm::ortho(0.0f, (real32)LOGICAL_WIDTH, 0.0f, (real32)LOGICAL_HEIGHT);
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3f(glGetUniformLocation(shader.ID, "color"), color.r, color.g, color.b);

    // Create the model matrix for position and size
    glm::mat4 model = glm::mat4(1.0f);                       // Identity matrix
    model = glm::translate(model, glm::vec3(x, y, 0.0f));    // Translate to position (x, y)
    model = glm::scale(model, glm::vec3(size, size, 1.0f));  // Scale to the desired size
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(global_square_VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    
    glBindVertexArray(0); // Unbind for safety
}

void draw_texture(Shader& shader, uint32 texture_id, real32 x, real32 y, real32 angle__degrees, real32 size)
{
    setupGeometryRenderingState();

    // Draw the square
    shader.use();

    glm::mat4 projection = glm::ortho(0.0f, (real32)LOGICAL_WIDTH, 0.0f, (real32)LOGICAL_HEIGHT);
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // Create the model matrix for position and size
    glm::mat4 model = glm::mat4(1.0f);                       // Identity matrix
    model = glm::translate(model, glm::vec3(x, y, 0.0f));    // Translate to position (x, y) 
    model = glm::scale(model, glm::vec3(size, size, 1.0f));  // Scale to the desired size
    real32 angle__radians = glm::radians(angle__degrees);
    model = glm::rotate(model, angle__radians, glm::vec3(0.0f, 0.0f, 1.0f));
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, glm::value_ptr(model));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glUniform1i(glGetUniformLocation(shader.ID, "texture1"), 0); // Tell the shader to use texture unit 0

     // Render a quad that covers the screen or the grid's intended area
    glBindVertexArray(quadVAO);  // Assume a preconfigured VAO for a full-screen quad
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    
    glBindVertexArray(0); // Unbind for safety
}

real32 get_angle_from_direction(Direction direction)
{
    real32 angle__degrees = 0.f;
    if (direction == DIRECTION_NORTH)
    {
        angle__degrees = 0.f;
    }
    else if (direction == DIRECTION_EAST)
    {
        angle__degrees = 270.f;
    }
    else if (direction == DIRECTION_SOUTH)
    {
        angle__degrees = 180.f;
    }
    else if (direction == DIRECTION_WEST)
    {
        angle__degrees = 90.f;
    }
    else {
        SDL_assert(!"Invalid direction passed to calculate angle");
    }

    return angle__degrees;
}

void drawGrid(Shader& shader,
              int xGrids,
              int yGrids,
              float gridSize,
              float borderThickness,
              glm::vec3 borderColor,
              glm::vec3 fillColor)
{
    // Loop through all grid cells
    for (int row = 0; row < yGrids; row++)
    {
        for (int col = 0; col < xGrids; col++)
        {
            // Calculate the position of the current grid cell
            float x = (col + 1) * gridSize - (gridSize / 2);
            float y = (row + 1) * gridSize - (gridSize / 2);

            drawSquare(shader, x, y, gridSize, borderColor);
            drawSquare(shader, x, y, gridSize - borderThickness, fillColor);
        }
    }
}

GLuint gridFBO, gridTexture;

void createGridFramebuffer(int gridWidth, int gridHeight)
{
    // Generate and bind the framebuffer
    glGenFramebuffers(1, &gridFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, gridFBO);

    // Create the texture to store the grid
    glGenTextures(1, &gridTexture);
    glBindTexture(GL_TEXTURE_2D, gridTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, gridWidth, gridHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // Disable smoothing (use nearest-neighbor filtering)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Attach the texture to the framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gridTexture, 0);

    // Check if framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        fprintf(stderr, "Error: Framebuffer is not complete.\n");
        return;
    }

    // Unbind the framebuffer and texture
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void renderGridToTexture(Shader& gridShader,
                         int xGrids,
                         int yGrids,
                         float gridSize,
                         float borderThickness,
                         glm::vec3 borderColor,
                         glm::vec3 fillColor)
{
    // Bind the framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, gridFBO);

    // Set the viewport to match the grid texture size
    glViewport(0, 0, (GLsizei)(xGrids * gridSize), (GLsizei)(yGrids * gridSize));

    // Clear the framebuffer
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);  // Transparent background
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw the grid
    drawGrid(gridShader, xGrids, yGrids, gridSize, borderThickness, borderColor, fillColor);

    // Unbind the framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Reset viewport to the default size
    adjust_viewport_to_window();
}

void renderCachedGrid(Shader& textureShader, real32 x, real32 y, real32 width, real32 height)
{
    setupGeometryRenderingState();

    // Bind the texture shader
    glUseProgram(textureShader.ID);

    // Set up orthographic projection
    glm::mat4 projection = glm::ortho(0.0f, (float)LOGICAL_WIDTH, 0.0f, (float)LOGICAL_HEIGHT);
    glUniformMatrix4fv(glGetUniformLocation(textureShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // Create the model matrix for position and size
    glm::mat4 model = glm::mat4(1.0f);                          // Identity matrix
    model = glm::translate(model, glm::vec3(x, y, 0.0f));       // Translate to position (x, y)
    model = glm::scale(model, glm::vec3(width, height, 1.0f));  // Scale to the desired size
    glUniformMatrix4fv(glGetUniformLocation(textureShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(model));

    // Bind the texture to a texture unit
    glActiveTexture(GL_TEXTURE0);                                        // Activate texture unit 0
    glBindTexture(GL_TEXTURE_2D, gridTexture);                           // Bind the grid texture
    glUniform1i(glGetUniformLocation(textureShader.ID, "texture1"), 0);  // Tell the shader to use texture unit 0

    // Render a quad that covers the screen or the grid's intended area
    glBindVertexArray(quadVAO);  // Assume a preconfigured VAO for a full-screen quad
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // Unbind the VAO and texture
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void gameplay__render(Scene* scene)
{
    Gameplay__State* state = (Gameplay__State*)scene->state;

    float borderThickness = 2.0f;                // Border thickness
    glm::vec3 borderColor(0.23f, 0.23f, 0.23f);  // Dark grey
    glm::vec3 fillColor(0.16f, 0.16f, 0.16f);    // Lighter grey
    // drawGrid(*global_basic_shader, X_GRIDS, Y_GRIDS, (real32)GRID_BLOCK_SIZE, borderThickness, borderColor, fillColor);
    if (!gridFBO)
    {
        createGridFramebuffer(X_GRIDS * GRID_BLOCK_SIZE, Y_GRIDS * GRID_BLOCK_SIZE);
        renderGridToTexture(*global_basic_shader,
                            X_GRIDS,
                            Y_GRIDS,
                            (real32)GRID_BLOCK_SIZE,
                            borderThickness,
                            borderColor,
                            fillColor);
    }
    renderCachedGrid(*global_grid_shader,
                     (real32)LOGICAL_WIDTH / 2,
                     (real32)LOGICAL_HEIGHT / 2,
                     (real32)LOGICAL_WIDTH,
                     (real32)LOGICAL_HEIGHT);

    {  // Draw Blip
        Screen_Space_Position square_screen_pos =
            map_world_space_position_to_screen_space_position((real32)state->blip_pos_x, (real32)state->blip_pos_y);
        real32 size = (real32)(GRID_BLOCK_SIZE);
        real32 x = (real32)(square_screen_pos.x - ((real32)GRID_BLOCK_SIZE / 2));
        real32 y = (real32)(square_screen_pos.y - ((real32)GRID_BLOCK_SIZE / 2));

        // glm::vec3 blue_color = {0.204f, 0.596f, 0.859f};
        // drawSquare(*global_basic_shader, x, y, size, blue_color);
        draw_texture(*global_grid_shader, global_egg_texture, x, y, 0.f, size);
    }

    {  // Draw Player
        // Tail parts
        for (uint32 i = 0; i < state->next_snake_part_index; i++)
        {
            Snake_Part* snake_part = &state->snake_parts[i];
            Screen_Space_Position screen_pos =
                map_world_space_position_to_screen_space_position((real32)snake_part->pos_x, (real32)snake_part->pos_y);

            real32 size = (real32)(GRID_BLOCK_SIZE);
            real32 x = screen_pos.x - ((real32)GRID_BLOCK_SIZE / 2);
            real32 y = screen_pos.y - ((real32)GRID_BLOCK_SIZE / 2);

            // glm::vec3 darkened_orange_color = {0.85f, 0.55f, 0.0f};
            // drawSquare(*global_basic_shader, x, y, size, darkened_orange_color);
            Direction direction = snake_part->direction;
            real32 angle = get_angle_from_direction(direction);

            if (i == state->next_snake_part_index - 1)
            {
                draw_texture(*global_grid_shader, global_snake_tail_texture, x, y, angle, size);
            }
            else
            {
                draw_texture(*global_grid_shader, global_snake_body_texture, x, y, angle, size);
            }
        }

        // Player
        Screen_Space_Position square_screen_pos =
            map_world_space_position_to_screen_space_position((real32)state->pos_x, (real32)state->pos_y);
        real32 size = (real32)(GRID_BLOCK_SIZE);
        real32 x = square_screen_pos.x - ((real32)GRID_BLOCK_SIZE / 2);
        real32 y = square_screen_pos.y - ((real32)GRID_BLOCK_SIZE / 2);

        // glm::vec3 red_color = {0.6039f, 0.2471f, 0.2314f};
        // drawSquare(*global_basic_shader, x, y, size, red_color);
        Direction direction = state->current_direction;
        real32 angle = get_angle_from_direction(direction);
        draw_texture(*global_grid_shader, global_snake_face_texture, x, y, angle, size);
    }

    {  // Dynamic Score Text
        float initial_x = (real32)LOGICAL_WIDTH - (LOGICAL_WIDTH * 0.05f);
        float initial_y = (real32)LOGICAL_HEIGHT - 5.0f;
        glm::vec3 text_color = white;

        char text[DYNAMIC_SCORE_LENGTH];
        snprintf(text, DYNAMIC_SCORE_LENGTH, "Score: %d", state->next_snake_part_index);

        real32 size_ratio = 0.5f;
        float text_scale = size_ratio / FONT_SCALE_FACTOR;

        // Compute the total text width
        float text_width = 0.0f;
        for (char c : text)
        {
            text_width += (Characters[c].Advance >> 6) * text_scale;  // Horizontal advance in pixels
        }

        float x = initial_x - text_width;
        float y =
            initial_y - (Characters['H'].Size.y * text_scale);  // Use a sample character for height

        RenderText(*global_text_shader, text, x, y, text_scale, text_color);
    }

    {  // Game over stuff
        if (state->game_over)
        {
            real32 initial_x = 0.5f * (real32)LOGICAL_WIDTH;
            real32 initial_y = 0.75f * (real32)LOGICAL_HEIGHT;
            real32 game_over_size_ratio = 1.75f;
            float game_over_text_scale = game_over_size_ratio / FONT_SCALE_FACTOR;
            real32 game_over_height = Characters['H'].Size.y * game_over_text_scale;

            {  // Render Game Over
                glm::vec3 text_color = white;

                std::string text = "Game Over";

                // Compute the total text width
                float text_width = 0.0f;
                for (char c : text)
                {
                    text_width += (Characters[c].Advance >> 6) * game_over_text_scale;  // Horizontal advance in pixels
                }

                // Adjust for centering
                float x = initial_x - (text_width / 2.0f);
                float y = initial_y - (game_over_height / 2.0f);

                RenderText(*global_text_shader, text, x, y, game_over_text_scale, text_color);
            }

            {  // Render Restart Text
                glm::vec3 text_color = white;
                real32 size_ratio = 1.0f;
                float text_scale = size_ratio / FONT_SCALE_FACTOR;

                initial_y -= game_over_height;
                initial_y -= game_over_height / 4.0f;

                std::string text = "Press <Enter> to restart.";

                // Compute the total text width
                float text_width = 0.0f;
                for (char c : text)
                {
                    text_width += (Characters[c].Advance >> 6) * text_scale;  // Horizontal advance in pixels
                }

                // Adjust for centering
                float x = initial_x - (text_width / 2.0f);
                float y =
                    initial_y - (Characters['H'].Size.y * text_scale / 2.0f);  // Use a sample character for height

                RenderText(*global_text_shader, text, x, y, text_scale, text_color);
            }
        }
    }

    {  // Paused
        // Game Paused
        if (state->is_paused)
        {
            real32 initial_x = (real32)LOGICAL_WIDTH / 2;
            real32 initial_y = (real32)LOGICAL_HEIGHT / 2;
            glm::vec3 text_color = white;
            real32 size_ratio = 0.75f;
            float text_scale = size_ratio / FONT_SCALE_FACTOR;

            std::string text = "PAUSED";

            // Compute the total text width
            float text_width = 0.0f;
            for (char c : text)
            {
                text_width += (Characters[c].Advance >> 6) * text_scale;  // Horizontal advance in pixels
            }
            // Compute the total text width
            float snake_game_text_width = 0.0f;
            for (char c : text)
            {
                snake_game_text_width += (Characters[c].Advance >> 6) * text_scale;  // Horizontal advance in pixels
            }

            // Adjust for centering
            float x = initial_x - (snake_game_text_width / 2.0f);
            float y = initial_y - (Characters['H'].Size.y * text_scale / 2.0f);  // Use a sample character for height

            RenderText(*global_text_shader, text, x, y, text_scale, text_color);
        }
    }
}