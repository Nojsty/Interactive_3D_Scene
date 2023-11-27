/**
* \file       Camera.h
* \author     Jakub Neustadt
* \date       2019
* \brief      Header file defining Camera class.
*/

#include "pgr.h"

class Camera
{
public:

	glm::vec3 cameraPos;
	glm::vec3 cameraDir;
	glm::vec3 cameraView;
	glm::vec3 cameraUp;

	float cameraTime;

	float cameraElevationAngleX;
	float cameraElevationAngleY;

	bool spotlightOn;

	bool freeMovement;

	Camera();
	void staticCameraFirst ( );
	void staticCameraSecond ( );
	void freeCamera ( );

};
