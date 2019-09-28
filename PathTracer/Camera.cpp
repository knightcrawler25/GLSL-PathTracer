#include <Camera.h>
#include <iostream>

namespace GLSLPT
{
	Camera::Camera(glm::vec3 eye, glm::vec3 lookat, float fov)
	{
		position = eye;
		pivot = lookat;
		worldUp = glm::vec3(0, 1, 0);

		glm::vec3 dir = glm::normalize(pivot - position);
		pitch = glm::degrees(asin(dir.y));
		yaw = glm::degrees(atan2(dir.z, dir.x));

		radius = glm::distance(eye, lookat);

		this->fov = glm::radians(fov);
		focalDist = 0.1f;
		aperture = 0.0;
		updateCamera();
	}

	Camera::Camera(const Camera& other)
	{
		*this = other;
	}

	Camera& Camera::operator = (const Camera& other)
	{
		ptrdiff_t l = (unsigned char*)&isMoving - (unsigned char*)&position.x;
		isMoving = memcmp(&position.x, &other.position.x, l) != 0;
		memcpy(&position.x, &other.position.x, l);
		return *this;
	}

	void Camera::offsetOrientation(float dx, float dy)
	{
		pitch -= dy;
		yaw += dx;
		updateCamera();
	}

	void Camera::strafe(float dx, float dy)
	{
		glm::vec3 translation = -dx * right + dy * up;
		pivot = pivot + translation;
		updateCamera();
	}

	void Camera::changeRadius(float dr)
	{
		radius += dr;
		updateCamera();
	}

	void Camera::updateCamera()
	{
		glm::vec3 forward_temp;
		forward_temp.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		forward_temp.y = sin(glm::radians(pitch));
		forward_temp.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

		forward = glm::normalize(forward_temp);
		position = pivot + -forward * radius;

		right = glm::normalize(glm::cross(forward, worldUp));
		up = glm::normalize(glm::cross(right, forward));
	}
}