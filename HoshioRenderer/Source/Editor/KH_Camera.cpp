#include "KH_Camera.h"

KH_Camera::KH_Camera(uint32_t width, uint32_t height, glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    :MovementSpeed(2.5f),
    MouseSensitivity(0.1f),
    Zoom(45.0f),
    Front(0.0f, 0.0f, -1.0f)
{
    Width = width;
    Height = height;
    Position = position;
    WorldUp = up;
    Yaw = yaw;
    Pitch = pitch;

    UpdateCameraVectors();
}

glm::mat4 KH_Camera::GetViewMatrix() const
{
    return glm::lookAt(Position, Position + Front, Up);
}

glm::mat4 KH_Camera::GetProjMatrix() const
{
    float AspectRatio = static_cast<float>(Width) / static_cast<float>(Height);
    return glm::perspective(glm::radians(Zoom), AspectRatio, NearPlane, FarPlane);
}

void KH_Camera::ProcessKeyboard(CameraMovement direction, float deltaTime)
{
    float velocity = MovementSpeed * deltaTime;

    if (direction == CameraMovement::Forward)
        Position += Front * velocity;
    if (direction == CameraMovement::Backward)
        Position -= Front * velocity;
    if (direction == CameraMovement::Left)
        Position -= Right * velocity;
    if (direction == CameraMovement::Right)
        Position += Right * velocity;
    if (direction == CameraMovement::Up)
        Position += Up * velocity;
    if (direction == CameraMovement::Down)
        Position -= Up * velocity;
}

void KH_Camera::ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch)
{
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;

    Yaw += xoffset;
    Pitch += yoffset;

    if (constrainPitch)
    {
        if (Pitch > 89.0f)
            Pitch = 89.0f;
        if (Pitch < -89.0f)
            Pitch = -89.0f;
    }

    UpdateCameraVectors();
}

void KH_Camera::ProcessMouseScroll(float yoffset)
{
    MovementSpeed += yoffset * SpeedSensitivity;

    if (MovementSpeed < 0.1f)
        MovementSpeed = 0.1f;

    if (MovementSpeed > 20.0f)
        MovementSpeed = 20.0f;
}

void KH_Camera::UpdateCameraVectors()
{
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    Front = glm::normalize(front);

    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up = glm::normalize(glm::cross(Right, Front));
}

