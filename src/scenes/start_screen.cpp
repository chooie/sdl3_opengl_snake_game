#include <SDL3/SDL.h>

typedef enum
{
    Start_Screen_Option__Start_Game,
    Start_Screen_Option__Exit_Game,
} Start_Screen__Option;

struct Start_Screen__State
{
    bool32 is_starting;
    glm::vec3 blink_color;
    Start_Screen__Option current_option;
};

glm::vec3 yellow = glm::vec3(0.7686f, 0.6275f, 0.0118f);
glm::vec3 white = {1.0f, 1.0f, 1.0f};

void start_screen__reset_state(Scene* scene)
{
    Start_Screen__State* state = (Start_Screen__State*)scene->state;
    state->is_starting = 1;
    state->blink_color = white;
    state->current_option = Start_Screen_Option__Start_Game;
}

void start_screen__handle_input(Scene* scene, Input* input)
{
    Start_Screen__State* state = (Start_Screen__State*)scene->state;

    if (pressed(BUTTON_ENTER) && state->current_option == Start_Screen_Option__Start_Game)
    {
        global_next_scene = &global_gameplay_scene;
    }

    if (pressed(BUTTON_ENTER) && state->current_option == Start_Screen_Option__Exit_Game)
    {
        global_running = 0;
    }

    if (pressed(BUTTON_D) || pressed(BUTTON_A))
    {
        play_sound_effect(global_audio_context.effect_beep);
    }

    if (pressed(BUTTON_D))
    {
        if (state->current_option == Start_Screen_Option__Start_Game)
        {
            state->current_option = Start_Screen_Option__Exit_Game;
        }
        else if (state->current_option == Start_Screen_Option__Exit_Game)
        {
            state->current_option = Start_Screen_Option__Start_Game;
        }
    }

    if (pressed(BUTTON_A))
    {
        if (state->current_option == Start_Screen_Option__Start_Game)
        {
            state->current_option = Start_Screen_Option__Exit_Game;
        }
        else if (state->current_option == Start_Screen_Option__Exit_Game)
        {
            state->current_option = Start_Screen_Option__Start_Game;
        }
    }
}

real32 TICK_EVERY__SECONDS = 0.15f;
real32 global_tick_time_remaining = TICK_EVERY__SECONDS;
int32 global_marker;
int32 global_next_marker;

void start_screen__update(struct Scene* scene, real64 simulation_time_elapsed, real32 dt_s)
{
    Start_Screen__State* state = (Start_Screen__State*)scene->state;

    if (state->is_starting)
    {
        state->is_starting = 0;
        play_music(global_audio_context.start_screen_background_music);
        set_music_volume(100.f);
    }

    if (global_tick_time_remaining <= 0)
    {
        global_tick_time_remaining = TICK_EVERY__SECONDS;

        uint32 seconds = (uint32)simulation_time_elapsed;

        if (global_marker == 0)
        {
            state->blink_color = white;
            global_next_marker = 1;
        }
        else if (global_marker == 1)
        {
            state->blink_color = yellow;
            global_next_marker = 0;
        }

        if (global_next_marker != global_marker)
        {
            global_marker = global_next_marker;
        }
    }

    global_tick_time_remaining -= dt_s;
}

void start_screen__render(Scene* scene)
{
    Start_Screen__State* state = (Start_Screen__State*)scene->state;

    {  // Snake Game Text
        float snake_game_initial_x = LOGICAL_WIDTH / 2.0f;
        float snake_game_initial_y = LOGICAL_HEIGHT * 3.0f / 4.0f;
        glm::vec3 text_color = white;

        std::string snake_game_text = "Snake Game";
        float snake_game_text_scale = 2.0f / FONT_SCALE_FACTOR;

        // Compute the total text width
        float snake_game_text_width = 0.0f;
        for (char c : snake_game_text)
        {
            snake_game_text_width +=
                (Characters[c].Advance >> 6) * snake_game_text_scale;  // Horizontal advance in pixels
        }

        // Adjust for centering
        float snake_game_x = snake_game_initial_x - (snake_game_text_width / 2.0f);
        float snake_game_y = snake_game_initial_y - (Characters['H'].Size.y * snake_game_text_scale /
                                                     2.0f);  // Use a sample character for height

        RenderText(*global_text_shader, snake_game_text, snake_game_x, snake_game_y, snake_game_text_scale, text_color);
    }

    {  // Start Text
        float start_initial_x = LOGICAL_WIDTH * 1.0f / 4.0f;
        float start_initial_y = LOGICAL_HEIGHT * 1.0f / 4.0f;
        glm::vec3 text_color = white;
        if (state->current_option == Start_Screen_Option__Start_Game)
        {
            text_color = state->blink_color;
        }

        std::string start_text = "Start";
        float start_text_scale = 1.0f / FONT_SCALE_FACTOR;

        // Compute the total text width
        float start_text_width = 0.0f;
        for (char c : start_text)
        {
            start_text_width += (Characters[c].Advance >> 6) * start_text_scale;  // Horizontal advance in pixels
        }

        // Adjust for centering
        float start_x = start_initial_x - (start_text_width / 2.0f);
        float start_y =
            start_initial_y + (Characters['H'].Size.y * start_text_scale);  // Use a sample character for height

        RenderText(*global_text_shader, start_text, start_x, start_y, start_text_scale, text_color);
    }

    {  // Exit Text
        float exit_initial_x = LOGICAL_WIDTH * 3.0f / 4.0f;
        float exit_initial_y = LOGICAL_HEIGHT * 1.0f / 4.0f;
        glm::vec3 text_color = white;
        if (state->current_option == Start_Screen_Option__Exit_Game)
        {
            text_color = state->blink_color;
        }

        std::string exit_text = "Exit";
        float exit_text_scale = 1.0f / FONT_SCALE_FACTOR;

        // Compute the total text width
        float exit_text_width = 0.0f;
        for (char c : exit_text)
        {
            exit_text_width += (Characters[c].Advance >> 6) * exit_text_scale;  // Horizontal advance in pixels
        }

        // Adjust for centering
        float exit_x = exit_initial_x - (exit_text_width / 2.0f);
        float exit_y =
            exit_initial_y + (Characters['H'].Size.y * exit_text_scale);  // Use a sample character for height

        RenderText(*global_text_shader, exit_text, exit_x, exit_y, exit_text_scale, text_color);
    }
}