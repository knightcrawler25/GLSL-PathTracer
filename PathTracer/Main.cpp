#define _USE_MATH_DEFINES

// third-party libraries
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <SDL.h>
#include <GLES3/gl3.h>
#else
#include <SDL2/SDL.h>
#include <GL/gl3w.h>
#endif

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <time.h>
#include <math.h>

#include "Scene.h"
#include "TiledRenderer.h"
#include "Camera.h"
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"

#include "boyTestScene.h"
#include "ajaxTestScene.h"
#include "cornellTestScene.h"

using namespace glm;
using namespace std;
using namespace GLSLPT;

float moveSpeed = 0.5f;
float mouseSensitivity = 1.0f;
bool keyPressed = false;
Scene *scene = nullptr;
Renderer *renderer = nullptr;
int currentSceneIndex = 2;
double lastTime = SDL_GetTicks(); //glfwGetTime();
bool done = false;


RenderOptions renderOptions;
#ifdef __EMSCRIPTEN__
EM_JS(void, _HideLoader, (), {
    document.getElementById("loader").style.display = "none";
    });
void HideLoader() { _HideLoader(); }
#endif

struct LoopData
{
    SDL_Window*             mWindow = nullptr;
    SDL_GLContext           mGLContext = nullptr;
};

void loadScene(int index)
{
    delete scene;
	//scene = LoadScene(std::string("./assets/")+sceneFilenames[index]);
	scene = new Scene();
	switch (index)
	{
		case 0:	loadAjaxTestScene(scene, renderOptions);
				break;
		case 1:	loadBoyTestScene(scene, renderOptions);
				break;
		case 2:	loadCornellTestScene(scene, renderOptions);
			break;
	}
	
    scene->renderOptions = renderOptions;
	if (!scene)
	{
		std::cout << "Unable to load scene\n";
		exit(0);
	}
	std::cout << "Scene Loaded\n\n";
}

bool initRenderer()
{
    delete renderer;
#ifdef __EMSCRIPTEN__
	renderer = new TiledRenderer(scene, "./shaders/");
#else
    renderer = new TiledRenderer(scene, "../PathTracer/shaders/");
#endif
    renderer->init();
    return true;
}

void render()
{
	renderer->render();
    const glm::ivec2 screenSize = renderer->getScreenSize();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, screenSize.x, screenSize.y);
    renderer->present();

    // Rendering
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void update(float secondsElapsed)
{
	renderer->update(secondsElapsed);
	keyPressed = false;
	//Camera Movement
	scene->camera->isMoving = false;
	if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) && ImGui::IsAnyMouseDown())
	{
		if (ImGui::IsMouseDown(0))
		{
			ImVec2 mouseDelta = ImGui::GetMouseDragDelta(0, 0);
			scene->camera->offsetOrientation(mouseSensitivity * mouseDelta.x, mouseSensitivity * mouseDelta.y);
			ImGui::ResetMouseDragDelta(0);
		}
		else if (ImGui::IsMouseDown(2))
		{
			ImVec2 mouseDelta = ImGui::GetMouseDragDelta(2, 0);
			scene->camera->strafe(0.01 * mouseDelta.x, 0.01 * mouseDelta.y);
			ImGui::ResetMouseDragDelta(2);
		}
		else if (ImGui::IsMouseDown(1))
		{
			ImVec2 mouseDelta = ImGui::GetMouseDragDelta(1, 0);
			scene->camera->changeRadius(mouseSensitivity * 0.01 * mouseDelta.y);
			ImGui::ResetMouseDragDelta(1);
		}
		scene->camera->isMoving = true;
	}
}


void MainLoop(void* arg)
{
    LoopData &loopdata = *(LoopData*)arg;

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT)
        {
            done = true;
        }
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
            event.window.windowID == SDL_GetWindowID(loopdata.mWindow))
        {
            done = true;
        }
    }

    ImGui_ImplSDL2_ProcessEvent(&event);
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(loopdata.mWindow);
    ImGui::NewFrame();

    {
        ImGui::Begin("GLSL PathTracer"); // Create a window called "Hello, world!" and append into it.

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        if (ImGui::Combo("Scene", &currentSceneIndex, "Ajax Bust\0Substance Boy\0Cornell Box\0"))
        {
            loadScene(currentSceneIndex);
            initRenderer();
        }

        bool renderOptionsChanged = false;
        //renderOptionsChanged |= ImGui::Combo("Render Type", &renderOptions.rendererType, "Progressive\0Tiled\0");
        //renderOptionsChanged |= ImGui::InputInt2("Resolution", &renderOptions.resolution.x);
        //renderOptionsChanged |= ImGui::InputInt("Max Samples", &renderOptions.maxSamples);
        renderOptionsChanged |= ImGui::InputInt("Max Depth", &renderOptions.maxDepth);
        //renderOptionsChanged |= ImGui::InputInt("Tiles X", &renderOptions.numTilesX);
        //renderOptionsChanged |= ImGui::InputInt("Tiles Y", &renderOptions.numTilesY);
        renderOptionsChanged |= ImGui::Checkbox("Use envmap", &renderOptions.useEnvMap);
        renderOptionsChanged |= ImGui::InputFloat("HDR multiplier", &renderOptions.hdrMultiplier);

        if (renderOptionsChanged)
        {
            scene->renderOptions = renderOptions;
            initRenderer();
        }
        ImGui::End();
    }

    double presentTime = SDL_GetTicks();
    update((float)(presentTime - lastTime));
    lastTime = presentTime;
    glClearColor(0., 0., 0., 0.);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    render();
    SDL_GL_SwapWindow(loopdata.mWindow);
}

int main(int argc, char** argv)
{
	srand((unsigned int)time(0));

    
	loadScene(currentSceneIndex);

    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    LoopData loopdata;

#ifdef __EMSCRIPTEN__
    const char* glsl_version = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif __APPLE__
    // GL 3.2 Core + GLSL 150
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#endif

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_DisplayMode current;
    SDL_GetCurrentDisplayMode(0, &current);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    loopdata.mWindow = SDL_CreateWindow("GLSL PathTracer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
#ifndef __EMSCRIPTEN__
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
    //glThreadContext = SDL_GL_CreateContext(loopdata.mWindow);
    //glThreadWindow = loopdata.mWindow;
#endif
    loopdata.mGLContext = SDL_GL_CreateContext(loopdata.mWindow);
    if (!loopdata.mGLContext)
    {
        fprintf(stderr, "Failed to initialize GL context!\n");
        return 1;
    }
    SDL_GL_SetSwapInterval(0); // Disable vsync

    // Initialize OpenGL loader
#ifndef __EMSCRIPTEN__
#if GL_VERSION_3_2
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
    bool err = gl3wInit() != 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
    bool err = glewInit() != GLEW_OK;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
    bool err = gladLoadGL() == 0;
#endif
    if (err)
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return 1;
    }
#endif
#endif // __EMSCRIPTEN__

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls

    ImGui_ImplSDL2_InitForOpenGL(loopdata.mWindow, loopdata.mGLContext);

    //InitFonts();

    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls

    ImGui_ImplOpenGL3_Init(glsl_version);


    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();


    if (!initRenderer())
        return 1;


#ifdef __EMSCRIPTEN__
    HideLoader();
    // This function call won't return, and will engage in an infinite loop, processing events from the browser, and dispatching them.
    emscripten_set_main_loop_arg(MainLoop, &loopdata, 0, true);
#else   
    while (!done)
    {
        MainLoop(&loopdata);
    }
        

    delete renderer;
    delete scene;

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(loopdata.mGLContext);
    SDL_DestroyWindow(loopdata.mWindow);
    SDL_Quit();
#endif
    return 0;
}

