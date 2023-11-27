//----------------------------------------------------------------------------------------
/**
* \file    main.cpp
* \author  Jakub Neustadt
* \date    2019
* \brief   OpenGL 3D scene project, loosely inspired by Harry Potter universe.
*/
//----------------------------------------------------------------------------------------

#include <time.h>
#include <stdio.h>
#include <iostream>
#include <vector>

#include "pgr.h"
#include "render_stuff.h"
#include "Camera.h"
#include "Spline.h"

#define WIN_WIDTH  1280
#define WIN_HEIGHT 720
#define WIN_TITLE "Castle"

#define SCENE_WIDTH  1.0f
#define SCENE_HEIGHT 1.0f
#define SCENE_DEPTH  1.0f

#define BROOM_STICK_SIZE  1.0f
#define CAULDRON_SIZE     0.5f
#define CASTLE_SIZE		  20.0f
#define WAND_SIZE		  0.2f
#define WOODEN_TABLE_SIZE 0.6f
#define WOODEN_DOOR_SIZE  2.2f
#define GROUND_SIZE		  100.0f
#define TREE_SIZE		  2.0f
#define BANNER_SIZE		  1.0f
#define FLAME_SIZE		  1.0f

#define BROOM_STICK_SPEED 2.0f
#define WALK_SPEED 20.0f

#define REFRESH_TIME 33
#define CAMERA_VERTICAL_MAX 90.0f	// 90 degrees upwards

//-----------------------------------------------------------------------------------------------------------------------------------------------

// Shaders
extern SCommonShaderProgram shaderProgram;
extern SkyboxShaderProgram skyboxShaderProgram;
extern BannerShaderProgram bannerShaderProgram;
extern BannerShaderProgram animBannerShaderProgram;
extern FlameShaderProgram flameShaderProgram;

extern glm::vec3 curveData[];
extern size_t curveSize;

bool dirLight = true;

// global vars
struct GameState
{
	GLsizei windowWidth;
	GLsizei windowHeight;

	float elapsedTime;
	float lastUpdateTime;

	bool gameOver;
	bool fog;
	bool wandGrabbed;
	bool bannerOn;

	bool engorgio;		// make cauldron bigger
	bool engorgioFinal; // bool for assessing if game is over
	bool alohomora;		// open the door

	glm::vec3 cameraDirection;

} gameState; 

// arrow keys assignment
struct Keyboard
{
	bool leftArrow;
	bool rightArrow;
	bool upArrow;
	bool downArrow;

} keyboard;

// objects in the scene
BroomObject * broom;
Object * cauldron;
Object * castle;
Object * wand;
Object * table;
Object * door;
Object * openedDoor;
Object * ground;
Object * tree;

Camera * player;

Object * banner;
Object * animBanner;
FlameObject * flame;

//========================================================================

void drawWindowContents ( void )
{
	glm::mat4 orthoProjectionMatrix = glm::ortho(
		-SCENE_WIDTH, SCENE_WIDTH,
		-SCENE_HEIGHT, SCENE_HEIGHT,
		-10.0f * SCENE_DEPTH, 10.0f * SCENE_DEPTH
	);

	glm::mat4 orthoViewMatrix = glm::lookAt(
		glm::vec3(0.0f, 0.0f, 1.0f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f)
	);

	glm::mat4 viewMatrix = orthoViewMatrix;
	glm::mat4 projectionMatrix = orthoProjectionMatrix;

	glm::vec3 cameraPosition = player->cameraPos;
	glm::vec3 cameraCenter = player->cameraDir + cameraPosition;
	glm::vec3 cameraUpVector = glm::vec3(0.0f, 1.0f, 0.0f);

	if ( player->freeMovement )
	{
		glm::vec3 cameraViewDirection = player->cameraDir;

		glm::vec3 rotationAxis = glm::cross(cameraViewDirection, glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 cameraTransform = glm::rotate(glm::mat4(1.0f), glm::radians(-player->cameraElevationAngleY), rotationAxis);

		cameraUpVector = glm::vec3(cameraTransform * glm::vec4(cameraUpVector, 0.0f));
		cameraViewDirection = glm::vec3(cameraTransform * glm::vec4(cameraViewDirection, 0.0f));

		cameraCenter = cameraPosition + cameraViewDirection;
	}

	viewMatrix = glm::lookAt(
		cameraPosition,
		cameraCenter,
		cameraUpVector
	);

	//											FOVy				         aspect ratio								   near  far
	projectionMatrix = glm::perspective(glm::radians(70.0f), (float)gameState.windowWidth / (float)gameState.windowHeight, 0.1f, 100.0f);

	glUseProgram(shaderProgram.program);
	glUniform1f(shaderProgram.timeLocation, gameState.elapsedTime);
	glUniform3fv(shaderProgram.reflectorPositionLocation, 1, glm::value_ptr(player->cameraPos));
	glUniform3fv(shaderProgram.reflectorDirectionLocation, 1, glm::value_ptr(cameraCenter - player->cameraPos));
	glUniform1i(shaderProgram.reflectorLocation, player->spotlightOn);

	dirLight = false;
	glUniform1i(shaderProgram.dirLightLocation, dirLight);
	glUniform1i(shaderProgram.fogLocation, gameState.fog);
	//glUniform1iv(shaderProgram.fireLocation, 1, true);
	glUseProgram(0);

	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	CHECK_GL_ERROR();

	// turn on directional light 
	dirLight = true;
	glUseProgram(shaderProgram.program);
	glUniform1i(shaderProgram.dirLightLocation, dirLight);

	// draw interactable wand
	glStencilFunc(GL_ALWAYS, 1, 255);
	if ( !gameState.wandGrabbed )
		drawWand ( wand, viewMatrix, projectionMatrix );
	CHECK_GL_ERROR();

	// draw interactable cauldron
	glStencilFunc(GL_ALWAYS, 2, 255);
	if ( gameState.engorgio )
	{
		cauldron->size *= 5.0f;
		cauldron->position += 1.0f;
		gameState.engorgio = false;
	}
	drawCauldron ( cauldron, viewMatrix, projectionMatrix );
	CHECK_GL_ERROR();

	// draw interactable door
	glStencilFunc(GL_ALWAYS, 3, 255);
	if ( !gameState.alohomora )
	{
		drawDoor ( door, viewMatrix, projectionMatrix );
	}
	else
		drawOpenedDoor ( openedDoor, viewMatrix, projectionMatrix );
	CHECK_GL_ERROR();

	// draw interactable tree
	/*
	glStencilFunc ( GL_ALWAYS, 4, 255 );
	drawTree ( tree, viewMatrix, projectionMatrix );
	CHECK_GL_ERROR();
	*/

	// po vykresleni objektu s kterym interaguju disable stencil test
	glDisable(GL_STENCIL_TEST);

	drawCastle ( castle, viewMatrix, projectionMatrix );
	drawTable ( table, viewMatrix, projectionMatrix );
	drawBroom ( broom, viewMatrix, projectionMatrix );
	CHECK_GL_ERROR();

	glUseProgram(0);

	dirLight = false;
	glUseProgram(skyboxShaderProgram.program);
	glUniform1i(skyboxShaderProgram.fogOnLocation, gameState.fog);
	drawSkybox(viewMatrix, projectionMatrix);
	CHECK_GL_ERROR();

	glUseProgram(0);

	// turn on directional light 
	dirLight = true;
	glUseProgram(shaderProgram.program);
	glUniform1i(shaderProgram.dirLightLocation, dirLight);

	glUseProgram(0);

	drawGround ( ground, viewMatrix, projectionMatrix );
	drawFlame ( flame, viewMatrix, projectionMatrix );

	if ( gameState.bannerOn && banner != NULL )
		drawBanner ( banner, orthoViewMatrix, orthoProjectionMatrix );

	if ( gameState.gameOver )
	{
		drawAnimatedBanner ( animBanner, orthoViewMatrix, orthoProjectionMatrix );
		animBanner->currentTime = gameState.elapsedTime;
	}
}

// create objects and assign their initial attributes
void setInitialObjectProperties ( void )
{
	// broom
	if ( broom == NULL )
		broom = new BroomObject;
	broom->position = glm::vec3 ( -7.0f, 1.5f, -7.0f );
	broom->initPosition = broom->position;
	broom->direction = glm::vec3 ( 3.0f, -1.0f, 1.5f );
	broom->size = BROOM_STICK_SIZE;
	broom->speed = BROOM_STICK_SPEED;

	// cauldron
	if ( cauldron == NULL )
		cauldron = new Object;
	cauldron->position = glm::vec3 ( 11.5f, 0.0f, -11.3f );
	cauldron->direction = glm::vec3 ( 1.0f, 0.0f, 0.0f );
	cauldron->size = CAULDRON_SIZE;

	// castle
	if ( castle == NULL )
		castle = new Object;
	castle->position = glm::vec3 ( -2.0f, 16.6f, -23.0f );
	castle->direction = glm::vec3 ( 0.0f, 0.0f, 0.0f );
	castle->size = CASTLE_SIZE;

	// wand
	if ( wand == NULL )
		wand = new Object;
	wand->position = glm::vec3 ( 8.3f, 0.1f, -11.0f );
	wand->direction = glm::vec3 ( 0.0f, 0.0f, 0.0f );
	wand->size = WAND_SIZE;

	// table
	if ( table == NULL )
		table = new Object;
	table->position = glm::vec3 ( 8.3f, -0.1f, -11.0f );
	table->direction = glm::vec3 ( 0.0f, 0.0f, 0.0f );
	table->size = WOODEN_TABLE_SIZE;

	// door
	if ( door == NULL )
		door = new Object;
	door->position = glm::vec3 ( 6.5f, 0.7f, -23.0f );
	door->direction = glm::vec3 ( 0.0f, 0.0f, 0.0f );
	door->size = WOODEN_DOOR_SIZE;

	// opened door
	if ( openedDoor == NULL )
		openedDoor = new Object;
	openedDoor->position = glm::vec3 ( 5.35f, 0.7f, -21.2f );
	openedDoor->direction = glm::vec3 ( 0.0f, 0.0f, 0.0f );
	openedDoor->size = WOODEN_DOOR_SIZE;

	// ground
	if ( ground == NULL )
		ground = new Object;
	ground->position = glm::vec3 ( -70.0f, -0.4f, -70.0f );
	ground->size = GROUND_SIZE;

	// tree
	if ( tree == NULL )
		tree = new Object;
	tree->position = glm::vec3 ( 1.5f, -0.4f, -15.0f );
	tree->direction = glm::vec3 ( 0.0f, 0.0f, 0.0f );
	tree->size = TREE_SIZE;

	// camera 
	if ( player == NULL )
		player = new Camera;
	player->spotlightOn = false;
	player->cameraElevationAngleX = 170.0f;
	player->cameraElevationAngleY = 0.0f;
	player->cameraPos = glm::vec3 ( -1.0f, 0.0f, 2.0f );
	player->cameraDir = glm::vec3 ( -0.9f, -0.3f, 0.5f );

	// banner
	if ( banner == NULL )
		banner = new Object;
	banner->size = BANNER_SIZE;
	banner->position = glm::vec3 ( 0.0f, 0.0f, 0.0f );
	banner->direction = glm::vec3 ( 0.0f, 1.0f, 0.0f );

	banner->startTime = gameState.elapsedTime;
	banner->currentTime = banner->startTime;

	// animated Banner
	if ( animBanner == NULL )
		animBanner = new Object;
	animBanner->size = BANNER_SIZE;
	animBanner->position = glm::vec3 ( 0.0f, 0.0f, 0.0f );
	animBanner->direction = glm::vec3 ( 0.0f, 1.0f, 0.0f );
	animBanner->speed = 0.0f;
	animBanner->startTime = gameState.elapsedTime;
	animBanner->currentTime = banner->startTime;

	// flame
	if ( flame == NULL )
		flame = new FlameObject;
	flame->size = FLAME_SIZE;
	flame->position = glm::vec3 ( -2.0f, 3.0f, -16.5f );
	flame->direction = glm::vec3 ( 0.0f, 0.0f, 1.0f );

	flame->startTime = gameState.elapsedTime;
	flame->currentTime = banner->startTime;

	flame->frameDuration = 0.06f;
	flame->textureFrames = 16;		// 4 * 4 frames

									// initialize times in objects
	player->cameraTime = gameState.elapsedTime;
	broom->startTime = gameState.elapsedTime;
	broom->currentTime = gameState.elapsedTime;
}

// set initial gameState values, call setInitialObjectProperties
void restartGame()
{
	// set initial gameState values
	gameState.elapsedTime = 0.001f * (float)glutGet(GLUT_ELAPSED_TIME);
	gameState.lastUpdateTime = gameState.elapsedTime;
	gameState.gameOver = false;
	gameState.fog = false;
	gameState.wandGrabbed = false;
	gameState.bannerOn = false;
	gameState.engorgio = false;
	gameState.engorgioFinal = false;
	gameState.alohomora = false;

	glutWarpPointer(
		gameState.windowWidth,
		gameState.windowHeight
	);

	setInitialObjectProperties ( );
}

// true -> there is collision, false -> there is not
bool checkCollision ( glm::vec3 pos )
{
	// borders of scene, X coord
	if ( pos.x > 22.0f || pos.x < -22.0f )
		return true;

	// borders of scene, Z coord
	if ( pos.z > 10.0f || pos.z < -40.0f )
		return true;

	// borders of objects
	if ( glm::distance( table->position, pos ) < 0.5f )
		return true;

	if ( gameState.engorgio )
	{
		glm::vec3 tmp ( cauldron->position.x, pos.y, cauldron->position.z );

		if ( glm::distance( tmp, pos ) < 1.0f )
			return true;
	}

	if ( glm::distance( cauldron->position, pos ) < 0.6f )
		return true;

	if ( glm::distance( tree->position, pos ) < 0.6f )
		return true;

	if ( (!gameState.alohomora) && (glm::distance( door->position, pos ) < 1.0f) )
		return true;

	return false;
}

// call drawWindowContents and glutSwapBuffers
void buildScene()
{
	GLbitfield mask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
	mask |= GL_STENCIL_BUFFER_BIT;
	glClear( mask );

	drawWindowContents();

	glutSwapBuffers();
	CHECK_GL_ERROR();
}

void windowResize(int width, int height) 
{
	gameState.windowWidth = width;
	gameState.windowHeight = height;

	glViewport(0, 0, (GLsizei)width, (GLsizei)height);
}

void mouseCallback(int button, int state, int x, int y)
{
	if ( button == GLUT_LEFT_BUTTON && state == GLUT_DOWN )
	{
		// default value 0, which means non-interactable object / background
		GLubyte objectID = 0;

		glReadPixels ( x, gameState.windowHeight - y - 1, 1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, &objectID );

		// picking up wand
		if ( objectID == 1 && glm::distance( player->cameraPos, wand->position) < 1.5f )
		{
			gameState.wandGrabbed = true;
			gameState.bannerOn = true;
		}

		// "cast spell" on cauldron
		if ( objectID == 2 && glm::distance( player->cameraPos, cauldron->position) < 5.0f 
			&& gameState.wandGrabbed )
		{
			gameState.engorgio = true;
			gameState.engorgioFinal = true;
		}

		// "cast spell" on door
		if ( objectID == 3 && glm::distance( player->cameraPos, door->position) < 5.0f 
			&& gameState.wandGrabbed )
		{
			gameState.alohomora = true;
		}
	}
}

void keyboardCallback(unsigned char key, int x, int y)
{

	switch ( key )
	{
	case 27:
		glutLeaveMainLoop();
		break;

	case 'r':
		restartGame();
		break;

	case 'c':
		std::cout << player->cameraPos.x << ", " << player->cameraPos.y << ", " << player->cameraPos.z << std::endl;
		break;

	case 'g':
		gameState.fog = !gameState.fog;
		break;

	case 'l':
		player->spotlightOn = !player->spotlightOn;
		break;

	default:
		break;
	}

}

void keyboardUpCallback(unsigned char key, int x, int y)
{

}

void keyboardSpecialCallback(int key, int x, int y)
{
	if ( gameState.gameOver )
		return;

	switch ( key )
	{
	case GLUT_KEY_LEFT:
		keyboard.leftArrow = true;
		break;

	case GLUT_KEY_RIGHT:
		keyboard.rightArrow = true;
		break;

	case GLUT_KEY_UP:
		keyboard.upArrow = true;
		break;

	case GLUT_KEY_DOWN:
		keyboard.downArrow = true;
		break;

	case GLUT_KEY_F1:
		player->staticCameraFirst();
		break;

	case GLUT_KEY_F2:
		player->staticCameraSecond();
		break;

	case GLUT_KEY_F3:
		player->freeCamera();
		break;

	default:
		break;
	}
}

void keyboardSpecialUpCallback(int key, int x, int y)
{
	if ( gameState.gameOver )
		return;

	switch ( key )
	{
	case GLUT_KEY_LEFT:
		keyboard.leftArrow = false;
		break;

	case GLUT_KEY_RIGHT:
		keyboard.rightArrow = false;
		break;

	case GLUT_KEY_UP:
		keyboard.upArrow = false;
		break;

	case GLUT_KEY_DOWN:
		keyboard.downArrow = false;
		break;

	default:
		break;
	}
}

void passiveMotionFunc(int newPosX, int newPosY) 
{
	if ( gameState.gameOver )
		return;

	if ( player->freeMovement )
	{
		int mouseDeltaX = newPosX - gameState.windowWidth / 2;
		int mouseDeltaY = newPosY - gameState.windowHeight / 2;

		// move camera position by mouseDeltaX and mouseDeltaY
		if ( newPosY != gameState.windowHeight / 2 )
		{
			float cameraVerticalAngleDelta = 0.5f * mouseDeltaY;

			if ( fabs( player->cameraElevationAngleY + cameraVerticalAngleDelta ) < CAMERA_VERTICAL_MAX )
				player->cameraElevationAngleY += cameraVerticalAngleDelta;
		}

		if ( newPosX != gameState.windowWidth / 2 )
		{
			float cameraHorizontalAngleDelta = 0.5f * mouseDeltaX;

			player->cameraElevationAngleX += cameraHorizontalAngleDelta;
			if ( player->cameraElevationAngleX > 360.0f )
				player->cameraElevationAngleX -= 360.0f;
			if ( player->cameraElevationAngleX < 0.0f )
				player->cameraElevationAngleX += 360.0f;

			player->cameraDir = glm::vec3( sin( glm::radians( -player->cameraElevationAngleX ) ), 0, cos( glm::radians( player->cameraElevationAngleX ) ) );
		}
	}

	// premisti cursor pointer do stredu okna
	glutWarpPointer( gameState.windowWidth / 2, gameState.windowHeight / 2 );

	glutPostRedisplay ( );
}

// timer function called every frame, updating objects' properties, movement
void timerFunc(int id) 
{
	gameState.elapsedTime = 0.001f * (float)glutGet(GLUT_ELAPSED_TIME);

	float timeDelta = gameState.elapsedTime - gameState.lastUpdateTime;
	gameState.lastUpdateTime = gameState.elapsedTime;

	glm::vec3 updatedPosition;

	if ( !player->freeMovement )
		//nehybat ani s mysi

		// turn camera left by specified angle
		if (keyboard.leftArrow)
		{
			updatedPosition = player->cameraPos - timeDelta * WALK_SPEED * glm::cross(player->cameraDir, glm::vec3(0.0f, 1.0f, 0.0f));
			if ( !checkCollision( updatedPosition ) )
				player->cameraPos = updatedPosition;
		}

	// turn camera right by specified angle
	if (keyboard.rightArrow)
	{
		updatedPosition = player->cameraPos + timeDelta * WALK_SPEED * glm::cross(player->cameraDir, glm::vec3(0.0f, 1.0f, 0.0f));
		if ( !checkCollision( updatedPosition ) )
			player->cameraPos = updatedPosition;
	}

	// increase speed at which camera is moving
	if (keyboard.upArrow)
	{
		updatedPosition = player->cameraPos + timeDelta * WALK_SPEED * player->cameraDir;

		if ( !checkCollision( updatedPosition ) )
			player->cameraPos = updatedPosition;
	}

	// decrease speed at which camera is moving
	if (keyboard.downArrow)
	{
		updatedPosition = player->cameraPos - timeDelta * WALK_SPEED * player->cameraDir;

		if ( !checkCollision( updatedPosition ) )
			player->cameraPos = updatedPosition;
	}		


	// update position of broom on its curve
	broom->currentTime = gameState.elapsedTime;

	float curveParamT = broom->speed * ( broom->currentTime - broom->startTime );

	broom->position = broom->initPosition + evaluateClosedCurve ( curveData, curveSize, curveParamT );
	broom->direction = glm::normalize ( evaluateClosedCurve_1stDerivative ( curveData, curveSize, curveParamT ) );

	flame->currentTime = gameState.elapsedTime;

	// if everything's been done
	if ( gameState.wandGrabbed && gameState.engorgioFinal && gameState.alohomora )
	{
		std::cout << "all done" << std::endl;
		animBanner->startTime = gameState.elapsedTime;
		animBanner->currentTime = gameState.elapsedTime;

		gameState.engorgioFinal = false;
		gameState.gameOver = true;
	}

	glutPostRedisplay();

	// nastav kdy se znovu zavola timerCallback
	glutTimerFunc( REFRESH_TIME, timerFunc, 0 );
}

// enable depth test, call initializeShaderPrograms(), initializeModels()
// and restartGame()
void init()
{
	glClearColor(0.1f, 0.1f, 4.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);

	// simple glut cursor in window
	glutSetCursor(GLUT_CURSOR_CROSSHAIR);

	// in-game menu
	// glutCreateMenu();

	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glBlendEquation(GL_FUNC_ADD);

	initializeShaderPrograms();
	initializeModels();

	restartGame();

	//glViewport(0, 0, gameState.windowWidth, gameState.windowHeight);
}

// delete all object pointers
void cleanupObjects()
{
	broom = NULL;
	cauldron = NULL;	
	castle = NULL;	
	wand = NULL;	
	table = NULL;	
	door = NULL;
	openedDoor = NULL;
	ground = NULL;

	player = NULL;

	banner = NULL;
	flame = NULL;
}

// cleanupObjects(), cleanupModels(), cleanupShaderPrograms()
void destroy() 
{
	//delete all allocated resources
	cleanupObjects();
	cleanupModels();

	// delete shaders
	cleanupShaderPrograms();
}

int main(int argc, char **argv)
{
	glutInit(&argc, argv);

	glutInitContextVersion(pgr::OGL_VER_MAJOR, pgr::OGL_VER_MINOR);
	glutInitContextFlags(GLUT_FORWARD_COMPATIBLE);

	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_STENCIL);		// GLUT_STENCIL PØIDAT??
	glutInitWindowSize(WIN_WIDTH, WIN_HEIGHT);
	glutCreateWindow(WIN_TITLE);

	/*
	createMenu();
	menuNumber = -1;
	*/

	glutDisplayFunc(buildScene);
	glutReshapeFunc(windowResize);

	glutKeyboardFunc(keyboardCallback);
	glutKeyboardUpFunc(keyboardUpCallback);

	glutSpecialFunc(keyboardSpecialCallback);
	glutSpecialUpFunc(keyboardSpecialUpCallback);

	glutMouseFunc(mouseCallback);
	glutPassiveMotionFunc(passiveMotionFunc);   // callback na pohyb mysi bez stisknuteho tlacitka
												//glutMotionFunc();							// click and drag funkce, za bonusovy body
	glutTimerFunc(REFRESH_TIME, timerFunc, 0);
	glutCloseFunc(destroy);						// kdyz user zavre sam okno -> musim uklidit

	if (!pgr::initialize(pgr::OGL_VER_MAJOR, pgr::OGL_VER_MINOR))
		pgr::dieWithError("pgr init failed, required OpenGL not supported?");

	init();

	glutMainLoop();

	return 0;
}
