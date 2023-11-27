#include "Camera.h"

Camera::Camera()
{}

void Camera::staticCameraFirst ( )
{
	freeMovement = false;

	cameraPos = glm::vec3 ( 5.0f, 5.0f, -9.0f );
	cameraDir = glm::vec3 ( 1.0f, -1.0f, -1.0f );
}

void Camera::staticCameraSecond ( )
{
	freeMovement = false;

	cameraPos = glm::vec3 ( -3.0f, 5.0f, -12.0f );
	cameraDir = glm::vec3 ( -1.0f, 0.0f, -1.0f );
}

void Camera::freeCamera ( )
{
	freeMovement = true;

	cameraPos = glm::vec3 ( -1.0f, 0.0f, 2.0f );
	cameraDir = glm::vec3 ( 1.0f, 0.0f, 1.0f );
}
