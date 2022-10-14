#ifdef RENDER_SDL2

#include "common/meta.h"
#include "window_SDL2.h"
#include "graphics_SDL2.h"
#include "SDL.h"
#include "SDL_Image.h"
#include "SDL_ttf.h"
#include "SDL_syswm.h"
#include "game/graphics/video.h"
#include "common/log.h"
#include <string>
#include "config/config_mgr.h"
#include "common/sysutil.h"
#include "imgui.h"
#include "backends/imgui_impl_sdl.h"
#include "backends/imgui_impl_sdlrenderer.h"

static SDL_Rect canvasRect;
static SDL_Rect windowRect;

int graphics_init()
{
    // SDL2
    {
        if (SDL_Init(SDL_INIT_VIDEO))
        {
            LOG_ERROR << "[SDL2] Library init ERROR! " << SDL_GetError();
            return -99;
        }
        LOG_INFO << "[SDL2] Library version " << SDL_MAJOR_VERSION << '.' << SDL_MINOR_VERSION << "." << SDL_PATCHLEVEL;

        std::string title;
        title += PROJECT_NAME;
#ifdef _DEBUG
        title += ' ';
        title += "Debug";
#endif
        title += ' ';
        title += PROJECT_VERSION;

#if _WIN32
        // Direct3D9
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "direct3d");
#else
        //SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
#endif

        SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");

        Uint32 flags = SDL_WINDOW_HIDDEN;   // window created with opengles2 will destroy itself when creating Renderer, resulting in flashes
#ifdef _DEBUG
        flags |= SDL_WINDOW_RESIZABLE;
#endif
        auto mode = ConfigMgr::get("V", cfg::V_WINMODE, cfg::V_WINMODE_WINDOWED);
        if (strEqual(mode, cfg::V_WINMODE_BORDERLESS, true))
        {
            flags |= SDL_WINDOW_BORDERLESS;
        }
        else if (strEqual(mode, cfg::V_WINMODE_FULL, true))
        {
            flags |= SDL_WINDOW_FULLSCREEN;
        }
        else 
        {
            // fallback to windowed
        }
        windowRect.w = ConfigMgr::get("V", cfg::V_DISPLAY_RES_X, CANVAS_WIDTH);
        windowRect.h = ConfigMgr::get("V", cfg::V_DISPLAY_RES_Y, CANVAS_HEIGHT);
        gFrameWindow = SDL_CreateWindow(title.c_str(),
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowRect.w, windowRect.h, flags);
        if (!gFrameWindow)
        {
            LOG_ERROR << "[SDL2] Init window ERROR! " << SDL_GetError();
            return -1;
        }

        canvasRect.w = ConfigMgr::get("V", cfg::V_RES_X, CANVAS_WIDTH);
        canvasRect.h = ConfigMgr::get("V", cfg::V_RES_Y, CANVAS_HEIGHT);
        graphics_resize_canvas(canvasRect.w, canvasRect.h);

        int ss = ConfigMgr::get("V", cfg::V_RES_SUPERSAMPLE, 1);
        graphics_set_supersample_level(ss);

        int maxFPS = ConfigMgr::get("V", cfg::V_MAXFPS, 480);
        if (maxFPS < 30 && maxFPS != 0)
            maxFPS = 30;
        graphics_set_maxfps(maxFPS);

        if (ConfigMgr::get("V", cfg::V_VSYNC, false))
        {
            gFrameRenderer = SDL_CreateRenderer(
                gFrameWindow, -1,
                SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
        }
        else
        {
            gFrameRenderer = SDL_CreateRenderer(
                gFrameWindow, -1,
                SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
        }
        if (!gFrameRenderer)
        {
            LOG_ERROR << "[SDL2] Init renderer ERROR! " << SDL_GetError();
            return -2;
        }

        SDL_ShowWindow(gFrameWindow);

        SDL_SysWMinfo wmInfo;
        SDL_VERSION(&wmInfo.version);
        SDL_GetWindowWMInfo(gFrameWindow, &wmInfo);

#if _WIN32 || _WIN64
        setWindowHandle((void*)&wmInfo.info.win.window);
#endif

        gInternalRenderTarget = SDL_CreateTexture(gFrameRenderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_TARGET,
            3840, 2160);
        if (!gInternalRenderTarget)
        {
            LOG_ERROR << "[SDL2] Init Target Texture Error! " << SDL_GetError();
            return -3;
        }
        SDL_SetTextureScaleMode(gInternalRenderTarget, SDL_ScaleModeBest);

        SDL_SetRenderTarget(gFrameRenderer, gInternalRenderTarget);

        SDL_ShowCursor(SDL_DISABLE);

        SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);

        LOG_DEBUG << "[SDL2] Initializing window and render complete.";
    }

    // SDL_Image
    {
        auto flags = IMG_INIT_JPG | IMG_INIT_PNG;
        if (flags != IMG_Init(flags))
        {
            // error handling
            LOG_ERROR << "[SDL2] Image module init failed. " << IMG_GetError();
            return 1;
        }
        LOG_INFO << "[SDL2] Image module version " << SDL_IMAGE_MAJOR_VERSION << '.' << SDL_IMAGE_MINOR_VERSION << "." << SDL_IMAGE_PATCHLEVEL;
    }
    // SDL_ttf
    {
        if (-1 == TTF_Init())
        {
            // error handling
            LOG_ERROR << "[SDL2] TTF module init failed. " << TTF_GetError();
            return 2;
        }
        LOG_INFO << "[SDL2] TTF module version " << SDL_TTF_MAJOR_VERSION << '.' << SDL_TTF_MINOR_VERSION << "." << SDL_TTF_PATCHLEVEL;
    }

#ifndef VIDEO_DISABLED
	// libav
	video_init();
#endif

    // imgui
    ImGui_ImplSDL2_InitForSDLRenderer(gFrameWindow, gFrameRenderer);
    ImGui_ImplSDLRenderer_Init(gFrameRenderer);

    // Draw a black frame to prevent flashbang
    graphics_clear();
    graphics_flush();

    return 0;
}

void graphics_clear()
{
    SDL_RenderClear(gFrameRenderer);
}

static int maxFPS = 0;
static std::chrono::nanoseconds desiredFrameTimeBetweenFrames;
static std::chrono::high_resolution_clock::time_point frameTimestampPrev;
void graphics_flush()
{
    SDL_SetRenderTarget(gFrameRenderer, NULL);
    {
        // TODO scale internal canvas
        Rect ssRect = canvasRect;
        int ssLevel = graphics_get_supersample_level();
        ssRect.w *= ssLevel;
        ssRect.h *= ssLevel;

        // render internal canvas texture
        SDL_RenderCopyEx(
            gFrameRenderer, gInternalRenderTarget,
            &ssRect, &windowRect,
            0, NULL, SDL_FLIP_NONE);

        // render imgui
        auto pData = ImGui::GetDrawData();
        if (pData != NULL)
        {
            if (pData->CmdListsCount == 0)
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_None);
            }
            ImGui_ImplSDLRenderer_RenderDrawData(pData);
        }

        SDL_RenderPresent(gFrameRenderer);
    }
    SDL_SetRenderTarget(gFrameRenderer, gInternalRenderTarget);

    if (maxFPS != 0)
    {
        long long sleep_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            frameTimestampPrev + desiredFrameTimeBetweenFrames - std::chrono::high_resolution_clock::now()).count();
        preciseSleep(sleep_ns);
    }
    frameTimestampPrev = std::chrono::high_resolution_clock::now();
}

int graphics_free()
{
    ImGui_ImplSDLRenderer_Shutdown();
    ImGui_ImplSDL2_Shutdown();

    SDL_DestroyTexture(gInternalRenderTarget);
    gInternalRenderTarget = nullptr;
    SDL_DestroyRenderer(gFrameRenderer);
    gFrameRenderer = nullptr;
    SDL_DestroyWindow(gFrameWindow);
    gFrameWindow = nullptr;
    TTF_Quit();
    IMG_Quit();
	SDL_Quit();

    return 0;
}

void graphics_change_window_mode(int mode)
{
    switch (mode)
    {
    case 0:
        SDL_SetWindowFullscreen(gFrameWindow, 0);
        SDL_SetWindowBordered(gFrameWindow, SDL_TRUE);
        break;
    case 1:
        SDL_SetWindowFullscreen(gFrameWindow, SDL_WINDOW_FULLSCREEN);
        break;
    case 2:
        SDL_SetWindowFullscreen(gFrameWindow, 0);
        SDL_SetWindowBordered(gFrameWindow, SDL_FALSE);
        break;
    }
}

void graphics_resize_window(int x, int y)
{
    if (x != 0) windowRect.w = x;
    if (y != 0) windowRect.h = y;
    SDL_SetWindowSize(gFrameWindow, windowRect.w, windowRect.h);
    SDL_SetWindowPosition(gFrameWindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
}

void graphics_change_vsync(int mode)
{
#if _WIN32
    SDL_RenderSetVSync(gFrameRenderer, mode);
#else
    // codes below should work since we are explicitly indicated to use OpenGL backend
    SDL_GL_SetSwapInterval(mode == 2 ? -1 : mode);
#endif
}

static int superSampleLevel = 1;
void graphics_set_supersample_level(int level)
{
    assert(canvasRect.w * level <= 3840);
    superSampleLevel = level;
}
int graphics_get_supersample_level()
{
    return superSampleLevel;
}

static double canvasScaleX = 1.0;
static double canvasScaleY = 1.0;
void graphics_resize_canvas(int x, int y)
{
    canvasRect.w = x;
    canvasRect.h = y;
    canvasScaleX = (double)windowRect.w / x;
    canvasScaleY = (double)windowRect.h / y;
}
double graphics_get_canvas_scale_x() { return canvasScaleX; }
double graphics_get_canvas_scale_y() { return canvasScaleY; }


void graphics_set_maxfps(int fps)
{
    maxFPS = fps;
    if (maxFPS != 0)
    {
        desiredFrameTimeBetweenFrames = std::chrono::nanoseconds(long long(std::round(1e9 / maxFPS)));
    }
}

///////////////////////////////////////////////////////////////////////////

extern bool gEventQuit;

static bool isEditing = false;
static std::string textBuf, textBufSuffix;
void funEditing(const SDL_TextEditingEvent& e);
void funInput(const SDL_TextInputEvent& e);
void funKeyDown(const SDL_KeyboardEvent& e);

void event_handle()
{
    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
        ImGui_ImplSDL2_ProcessEvent(&e);

        switch (e.type)
        {
        case SDL_QUIT:
            gEventQuit = true;
            break;

        case SDL_SYSWMEVENT:
#ifdef WIN32
            callWMEventHandler(
                &e.syswm.msg->msg.win.hwnd,
                &e.syswm.msg->msg.win.msg,
                &e.syswm.msg->msg.win.wParam,
                &e.syswm.msg->msg.win.lParam);
#elif defined __linux__

#endif
            break;

        case SDL_WINDOWEVENT:
            switch (e.window.event)
            {
            case SDL_WINDOWEVENT_FOCUS_GAINED:
                SetWindowForeground(true);
                break;
            case SDL_WINDOWEVENT_FOCUS_LOST:
                SetWindowForeground(false);
                break;
            default:
                break;
            }
            break;

        case SDL_TEXTINPUT:
            if (isEditing) funInput(e.text);
            break;

        case SDL_TEXTEDITING:
            if (isEditing) funEditing(e.edit);
            break;

        case SDL_KEYDOWN:
            if (isEditing) funKeyDown(e.key);
            break;

        default:
            break;
        }
    }
}

void ImGuiNewFrame()
{
    SDL_SetRenderTarget(gFrameRenderer, NULL);
    ImGui_ImplSDLRenderer_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    SDL_SetRenderTarget(gFrameRenderer, gInternalRenderTarget);
}

static std::function<void(const std::string&)> funUpdateText;
void startTextInput(const Rect& textBox, const std::string& oldText, std::function<void(const std::string&)> funUpdateText)
{
    textBuf = oldText;
    textBuf.reserve(32);

    ::funUpdateText = funUpdateText;

    SDL_Rect r;
    r.x = textBox.x;
    r.y = textBox.y;
    r.w = textBox.w;
    r.h = textBox.h;
    SDL_SetTextInputRect(&r);
    SDL_StartTextInput();
    isEditing = true;

    funUpdateText(textBuf);
}

void stopTextInput()
{
    isEditing = false;
    funUpdateText = [](const std::string&) {};
    SDL_StopTextInput();
}


void funEditing(const SDL_TextEditingEvent& e)
{
    LOG_DEBUG << "Editing " << e.start << " " << e.length << " " << e.text;
    if (strlen(e.text) == 0)
    {
        textBuf += textBufSuffix;
        textBufSuffix.clear();
        funUpdateText(textBuf);
    }
    else
    {
        if (e.length > 0)
        {
            textBufSuffix = textBuf.substr(e.start + e.length);
            textBuf.erase(e.start, textBuf.npos);
        }
        funUpdateText(textBuf + e.text + textBufSuffix);
    }
}

void funInput(const SDL_TextInputEvent& e)
{
    textBuf = textBuf + e.text + textBufSuffix;
    textBufSuffix.clear();
    funUpdateText(textBuf);
}

void funKeyDown(const SDL_KeyboardEvent& e)
{
    if (e.keysym.mod & (KMOD_LCTRL | KMOD_RCTRL))
    {
        switch (e.keysym.sym)
        {
        case SDLK_v:
        {
            std::stringstream ss;
            char* pText = SDL_GetClipboardText();
            ss << pText;
            SDL_free(pText);
            std::string textLine;
            std::getline(ss, textLine);
            textBuf += textLine;
            funUpdateText(textBuf + textBufSuffix);
            break;
        }
        }
    }
    else
    {
        switch (e.keysym.sym)
        {
        case SDLK_BACKSPACE:
            if (textBufSuffix.empty() && !textBuf.empty())
            {
                std::u32string textTmp = utf8_to_utf32(textBuf);
                textTmp.erase(textTmp.length() - 1, 1);
                textBuf = utf32_to_utf8(textTmp);
                funUpdateText(textBuf + textBufSuffix);
            }
            break;
        }
    }
}

#endif