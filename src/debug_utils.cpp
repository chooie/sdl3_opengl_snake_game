bool32 has_initiated_draw = 0;

#define DEBUG_TEXT_STRING_LENGTH 100

void print_debug_info_text_to_console(Master_Timer timer)
{
    {  // FPS
        real32 fps = 1.0f / timer.total_frame_time_elapsed__seconds;
        printf("FPS: %.1f, ", fps);
    }

    real32 ms_per_frame = timer.total_frame_time_elapsed__seconds * 1000.0f;
    {  // Total Frame Time (MS)
        printf("Ms/frame: %.04f (Target: %.04f), ", ms_per_frame, TARGET_TIME_PER_FRAME_MS);
    }

    {  // Work Frame Time (MS)
        real32 work_ms_per_frame = timer.time_elapsed_for_work__seconds * 1000.0f;
        printf("Work ms: %.04f, (%.1f%%), ", work_ms_per_frame, (work_ms_per_frame / ms_per_frame) * 100);
    }

    {  // Buffer Ms
        real32 writing_buffer_ms_per_frame = timer.time_elapsed_for_writing_buffer__seconds * 1000.0f;
        printf("Buffer ms: %.04f, (%.1f%%), ",
               writing_buffer_ms_per_frame,
               (writing_buffer_ms_per_frame / ms_per_frame) * 100);
    }

    {  // Render Frame Time (MS)
        real32 render_ms_per_frame = timer.time_elapsed_for_render__seconds * 1000.0f;
        printf("Render ms: %.04f, (%.1f%%), ", render_ms_per_frame, (render_ms_per_frame / ms_per_frame) * 100);
    }

    {  // Sleep Frame Time (MS)
        real32 sleep_ms_per_frame = timer.time_elapsed_for_sleep__seconds * 1000.0f;
        printf("Sleep ms: %.04f, (%.1f%%)", sleep_ms_per_frame, (sleep_ms_per_frame / ms_per_frame) * 100);
    }

    printf("\n");

#if 0
    printf("X Grids: %d, Y Grids: %d, X: %d, Y: %d\n", X_GRIDS, Y_GRIDS, state.blip_pos_x, state.blip_pos_y);
#endif
}

#ifdef _WIN32
    char mega_cycles_text[DEBUG_TEXT_STRING_LENGTH] = "";
    char work_mega_cycles_text[DEBUG_TEXT_STRING_LENGTH] = "";
    char render_mega_cycles_text[DEBUG_TEXT_STRING_LENGTH] = "";
#endif
char fps_text[DEBUG_TEXT_STRING_LENGTH] = "";
char ms_per_frame_text[DEBUG_TEXT_STRING_LENGTH] = "";
char work_ms_per_frame_text[DEBUG_TEXT_STRING_LENGTH] = "";
char writing_buffer_ms_per_frame_text[DEBUG_TEXT_STRING_LENGTH] = "";
char render_ms_per_frame_text[DEBUG_TEXT_STRING_LENGTH] = "";
char sleep_ms_per_frame_text[DEBUG_TEXT_STRING_LENGTH] = "";

void display_debug_info_text(Master_Timer timer)
{
    if (!has_initiated_draw)
    {
        // NOTE(choo): Doing this so the debug info displays instantly
        has_initiated_draw = 1;
        global_debug_counter = 0;
    }
    // TODO: remove this when changing the win32 stuff
    glm::vec3 debug_text_color = {1.0f, 1.0f, 1.0f};  // White color
    real32 debug_text_scale = 0.5f / FONT_SCALE_FACTOR;
    real32 height_offset = Characters['H'].Size.y * debug_text_scale; 
    real32 padding = LOGICAL_WIDTH * 0.02f;
    real32 x_pos = padding;
    real32 y_pos = LOGICAL_HEIGHT - (height_offset + padding);
    real32 vertical_offset = height_offset + (padding / 2);

#ifdef _WIN32
    {  // Mega Cycles Per Frame
        real64 mega_cycles_per_frame = global_LAST_total_cycles_elapsed / (1000.0f * 1000.0f);

        if (global_debug_counter == 0)
        {
            snprintf(mega_cycles_text, sizeof(mega_cycles_text), "Mega cycles/Frame: %.02f", mega_cycles_per_frame);
        }

        RenderText(*global_text_shader, mega_cycles_text, x_pos, y_pos, debug_text_scale, debug_text_color);
        y_pos -= vertical_offset;
    }

    {  // Work Cycles Per Frame
        real64 mega_cycles_for_actual_work = global_LAST_cycles_elapsed_before_render / (1000.0f * 1000.0f);

        if (global_debug_counter == 0)
        {
            snprintf(work_mega_cycles_text,
                     sizeof(work_mega_cycles_text),
                     "Work mega cycles/Frame: %.02f",
                     mega_cycles_for_actual_work);
        }

        RenderText(*global_text_shader, work_mega_cycles_text, x_pos, y_pos, debug_text_scale, debug_text_color);
        y_pos -= vertical_offset;
    }

    {  // Render Cycles Per Frame
        real64 mega_cycles_for_render = global_LAST_cycles_elapsed_after_render / (1000.0f * 1000.0f);

        if (global_debug_counter == 0)
        {
            snprintf(render_mega_cycles_text,
                     sizeof(render_mega_cycles_text),
                     "Render mega cycles/Frame: %.02f",
                     mega_cycles_for_render);
        }

        RenderText(*global_text_shader, render_mega_cycles_text, x_pos, y_pos, debug_text_scale, debug_text_color);
        y_pos -= vertical_offset;
    }
#endif

    {  // FPS
        real32 fps = 1.0f / timer.total_frame_time_elapsed__seconds;

        if (global_debug_counter == 0)
        {
            snprintf(fps_text, sizeof(fps_text), "FPS: %.02f", fps);
        }

        RenderText(*global_text_shader, fps_text, x_pos, y_pos, debug_text_scale, debug_text_color);
        y_pos -= vertical_offset;
    }

    real32 ms_per_frame = timer.total_frame_time_elapsed__seconds * 1000.0f;

    {  // Total Frame Time (MS)
        if (global_debug_counter == 0)
        {
            snprintf(ms_per_frame_text,
                     sizeof(ms_per_frame_text),
                     "Ms/frame: %.04f (Target: %.04f)",
                     ms_per_frame,
                     TARGET_TIME_PER_FRAME_MS);
        }

        RenderText(*global_text_shader, ms_per_frame_text, x_pos, y_pos, debug_text_scale, debug_text_color);
        y_pos -= vertical_offset;
    }
    {  // Work Frame Time (MS)
        real32 work_ms_per_frame = timer.time_elapsed_for_work__seconds * 1000.0f;

        if (global_debug_counter == 0)
        {
            snprintf(work_ms_per_frame_text,
                     sizeof(work_ms_per_frame_text),
                     "Work ms: %.04f, (%.1f%%)",
                     work_ms_per_frame,
                     (work_ms_per_frame / ms_per_frame) * 100);
        }

        RenderText(*global_text_shader, work_ms_per_frame_text, x_pos, y_pos, debug_text_scale, debug_text_color);
        y_pos -= vertical_offset;
    }
    {  // Buffer Writing Time (MS)
        real32 writing_buffer_ms_per_frame = timer.time_elapsed_for_writing_buffer__seconds * 1000.0f;

        if (global_debug_counter == 0)
        {
            snprintf(writing_buffer_ms_per_frame_text,
                     sizeof(writing_buffer_ms_per_frame_text),
                     "Buffer ms: %.04f, (%.1f%%)",
                     writing_buffer_ms_per_frame,
                     (writing_buffer_ms_per_frame / ms_per_frame) * 100);
        }

        RenderText(*global_text_shader,
                   writing_buffer_ms_per_frame_text,
                   x_pos,
                   y_pos,
                   debug_text_scale,
                   debug_text_color);
        y_pos -= vertical_offset;
    }

    {  // Render Frame Time (MS)
        real32 render_ms_per_frame = timer.time_elapsed_for_render__seconds * 1000.0f;

        if (global_debug_counter == 0)
        {
            snprintf(render_ms_per_frame_text,
                     sizeof(render_ms_per_frame_text),
                     "Render ms: %.04f, (%.1f%%)",
                     render_ms_per_frame,
                     (render_ms_per_frame / ms_per_frame) * 100);
        }

        RenderText(*global_text_shader, render_ms_per_frame_text, x_pos, y_pos, debug_text_scale, debug_text_color);
        y_pos -= vertical_offset;
    }

    {  // Sleep Frame Time (MS)
        real32 sleep_ms_per_frame = timer.time_elapsed_for_sleep__seconds * 1000.0f;

        if (global_debug_counter == 0)
        {
            snprintf(sleep_ms_per_frame_text,
                     sizeof(sleep_ms_per_frame_text),
                     "Sleep ms: %.04f, (%.1f%%)",
                     sleep_ms_per_frame,
                     (sleep_ms_per_frame / ms_per_frame) * 100);
        }

        RenderText(*global_text_shader, sleep_ms_per_frame_text, x_pos, y_pos, debug_text_scale, debug_text_color);
        y_pos -= vertical_offset;
    }
}

void display_timer_in_window_name(Master_Timer timer)
{
    if (global_debug_counter == 0)
    {
        real32 fps = 1.0f / timer.total_frame_time_elapsed__seconds;

        char fps_str[30];  // Allocate enough space for the string
        snprintf(fps_str, 30, "%.2f", fps);

        char title_str[100] = "Snake Game (FPS: ";
        int index_to_start_adding = 0;
        while (title_str[index_to_start_adding] != '\0')
        {
            index_to_start_adding++;
        }

        for (uint32 i = 0; fps_str[i] != '\0'; i++)
        {
            title_str[index_to_start_adding++] = fps_str[i];
        }

        title_str[index_to_start_adding++] = ')';

        title_str[index_to_start_adding++] = '\0';

        SDL_SetWindowTitle(global_window, title_str);
            }
}

void tick_debug_counter(Master_Timer timer)
{
    global_debug_counter += timer.total_frame_time_elapsed__seconds;
    // Tick every second
    if (global_debug_counter >= 1.0f)
    {
        global_debug_counter = 0;
    }
}