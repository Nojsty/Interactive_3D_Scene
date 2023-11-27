#include <string>

#include "pgr.h"

typedef struct MeshGeometry 
{
	GLuint        vertexBufferObject;   // identifier for the vertex buffer object
	GLuint        elementBufferObject;  // identifier for the element buffer object
	GLuint        vertexArrayObject;    // identifier for the vertex array object
	unsigned int  numTriangles;         // number of triangles in the mesh

										// material
	glm::vec3     ambient;
	glm::vec3     diffuse;
	glm::vec3     specular;
	float         shininess;

	GLuint        texture;

} MeshGeometry;

// parameters of individual objects in the scene (e.g. position, size, speed, etc.)
typedef struct Object 
{
	glm::vec3 position;
	glm::vec3 direction;
	float     speed;
	float     size;

	float startTime;
	float currentTime;

} Object;

struct BroomObject : public Object
{
	glm::vec3 initPosition;
	float speed;
};

struct FlameObject : public Object
{
	int    textureFrames;
	float  frameDuration;
};

typedef struct _commonShaderProgram 
{
	// identifier for the program
	GLuint program;          // = 0;
							 // vertex attributes locations
	GLint posLocation;       // = -1;
	GLint colorLocation;     // = -1;
	GLint normalLocation;    // = -1;
	GLint texCoordLocation;  // = -1;
							 // uniforms locations
	GLint PVMmatrixLocation;    // = -1;
	GLint VmatrixLocation;      // = -1;  view/camera matrix
	GLint MmatrixLocation;      // = -1;  modeling matrix
	GLint normalMatrixLocation; // = -1;  inverse transposed Mmatrix

	GLint timeLocation;         // = -1; elapsed time in seconds

								// material 
	GLint diffuseLocation;    // = -1;
	GLint ambientLocation;    // = -1;
	GLint specularLocation;   // = -1;
	GLint shininessLocation;  // = -1;
							  // texture
	GLint useTextureLocation; // = -1; 
	GLint texSamplerLocation; // = -1;
							  // reflector related uniforms
	GLint reflectorPositionLocation;  // = -1; 
	GLint reflectorDirectionLocation; // = -1;
	GLint reflectorLocation;

	GLint cauldronLightLocation;
	GLint fogLocation;
	GLint dirLightLocation;

} SCommonShaderProgram;

typedef struct skyboxFarPlaneShaderProgram 
{
	// identifier for the program
	GLuint program;                 // = 0;
									// vertex attributes locations
	GLint screenCoordLocation;      // = -1;
									// uniforms locations
	GLint inversePVmatrixLocation; // = -1;
	GLint skyboxSamplerLocation;    // = -1;
	GLint fogOnLocation;

} SkyboxShaderProgram;

typedef struct bannerShaderProgram 
{
	// identifier for the program
	GLuint program;           // = 0;
							  // vertex attributes locations
	GLint posLocation;        // = -1;
	GLint texCoordLocation;   // = -1;
							  // uniforms locations
	GLint PVMmatrixLocation;  // = -1;
	GLint timeLocation;       // = -1;
	GLint texSamplerLocation; // = -1;
} BannerShaderProgram;

struct FlameShaderProgram : public BannerShaderProgram
{
	GLint VmatrixLocation;       // = -1;
	GLint frameDurationLocation; // = -1;
};

// ground data
const int groundTrianglesCount = 2;
const float groundVertices[] = {
	//x,y,z, u,v,   nx,ny,nz
	0,0,0, 0,0,   0,1,0,
	0,0,1, 0,30,  0,1,0,
	1,0,0, 30,0,  0,1,0,
	1,0,1, 30,30, 0,1,0,
};

const unsigned int groundIndices[] = {
	0,1,2,
	2,1,3,
};

// banner data
const int bannerNumQuadVertices = 4;
const float bannerVertexData[20] = {

	// x      y      z     u     v
	-1.0f,  1.00f, 0.0f, 0.0f, 2.0f,
	-1.0f, -1.00f, 0.0f, 0.0f, 0.0f,
	1.0f,  1.00f, 0.0f, 1.0f, 2.0f,
	1.0f, -1.00f, 0.0f, 1.0f, 0.0f
};

// flame data
const int flameNumQuadVertices = 4;
const float flameVertexData[20] = {
	-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
	1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
	-1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
	1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
};


glm::vec3 checkBounds(const glm::vec3 & position, float objectSize = 1.0f);

bool loadSingleMesh(const std::string &fileName, SCommonShaderProgram& shader, MeshGeometry** geometry);
void setTransformUniforms(const glm::mat4 &modelMatrix, const glm::mat4 &viewMatrix, const glm::mat4 &projectionMatrix);
void setMaterialUniforms(const glm::vec3 &ambient, const glm::vec3 &diffuse, const glm::vec3 &specular, float shininess, GLuint texture);

void drawCastle( Object * castle, const glm::mat4 & viewMatrix, const glm::mat4 & projectionMatrix );
void drawSkybox ( const glm::mat4 & viewMatrix, const glm::mat4 & projectionMatrix );
void drawBroom ( BroomObject * broom, const glm::mat4 & viewMatrix, const glm::mat4 & projectionMatrix );
void drawCauldron ( Object * cauldron, const glm::mat4 & viewMatrix, const glm::mat4 & projectionMatrix );
void drawWand ( Object * wand, const glm::mat4 & viewMatrix, const glm::mat4 & projectionMatrix ); 
void drawTable ( Object * table, const glm::mat4 & viewMatrix, const glm::mat4 & projectionMatrix );
void drawDoor ( Object * door, const glm::mat4 & viewMatrix, const glm::mat4 & projectionMatrix );
void drawOpenedDoor ( Object * door, const glm::mat4 & viewMatrix, const glm::mat4 & projectionMatrix );
void drawGround ( Object * ground, const glm::mat4 & viewMatrix, const glm::mat4 & projectionMatrix );
void drawTree ( Object * tree, const glm::mat4 & viewMatrix, const glm::mat4 & projectionMatrix );
void drawBanner ( Object * ground, const glm::mat4 & viewMatrix, const glm::mat4 & projectionMatrix );
void drawAnimatedBanner ( Object * ground, const glm::mat4 & viewMatrix, const glm::mat4 & projectionMatrix );
void drawFlame ( FlameObject * flame, const glm::mat4 & viewMatrix, const glm::mat4 & projectionMatrix );

void initializeBroom ( void );
void initializeCauldron ( void );
void initializeCastle ( void );
void initializeWand ( void );
void initializeTable ( void );
void initializeDoor ( void );
void initializeOpenedDoor ( void );
void initializeGround ( void );
void initializeTree ( void );
void initializeBanner ( void );
void initializeAnimatedBanner ( void );
void initializeSkybox(GLuint shader, MeshGeometry ** geometry);

void initializeShaderPrograms();
void cleanupShaderPrograms();

void cleanupGeometry(MeshGeometry * geometry);

void initializeModels();
void cleanupModels();