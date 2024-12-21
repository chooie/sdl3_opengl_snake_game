enum
{
    BUTTON_W,
    BUTTON_A,
    BUTTON_S,
    BUTTON_D,
    BUTTON_UP,
    BUTTON_DOWN,
    BUTTON_LEFT,
    BUTTON_RIGHT,
    BUTTON_ENTER,

    BUTTON_SPACE,
    BUTTON_ESCAPE,

    BUTTON_COUNT,  // Should be the last item
};

struct Button_State
{
    bool32 is_down;
    bool32 changed;
};

struct Input
{
    Button_State buttons[BUTTON_COUNT];
};

#define is_down(b) input->buttons[b].is_down
#define is_down_local(b) input.buttons[b].is_down
#define pressed(b) input->buttons[b].is_down && input->buttons[b].changed
#define pressed_local(b) input.buttons[b].is_down&& input.buttons[b].changed
#define released(b) (!input->buttons[b].is_down && input->buttons[b].changed)

#define process_input(button, sdl_key)                                            \
    case sdl_key:                                                                 \
    {                                                                             \
        if (event->type == SDL_EVENT_KEY_DOWN)                                    \
        {                                                                         \
            input->buttons[button].changed = input->buttons[button].is_down == 0; \
            input->buttons[button].is_down = 1;                                   \
        }                                                                         \
        else                                                                      \
        {                                                                         \
            input->buttons[button].changed = input->buttons[button].is_down == 1; \
            input->buttons[button].is_down = 0;                                   \
        }                                                                         \
    }                                                                             \
    break;

void handle_events(SDL_Event* event, Input* input)
{
    for (int i = 0; i < BUTTON_COUNT; i++)
    {
        input->buttons[i].changed = false;
    }

    while (SDL_PollEvent(event))
    {
        switch (event->type)
        {
            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP:
            {
                switch (event->key.scancode)
                {
                    process_input(BUTTON_W, SDL_SCANCODE_W);
                    process_input(BUTTON_A, SDL_SCANCODE_A);
                    process_input(BUTTON_S, SDL_SCANCODE_S);
                    process_input(BUTTON_D, SDL_SCANCODE_D);
                    process_input(BUTTON_UP, SDL_SCANCODE_UP);
                    process_input(BUTTON_LEFT, SDL_SCANCODE_LEFT);
                    process_input(BUTTON_DOWN, SDL_SCANCODE_DOWN);
                    process_input(BUTTON_RIGHT, SDL_SCANCODE_RIGHT);
                    process_input(BUTTON_SPACE, SDL_SCANCODE_SPACE);
                    process_input(BUTTON_ENTER, SDL_SCANCODE_RETURN);
                    process_input(BUTTON_ESCAPE, SDL_SCANCODE_ESCAPE);
                }

                if (event->type == SDL_EVENT_KEY_UP)
                {
                    switch (event->key.scancode)
                    {
                        case SDL_SCANCODE_GRAVE:  // Backquote key
                        {
                            global_display_debug_info = !global_display_debug_info;
                        }
                        break;

                        case SDL_SCANCODE_F:
                        {
                            SDL_WindowFlags flags = SDL_GetWindowFlags(global_window);
                            bool32 is_fullscreen = flags & SDL_WINDOW_FULLSCREEN;
                            SDL_SetWindowFullscreen(global_window, !is_fullscreen);
                        } break;
                    }
                }
            } break;

            case SDL_EVENT_MOUSE_MOTION:
                // std::cout << "Mouse motion: dx = " << event->motion.xrel << ", dy = " << event->motion.yrel << std::endl;
                break;

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                // std::cout << "Mouse button pressed: button = " << (int)event->button.button << std::endl;
                break;

            case SDL_EVENT_MOUSE_BUTTON_UP:
                // std::cout << "Mouse button released: button = " << (int)event->button.button << std::endl;
                break;

            case SDL_EVENT_MOUSE_WHEEL:
                // std::cout << "Mouse wheel: delta = " << event->wheel.y << std::endl;
                break;

            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            case SDL_EVENT_QUIT:
            {
                global_running = 0;
            } break;
        }
    }
}