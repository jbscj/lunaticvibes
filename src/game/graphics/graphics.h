#pragma once

// This header intends to provide a Render Layer Selection function,
// Which layer is used is decided by pre-defined macro.
// e.g.
//      #ifdef SDL2
//          #include "graphics_SDL2.h"
//      #elif defined SFML
//          #include "graphics_SFML.h"
//      #elif defined OGLNative
//          #include "graphics_OpenGL.h"
//      #endif

#ifdef RENDER_SDL2
#include "SDL2/graphics_SDL2.h"
#endif

int graphics_init();
void graphics_clear();
void graphics_flush();
int graphics_free();

// 0: windowed / 1: fullscreen / 2: borderless
void graphics_change_window_mode(int mode);

void graphics_resize_window(int x, int y);

void graphics_change_vsync(bool enable);


// scaling functions
void graphics_set_supersample_level(int scale);
int  graphics_get_supersample_level();

void graphics_resize_canvas(int x, int y);
double graphics_get_canvas_scale_x();
double graphics_get_canvas_scale_y();


void graphics_set_maxfps(int fps);

void event_handle();

void ImGuiNewFrame();


// text input support
void startTextInput(const Rect& textBox, const std::string& oldText, std::function<void(const std::string&)> funUpdateText);
void stopTextInput();