#define _USE_MATH_DEFINES

// third-party libraries
#include <GL/glew.h>
#include <GLFW/glfw3.h>	
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <time.h>
#include <math.h>

#include "Scene.h"
#include "TiledRenderer.h"
#include "ProgressiveRenderer.h"
#include "Camera.h"

using namespace glm;
using namespace std;
using namespace GLSLPathTracer;

float moveSpeed = 0.5f;
float mouseSensitivity = 0.05f;
double prevMouseX = 0, prevMouseY = 0;
bool isCameraMoving = true;
bool keyPressed = false;
Scene *scene = NULL;
Renderer *renderer;

void initScene()
{
	scene = LoadScene("./assets/ajax.scene");

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
	if(scene->renderOptions.rendererType.compare("Tiled") == 0)
		renderer = new TiledRenderer(scene, "./PathTracer/src/shaders/Tiled/");
	else if (scene->renderOptions.rendererType.compare("Progressive") == 0)
		renderer = new ProgressiveRenderer(scene, "./PathTracer/src/shaders/Progressive/");
	else
	{
		Log("Invalid Renderer Type\n");
        return false;
	}
    return true;
}

void render(GLFWwindow *window)
{
	renderer->render();
    const glm::ivec2 screenSize = renderer->getScreenSize();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, screenSize.x, screenSize.y);
    renderer->present();
	glfwSwapBuffers(window);
}

void update(float secondsElapsed, GLFWwindow *window)
{
	renderer->update(secondsElapsed);
	keyPressed = false;
	//Camera Movement
	if (glfwGetKey(window, 'W')){
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

	//Mouse Handling
	scene->camera->isMoving = false;
	double mouseX, mouseY;
	glfwGetCursorPos(window, &mouseX, &mouseY);
	if ( mouseX != prevMouseX || mouseY != prevMouseY || keyPressed)
		scene->camera->isMoving = true;
	
	prevMouseX = mouseX;
	prevMouseY = mouseY;
	scene->camera->offsetOrientation(mouseSensitivity * (float)mouseX, mouseSensitivity * (float)mouseY);

	glfwSetCursorPos(window, 0, 0);
}

void main()
{
	srand(unsigned int(time(0)));
	initScene();

	GLFWwindow *window;
	glfwInit();
	window = glfwCreateWindow((int)scene->renderOptions.resolution.x, (int)scene->renderOptions.resolution.y, "PathTracer", 0, 0);
	glfwSetWindowPos(window, 300, 100);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPos(window, 0, 0);
	glfwMakeContextCurrent(window);
	glfwSwapInterval(0);
	glewInit();

    if (!initRenderer())
        return;

	double lastTime = glfwGetTime();
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		if (glfwGetKey(window, GLFW_KEY_ESCAPE))
			glfwSetWindowShouldClose(window, GL_TRUE);
		double presentTime = glfwGetTime();
		update((float)(presentTime - lastTime), window);
		lastTime = presentTime;

		render(window);
	}

    renderer->finish();
    delete renderer;

	glfwTerminate();
}

