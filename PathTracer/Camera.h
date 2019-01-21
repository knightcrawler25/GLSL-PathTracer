#pragma once
#include <glm/glm.hpp>

namespace GLSLPathTracer
{
    class Camera
    {
    public:
        Camera(glm::vec3 pos, glm::vec3 lookAt, float fov);
        Camera(const Camera& other);
        Camera& operator = (const Camera& other);

        void offsetOrientation(float x, float y);
        void offsetPosition(glm::vec3 val);
        void updateCamera();
        glm::vec3 position;
        glm::vec3 up;
        glm::vec3 right;
        glm::vec3 forward;
        glm::vec3 worldUp;
        float pitch, yaw, fov, focalDist, aperture;
        bool isMoving;
    };
}