#include "Camera.h"
#include <iostream>

Camera::Camera(int width, int height, glm::vec3 position)
{
    Camera::width = width;
    Camera::height = height;
    Position = position;
    lastX = (double)width / 2.0;
    lastY = (double)height / 2.0;
}

glm::mat4 Camera::getViewMatrix()
{
    return glm::lookAt(Position, Position + Orientation, Up);
}

glm::mat4 Camera::getProjectionMatrix()
{
    return glm::perspective(glm::radians(FOVdeg), (float)width / (float)height, 0.1f, 1000.0f);
}

void Camera::Inputs(GLFWwindow* window, float deltaTime)
{
    float currentSpeed = speed * deltaTime;
    float speedMultiplier = 1.0f;

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
    {
        speedMultiplier = 4.0f;
    }

    currentSpeed *= speedMultiplier;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) Position += currentSpeed * Orientation;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) Position -= currentSpeed * Orientation;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) Position -= glm::normalize(glm::cross(Orientation, Up)) * currentSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) Position += glm::normalize(glm::cross(Orientation, Up)) * currentSpeed;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) Position += currentSpeed * Up;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) Position -= currentSpeed * Up;
}

void Camera::MouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstClick) {
        lastX = xpos;
        lastY = ypos;
        firstClick = false;
    }

    float xoffset = static_cast<float>(xpos - lastX);
    float yoffset = static_cast<float>(lastY - ypos);
    lastX = xpos;
    lastY = ypos;

    float currentSensitivity = sensitivity / 1000.0f;
    xoffset *= currentSensitivity;
    yoffset *= currentSensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    Orientation = glm::normalize(front);
}