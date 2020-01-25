#pragma once
#include <glm/glm.hpp>

namespace GLSLPT
{
	class Camera
	{
	public:
		Camera(glm::vec3 eye, glm::vec3 lookat, float fov);
		Camera(const Camera& other);
		Camera& operator = (const Camera& other);

		void offsetOrientation(float dx, float dy);
		void strafe(float dx, float dy);
		void changeRadius(float dr);
		void updateCamera();
		void computeViewProjectionMatrix(float* view, float* projection, float ratio);
		void setFov(float val);
		glm::vec3 position, pivot;
		glm::vec3 up;
		glm::vec3 right;
		glm::vec3 forward;
		glm::vec3 worldUp;
		float pitch, yaw, fov, focalDist, aperture, radius;
		bool isMoving;
	};
}