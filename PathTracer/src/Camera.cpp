#include <Camera.h>
#include <iostream>

namespace GLSLPathTracer
{
    Camera::Camera(glm::vec3 pos, glm::vec3 lookAt, float fov)
    {
        position = pos;
        worldUp = glm::vec3(0, 1, 0);
        glm::vec3 dir = glm::normalize(lookAt - position);
        pitch = glm::degrees(asin(dir.y));
        yaw = glm::degrees(atan2(dir.z, dir.x));

        this->fov = glm::radians(fov);
        focalDist = 0.1f;
        aperture = 0.0;
        updateCamera();
    }

    void Camera::offsetOrientation(float x, float y)
    {
        pitch -= y;
        yaw += x;
        updateCamera();
    }

    void Camera::offsetPosition(glm::vec3 newPos)
    {
        position += newPos;
        updateCamera();
    }

    void Camera::updateCamera()
    {
        glm::vec3 forward_temp;
        forward_temp.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        forward_temp.y = sin(glm::radians(pitch));
        forward_temp.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        forward = glm::normalize(forward_temp);

        right = glm::normalize(glm::cross(forward, worldUp));
        up = glm::normalize(glm::cross(right, forward));
    }
}