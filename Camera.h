#pragma once
#ifndef CAMERA_CLASS_H
#define CAMERA_CLASS_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>

class Camera
{
public:
    glm::vec3 Position;
    glm::vec3 Orientation = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);

    bool firstClick = true;

    int width;
    int height;

    float speed = 2.5f;
    float sensitivity = 100.0f;

    float FOVdeg = 45.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;

    double lastX = 0.0, lastY = 0.0;
    float yaw = -90.0f;
    float pitch = 0.0f;


    Camera(int width, int height, glm::vec3 position);

    glm::mat4 getViewMatrix();
    glm::mat4 getProjectionMatrix();

    void Inputs(GLFWwindow* window, float deltaTime);

    void MouseCallback(GLFWwindow* window, double xpos, double ypos);
};
#endif