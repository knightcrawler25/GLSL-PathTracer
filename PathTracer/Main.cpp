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
#include "ProgressiveRenderer.h"
#include "Camera.h"
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"


using namespace glm;
using namespace std;
using namespace GLSLPathTracer;

float moveSpeed = 0.5f;
float mouseSensitivity = 0.05f;
bool keyPressed = false;
Scene *scene = nullptr;
Renderer *renderer = nullptr;
int currentSceneIndex = 0;
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
    static const char *sceneFilenames[] = { "cornell.scene",
        "ajax.scene",
        "bathroom.scene",
        "boy.scene",
        "coffee.scene",
        "diningroom.scene",
        "glassBoy.scene",
        "hyperion.scene",
        "rank3police.scene",
        "spaceship.scene",
        "staircase.scene" };

    delete scene;
	scene = LoadScene(std::string("./assets/")+sceneFilenames[index]);
    scene->renderOptions = renderOptions;
	if (!scene)
	{
		std::cout << "Unable to load scene\n";
		exit(0);
	}
	std::cout << "Scene Loaded\n\n";

	scene->buildBVH();

	// --------Print info on memory usage ------------- //

	std::cout << "Triangles: " << scene->triangleIndices.size() << std::endl;
	std::cout << "Triangle Indices: " << scene->gpuBVH->bvhTriangleIndices.size() << std::endl;
	std::cout << "Vertices: " << scene->vertexData.size() << std::endl;

	long long scene_data_bytes =
		sizeof(GPUBVHNode) * scene->gpuBVH->bvh->getNumNodes() +
		sizeof(TriangleData) * scene->gpuBVH->bvhTriangleIndices.size() +
		sizeof(VertexData) * scene->vertexData.size() +
		sizeof(NormalTexData) * scene->normalTexData.size() +
		sizeof(MaterialData) * scene->materialData.size() +
		sizeof(LightData) * scene->lightData.size();

	std::cout << "GPU Memory used for BVH and scene data: " << scene_data_bytes / 1048576 << " MB" << std::endl;

	long long tex_data_bytes =
		int(scene->texData.albedoTextureSize.x * scene->texData.albedoTextureSize.y) * scene->texData.albedoTexCount * 3 +
		int(scene->texData.metallicRoughnessTextureSize.x * scene->texData.metallicRoughnessTextureSize.y) * scene->texData.metallicRoughnessTexCount * 3 +
		int(scene->texData.normalTextureSize.x * scene->texData.normalTextureSize.y) * scene->texData.normalTexCount * 3 +
		scene->hdrLoaderRes.width * scene->hdrLoaderRes.height * sizeof(GL_FLOAT) * 3;

	std::cout << "GPU Memory used for Textures: " << tex_data_bytes / 1048576 << " MB" << std::endl;

	std::cout << "Total GPU Memory used: " << (scene_data_bytes + tex_data_bytes) / 1048576 << " MB" << std::endl;

	// ----------------------------------- //
}

bool initRenderer()
{
    delete renderer;
    if (scene->renderOptions.rendererType == Renderer_Tiled)
    {
        renderer = new TiledRenderer(scene, "../PathTracer/shaders/Tiled/");
    }
    else if (scene->renderOptions.rendererType == Renderer_Progressive)
    {
        renderer = new ProgressiveRenderer(scene, "../PathTracer/shaders/Progressive/");
    }
	else
	{
		Log("Invalid Renderer Type\n");
        return false;
	}
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
	/*if (glfwGetKey(window, 'W')){
		scene->camera->offsetPosition(secondsElapsed * moveSpeed * scene->camera->forward);
		keyPressed = true;
	}
	else if (glfwGetKey(window, 'S')){
		scene->camera->offsetPosition(secondsElapsed * moveSpeed * -scene->camera->forward);
		keyPressed = true;
	}
    if (glfwGetKey(window, 'A')){
		scene->camera->offsetPosition(secondsElapsed * moveSpeed * -scene->camera->right);
		keyPressed = true;
	}
	else if (glfwGetKey(window, 'D')){
		scene->camera->offsetPosition(secondsElapsed * moveSpeed * scene->camera->right);
		keyPressed = true;
	}
    */
	//Mouse Handling
	scene->camera->isMoving = false;
    if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) && ImGui::IsMouseDown(0))
    {
        ImVec2 mouseDelta = ImGui::GetMouseDragDelta();
        scene->camera->offsetOrientation(mouseSensitivity * mouseDelta.x, mouseSensitivity * mouseDelta.y);
        scene->camera->isMoving = true;
        ImGui::ResetMouseDragDelta();
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
        ImGui::Begin("GLSL PathTracer");                          // Create a window called "Hello, world!" and append into it.

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        if (ImGui::Combo("Scene", &currentSceneIndex, "cornell\0ajax\0bathroom\0boy\0coffee\0diningroom\0glassBoy\0hyperion\0rank3police\0spaceship\0staircase\0"))
        {
            loadScene(currentSceneIndex);
            initRenderer();
        }

        bool renderOptionsChanged = false;
        renderOptionsChanged |= ImGui::Combo("Render Type", &renderOptions.rendererType, "Progressive\0Tiled\0");
        renderOptionsChanged |= ImGui::InputInt2("Resolution", &renderOptions.resolution.x);
        renderOptionsChanged |= ImGui::InputInt("Max Samples", &renderOptions.maxSamples);
        renderOptionsChanged |= ImGui::InputInt("Max Depth", &renderOptions.maxDepth);
        renderOptionsChanged |= ImGui::InputInt("Tiles X", &renderOptions.numTilesX);
        renderOptionsChanged |= ImGui::InputInt("Tiles Y", &renderOptions.numTilesY);
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
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_MAXIMIZED);
    loopdata.mWindow = SDL_CreateWindow("pathtrace", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
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
    SDL_GL_SetSwapInterval(1); // Enable vsync

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

