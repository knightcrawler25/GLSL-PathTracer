#define _USE_MATH_DEFINES

// third-party libraries
#include <GL/glew.h>
#include <GLFW/glfw3.h>	
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <time.h>
#include <math.h>
#include <Scene.h>
#include <TiledRenderer.h>
#include <ProgressiveRenderer.h>

using namespace glm;
using namespace std;

glm::vec2 screenSize(1280,720);
float moveSpeed = 0.5f;
float mouseSensitivity = 0.05f;
double prevMouseX = 0, prevMouseY = 0;
bool isCameraMoving = true;
bool keyPressed = false;
Scene *scene = NULL;
Renderer *renderer;

void init()
{
	scene = new Scene;

	if (!LoadScene(scene, "./assets/boy.scene"))
	{
		std::cout << "Unable to load scene\n";
		exit(0);
	}
	std::cout << "Scene Loaded\n\n";

	if(scene->renderOptions.rendererType.compare("Tiled") == 0)
		renderer = new TiledRenderer(scene, screenSize);
	else if (scene->renderOptions.rendererType.compare("Progressive") == 0)
		renderer = new ProgressiveRenderer(scene, screenSize);
	else
	{
		std::cout << "Invalid Renderer Type" << std::endl;
		exit(0);
	}
}

void render(GLFWwindow *window)
{
	renderer->render();
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
	srand(time(0));
	GLFWwindow *window;

	glfwInit();
	window = glfwCreateWindow((int)screenSize.x, (int)screenSize.y, "PathTracer", 0, 0);
	glfwSetWindowPos(window, 300, 100);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPos(window, 0, 0);
	glfwMakeContextCurrent(window);
	glfwSwapInterval(0);
	glewInit();
	init();

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
	glfwTerminate();
}

