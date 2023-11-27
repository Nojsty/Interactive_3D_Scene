#include <iostream>
#include "render_stuff.h"
#include "Spline.h"
#include "lowPolyTree.h"

// Source model files
const std::string BROOM_STICK_FILE = "vendor/models/broom-stick/source/BroomStick/broom_stick.obj";
const std::string CAULDRON_FILE = "vendor/models/cauldron/Cauldrin/cauldron.obj";
const std::string CASTLE_FILE = "vendor/models/great-hall/source/great_hall.obj";
const std::string WAND_FILE = "vendor/models/wand/wandTex.obj";
const std::string WOODEN_TABLE_FILE = "vendor/models/Wooden table/textured_table.obj";
const std::string WOODEN_DOOR_FILE = "vendor/models/wooden-door/source/Medieval_Door/door.obj";
const std::string WOODEN_DOOR_OPENED_FILE = "vendor/models/wooden-door/source/Medieval_Door/door_opened.obj";

const std::string GROUND_TEXTURE_FILE = "vendor/models/grass_texture.jpg";
const std::string TREE_TEXTURE_FILE = "vendor/models/tree/textures/tree04_dffs.png";

const std::string BANNER_TEXTURE_FILE = "vendor/models/banner.png";
const std::string ANIM_BANNER_TEXTURE_FILE = "vendor/models/bannerEnd.png";
const std::string FLAME_TEXTURE_FILE = "vendor/models/flame.png";

// Meshes
MeshGeometry * castleGeometry = NULL;
MeshGeometry * skyboxGeometry = NULL;
MeshGeometry * broomGeometry = NULL;
MeshGeometry * cauldronGeometry = NULL;
MeshGeometry * wandGeometry = NULL;
MeshGeometry * tableGeometry = NULL;
MeshGeometry * doorGeometry = NULL;
MeshGeometry * openedDoorGeometry = NULL;
MeshGeometry * groundGeometry = NULL;
MeshGeometry * treeGeometry = NULL;
MeshGeometry * bannerGeometry = NULL;
MeshGeometry * animBannerGeometry = NULL;
MeshGeometry * flameGeometry = NULL;

// Shaders
SCommonShaderProgram shaderProgram;
SkyboxShaderProgram skyboxShaderProgram;
BannerShaderProgram bannerShaderProgram;
BannerShaderProgram animBannerShaderProgram;
FlameShaderProgram flameShaderProgram;

//============================================================================================================================
/** Load mesh using assimp library
* \param filename [in] file to open/load
* \param shader [in] vao will connect loaded data to shader
* \param vbo [out] vertex and normal data |VVVVV...|NNNNN...| (no interleaving)
* \param eao [out] triangle indices
* \param vao [out] vao connects data to shader input
* \param numTriangles [out] how many triangles have been loaded and stored into index array eao
*/
bool loadSingleMesh(const std::string &fileName, SCommonShaderProgram& shader, MeshGeometry** geometry) {
	Assimp::Importer importer;

	// Unitize object in size (scale the model to fit into (-1..1)^3)
	importer.SetPropertyInteger(AI_CONFIG_PP_PTV_NORMALIZE, 1);

	// Load asset from the file - you can play with various processing steps
	const aiScene * scn = importer.ReadFile(fileName.c_str(), 0
		| aiProcess_Triangulate             // Triangulate polygons (if any).
		| aiProcess_PreTransformVertices    // Transforms scene hierarchy into one root with geometry-leafs only. For more see Doc.
		| aiProcess_GenSmoothNormals        // Calculate normals per vertex.
		| aiProcess_JoinIdenticalVertices);

	// abort if the loader fails
	if (scn == NULL) {
		std::cerr << "assimp error: " << importer.GetErrorString() << std::endl;
		*geometry = NULL;
		return false;
	}

	// some formats store whole scene (multiple meshes and materials, lights, cameras, ...) in one file, we cannot handle that in our simplified example
	if (scn->mNumMeshes != 1) {
		std::cerr << "this simplified loader can only process files with only one mesh" << std::endl;
		*geometry = NULL;
		return false;
	}

	// in this phase we know we have one mesh in our loaded scene, we can directly copy its data to OpenGL ...
	const aiMesh * mesh = scn->mMeshes[0];

	*geometry = new MeshGeometry;

	// vertex buffer object, store all vertex positions and normals
	glGenBuffers(1, &((*geometry)->vertexBufferObject));
	glBindBuffer(GL_ARRAY_BUFFER, (*geometry)->vertexBufferObject);
	glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(float)*mesh->mNumVertices, 0, GL_STATIC_DRAW); // allocate memory for vertices, normals, and texture coordinates
																							// first store all vertices
	glBufferSubData(GL_ARRAY_BUFFER, 0, 3 * sizeof(float)*mesh->mNumVertices, mesh->mVertices);
	// then store all normals
	glBufferSubData(GL_ARRAY_BUFFER, 3 * sizeof(float)*mesh->mNumVertices, 3 * sizeof(float)*mesh->mNumVertices, mesh->mNormals);

	// just texture 0 for now
	float *textureCoords = new float[2 * mesh->mNumVertices];  // 2 floats per vertex
	float *currentTextureCoord = textureCoords;

	// copy texture coordinates
	aiVector3D vect;

	if (mesh->HasTextureCoords(0)) {
		// we use 2D textures with 2 coordinates and ignore the third coordinate
		for (unsigned int idx = 0; idx<mesh->mNumVertices; idx++) {
			vect = (mesh->mTextureCoords[0])[idx];
			*currentTextureCoord++ = vect.x;
			*currentTextureCoord++ = vect.y;
		}
	}

	// finally store all texture coordinates
	glBufferSubData(GL_ARRAY_BUFFER, 6 * sizeof(float)*mesh->mNumVertices, 2 * sizeof(float)*mesh->mNumVertices, textureCoords);

	// copy all mesh faces into one big array (assimp supports faces with ordinary number of vertices, we use only 3 -> triangles)
	unsigned int *indices = new unsigned int[mesh->mNumFaces * 3];
	for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
		indices[f * 3 + 0] = mesh->mFaces[f].mIndices[0];
		indices[f * 3 + 1] = mesh->mFaces[f].mIndices[1];
		indices[f * 3 + 2] = mesh->mFaces[f].mIndices[2];
	}

	// copy our temporary index array to OpenGL and free the array
	glGenBuffers(1, &((*geometry)->elementBufferObject));
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, (*geometry)->elementBufferObject);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3 * sizeof(unsigned) * mesh->mNumFaces, indices, GL_STATIC_DRAW);

	delete[] indices;
	// copy the material info to MeshGeometry structure
	const aiMaterial *mat = scn->mMaterials[mesh->mMaterialIndex];
	aiColor4D color;
	aiString name;
	aiReturn retValue = AI_SUCCESS;

	// Get returns: aiReturn_SUCCESS 0 | aiReturn_FAILURE -1 | aiReturn_OUTOFMEMORY -3
	mat->Get(AI_MATKEY_NAME, name); // may be "" after the input mesh processing. Must be aiString type!

	if ((retValue = aiGetMaterialColor(mat, AI_MATKEY_COLOR_DIFFUSE, &color)) != AI_SUCCESS)
		color = aiColor4D(0.0f, 0.0f, 0.0f, 0.0f);

	(*geometry)->diffuse = glm::vec3(color.r, color.g, color.b);

	if ((retValue = aiGetMaterialColor(mat, AI_MATKEY_COLOR_AMBIENT, &color)) != AI_SUCCESS)
		color = aiColor4D(0.0f, 0.0f, 0.0f, 0.0f);
	(*geometry)->ambient = glm::vec3(color.r, color.g, color.b);

	if ((retValue = aiGetMaterialColor(mat, AI_MATKEY_COLOR_SPECULAR, &color)) != AI_SUCCESS)
		color = aiColor4D(0.0f, 0.0f, 0.0f, 0.0f);
	(*geometry)->specular = glm::vec3(color.r, color.g, color.b);

	ai_real shininess, strength;
	unsigned int max;	// changed: to unsigned

	max = 1;
	if ((retValue = aiGetMaterialFloatArray(mat, AI_MATKEY_SHININESS, &shininess, &max)) != AI_SUCCESS)
		shininess = 1.0f;
	max = 1;
	if ((retValue = aiGetMaterialFloatArray(mat, AI_MATKEY_SHININESS_STRENGTH, &strength, &max)) != AI_SUCCESS)
		strength = 1.0f;
	(*geometry)->shininess = shininess * strength;

	(*geometry)->texture = 0;

	// load texture image
	if (mat->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
		// get texture name 
		aiString path; // filename

		aiReturn texFound = mat->GetTexture(aiTextureType_DIFFUSE, 0, &path);
		std::string textureName = path.data;

		size_t found = fileName.find_last_of("/\\");
		// insert correct texture file path 
		if (found != std::string::npos) { // not found
										  //subMesh_p->textureName.insert(0, "/");
			textureName.insert(0, fileName.substr(0, found + 1));
		}

		std::cout << "Loading texture file: " << textureName << std::endl;
		(*geometry)->texture = pgr::createTexture(textureName);
	}
	CHECK_GL_ERROR();

	glGenVertexArrays(1, &((*geometry)->vertexArrayObject));
	glBindVertexArray((*geometry)->vertexArrayObject);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, (*geometry)->elementBufferObject); // bind our element array buffer (indices) to vao
	glBindBuffer(GL_ARRAY_BUFFER, (*geometry)->vertexBufferObject);

	glEnableVertexAttribArray(shader.posLocation);
	glVertexAttribPointer(shader.posLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glEnableVertexAttribArray(shader.normalLocation);
	glVertexAttribPointer(shader.normalLocation, 3, GL_FLOAT, GL_FALSE, 0, (void*)(3 * sizeof(float) * mesh->mNumVertices));


	glEnableVertexAttribArray(shader.texCoordLocation);
	glVertexAttribPointer(shader.texCoordLocation, 2, GL_FLOAT, GL_FALSE, 0, (void*)(6 * sizeof(float) * mesh->mNumVertices));
	CHECK_GL_ERROR();

	glBindVertexArray(0);

	(*geometry)->numTriangles = mesh->mNumFaces;

	return true;
}

void setTransformUniforms( const glm::mat4 &modelMatrix, const glm::mat4 &viewMatrix, const glm::mat4 &projectionMatrix )
{
	glm::mat4 PVMmatrix = projectionMatrix * viewMatrix * modelMatrix;

	glUniformMatrix4fv(shaderProgram.PVMmatrixLocation, 1, GL_FALSE, glm::value_ptr(PVMmatrix));

	glUniformMatrix4fv(shaderProgram.VmatrixLocation, 1, GL_FALSE, glm::value_ptr(viewMatrix));
	glUniformMatrix4fv(shaderProgram.MmatrixLocation, 1, GL_FALSE, glm::value_ptr(modelMatrix));

	//glm::mat4 normalMatrix = glm::transpose(glm::inverse(modelMatrix));

	const glm::mat4 modelRotationMatrix = glm::mat4(
		modelMatrix[0],
		modelMatrix[1],
		modelMatrix[2],
		glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
	);
	glm::mat4 normalMatrix = glm::transpose(glm::inverse(modelRotationMatrix));


	glUniformMatrix4fv(shaderProgram.normalMatrixLocation, 1, GL_FALSE, glm::value_ptr(normalMatrix));
}

void setMaterialUniforms( const glm::vec3 &ambient, const glm::vec3 &diffuse, const glm::vec3 &specular, float shininess, GLuint texture )
{
	glUniform3fv(shaderProgram.diffuseLocation, 1, glm::value_ptr(diffuse));	// 2nd parameter must be 1 - it declares number of vectors in the vector array
	glUniform3fv(shaderProgram.ambientLocation, 1, glm::value_ptr(ambient));
	glUniform3fv(shaderProgram.specularLocation, 1, glm::value_ptr(specular));
	glUniform1f(shaderProgram.shininessLocation, shininess);

	if (texture != 0) 
	{
		glUniform1i(shaderProgram.useTextureLocation, 1);	// do texture sampling
		glUniform1i(shaderProgram.texSamplerLocation, 0);	// texturing unit 0 -> samplerID   [for the GPU linker]
		glActiveTexture(GL_TEXTURE0 + 0);					// texturing unit 0 -> to be bound [for OpenGL BindTexture]
		glBindTexture(GL_TEXTURE_2D, texture);
	}
	else
	{
		glUniform1i(shaderProgram.useTextureLocation, 0);
	}
}

void drawSkybox ( const glm::mat4 & viewMatrix, const glm::mat4 & projectionMatrix )
{
	glUseProgram(skyboxShaderProgram.program);

	glm::mat4 matrix = projectionMatrix * viewMatrix;

	glm::mat4 viewRotation = viewMatrix;
	viewRotation[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

	glm::mat4 inversePVmatrix = glm::inverse(projectionMatrix * viewRotation);

	glUniformMatrix4fv(skyboxShaderProgram.inversePVmatrixLocation, 1, GL_FALSE, glm::value_ptr(inversePVmatrix));
	glUniform1i(skyboxShaderProgram.skyboxSamplerLocation, 0);

	glBindVertexArray(skyboxGeometry->vertexArrayObject);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxGeometry->texture);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, skyboxGeometry->numTriangles + 2);

	glBindVertexArray(0);
	glUseProgram(0);
}

void drawBroom ( BroomObject * broom, const glm::mat4 & viewMatrix, const glm::mat4 & projectionMatrix )
{
	glUseProgram(shaderProgram.program);

	//										position		   front				up vector
	glm::mat4 modelMatrix = alignObject ( broom->position, broom->direction, glm::vec3( 0.0f, 1.0f, 0.0f ) );
	modelMatrix = glm::rotate(modelMatrix, glm::radians(0.0f), glm::vec3(0, 1, 0));
	modelMatrix = glm::scale(modelMatrix, glm::vec3(broom->size));	

	setTransformUniforms(modelMatrix, viewMatrix, projectionMatrix);

	setMaterialUniforms(broomGeometry->ambient, broomGeometry->diffuse, broomGeometry->specular, 
		broomGeometry->shininess, broomGeometry->texture);

	// bind VAO
	glBindVertexArray(broomGeometry->vertexArrayObject);

	// draw Castle													0 = location where indices are stored
	glDrawElements(GL_TRIANGLES, broomGeometry->numTriangles * 3, GL_UNSIGNED_INT, 0);

	// unbind VAO, shader
	glBindVertexArray(0);
	glUseProgram(0);
}

void drawCauldron ( Object * cauldron, const glm::mat4 & viewMatrix, const glm::mat4 & projectionMatrix )
{
	glUseProgram(shaderProgram.program);

	glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), cauldron->position);
	modelMatrix = glm::scale(modelMatrix, glm::vec3(cauldron->size));

	setTransformUniforms(modelMatrix, viewMatrix, projectionMatrix);

	setMaterialUniforms(cauldronGeometry->ambient, cauldronGeometry->diffuse, cauldronGeometry->specular, 
		cauldronGeometry->shininess, cauldronGeometry->texture);

	// bind VAO
	glBindVertexArray(cauldronGeometry->vertexArrayObject);

	// draw Castle													0 = location where indices are stored
	glDrawElements(GL_TRIANGLES, cauldronGeometry->numTriangles * 3, GL_UNSIGNED_INT, 0);

	// unbind VAO, shader
	glBindVertexArray(0);
	glUseProgram(0);
}

void drawCastle( Object * castle, const glm::mat4 & viewMatrix, const glm::mat4 & projectionMatrix )
{
	glUseProgram(shaderProgram.program);

	glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), castle->position);
	modelMatrix = glm::scale(modelMatrix, glm::vec3(castle->size));

	setTransformUniforms(modelMatrix, viewMatrix, projectionMatrix);

	setMaterialUniforms(castleGeometry->ambient, castleGeometry->diffuse, castleGeometry->specular, 
		castleGeometry->shininess, castleGeometry->texture);

	// bind VAO
	glBindVertexArray(castleGeometry->vertexArrayObject);

	// draw Castle													0 = location where indices are stored
	glDrawElements(GL_TRIANGLES, castleGeometry->numTriangles * 3, GL_UNSIGNED_INT, 0);

	// unbind VAO, shader
	glBindVertexArray(0);
	glUseProgram(0);
}

void drawWand ( Object * wand, const glm::mat4 & viewMatrix, const glm::mat4 & projectionMatrix )
{
	glUseProgram(shaderProgram.program);

	glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), wand->position);
	modelMatrix = glm::scale(modelMatrix, glm::vec3(wand->size));

	setTransformUniforms(modelMatrix, viewMatrix, projectionMatrix);

	setMaterialUniforms(wandGeometry->ambient, wandGeometry->diffuse, wandGeometry->specular, wandGeometry->shininess, wandGeometry->texture);

	// bind VAO
	glBindVertexArray(wandGeometry->vertexArrayObject);

	// draw Castle													0 = location where indices are stored
	glDrawElements(GL_TRIANGLES, wandGeometry->numTriangles * 3, GL_UNSIGNED_INT, 0);

	// unbind VAO, shader
	glBindVertexArray(0);
	glUseProgram(0);
}

void drawTable ( Object * table, const glm::mat4 & viewMatrix, const glm::mat4 & projectionMatrix )
{
	glUseProgram(shaderProgram.program);

	glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), table->position);
	modelMatrix = glm::scale(modelMatrix, glm::vec3(table->size));

	setTransformUniforms(modelMatrix, viewMatrix, projectionMatrix);

	setMaterialUniforms(tableGeometry->ambient, tableGeometry->diffuse, tableGeometry->specular, 
		tableGeometry->shininess, tableGeometry->texture);

	// bind VAO
	glBindVertexArray(tableGeometry->vertexArrayObject);

	// draw Castle													0 = location where indices are stored
	glDrawElements(GL_TRIANGLES, tableGeometry->numTriangles * 3, GL_UNSIGNED_INT, 0);

	// unbind VAO, shader
	glBindVertexArray(0);
	glUseProgram(0);
}

void drawDoor ( Object * door, const glm::mat4 & viewMatrix, const glm::mat4 & projectionMatrix )
{
	glUseProgram(shaderProgram.program);

	glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), door->position);
	modelMatrix = glm::scale(modelMatrix, glm::vec3(door->size));

	setTransformUniforms(modelMatrix, viewMatrix, projectionMatrix);

	setMaterialUniforms(doorGeometry->ambient, doorGeometry->diffuse, doorGeometry->specular, 
		doorGeometry->shininess, doorGeometry->texture);

	// bind VAO
	glBindVertexArray(doorGeometry->vertexArrayObject);

	// draw Castle													0 = location where indices are stored
	glDrawElements(GL_TRIANGLES, doorGeometry->numTriangles * 3, GL_UNSIGNED_INT, 0);

	// unbind VAO, shader
	glBindVertexArray(0);
	glUseProgram(0);
}

void drawOpenedDoor ( Object * door, const glm::mat4 & viewMatrix, const glm::mat4 & projectionMatrix )
{
	glUseProgram(shaderProgram.program);

	glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), door->position);
	modelMatrix = glm::scale(modelMatrix, glm::vec3(door->size));

	setTransformUniforms(modelMatrix, viewMatrix, projectionMatrix);

	setMaterialUniforms(openedDoorGeometry->ambient, openedDoorGeometry->diffuse, openedDoorGeometry->specular, 
		openedDoorGeometry->shininess, openedDoorGeometry->texture);

	// bind VAO
	glBindVertexArray(openedDoorGeometry->vertexArrayObject);

	// draw Castle													0 = location where indices are stored
	glDrawElements(GL_TRIANGLES, openedDoorGeometry->numTriangles * 3, GL_UNSIGNED_INT, 0);

	// unbind VAO, shader
	glBindVertexArray(0);
	glUseProgram(0);
}

void drawGround ( Object * ground, const glm::mat4 & viewMatrix, const glm::mat4 & projectionMatrix )
{
	glUseProgram(shaderProgram.program);

	glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), ground->position);
	modelMatrix = glm::scale(modelMatrix, glm::vec3(ground->size));

	setTransformUniforms(modelMatrix, viewMatrix, projectionMatrix);

	setMaterialUniforms( groundGeometry->ambient, groundGeometry->diffuse, groundGeometry->specular,
		groundGeometry->shininess, groundGeometry->texture );

	glBindVertexArray(groundGeometry->vertexArrayObject);
	CHECK_GL_ERROR();

	glDrawElements(GL_TRIANGLES, groundGeometry->numTriangles * 3, GL_UNSIGNED_INT, 0);
	CHECK_GL_ERROR();

	glBindVertexArray(0);
	glUseProgram(0);
}

void drawTree ( Object * tree, const glm::mat4 & viewMatrix, const glm::mat4 & projectionMatrix )
{
	glUseProgram(shaderProgram.program);

	glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), tree->position);
	modelMatrix = glm::scale(modelMatrix, glm::vec3(tree->size));

	setTransformUniforms(modelMatrix, viewMatrix, projectionMatrix);

	setMaterialUniforms( treeGeometry->ambient, treeGeometry->diffuse, treeGeometry->specular,
		treeGeometry->shininess, treeGeometry->texture );

	glBindVertexArray(treeGeometry->vertexArrayObject);
	CHECK_GL_ERROR();

	glDrawElements(GL_TRIANGLES, treeGeometry->numTriangles * 3, GL_UNSIGNED_INT, 0);
	CHECK_GL_ERROR();

	glBindVertexArray(0);
	glUseProgram(0);
}

void drawBanner ( Object * banner, const glm::mat4 & viewMatrix, const glm::mat4 & projectionMatrix )
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDisable(GL_DEPTH_TEST);

	glUseProgram(bannerShaderProgram.program);
	CHECK_GL_ERROR();

	glm::mat4 matrix = glm::translate(glm::mat4(1.0f), banner->position);
	matrix = glm::scale(matrix, glm::vec3(banner->size));
	CHECK_GL_ERROR();

	glm::mat4 PVMmatrix = projectionMatrix * viewMatrix * matrix;
	glUniformMatrix4fv(bannerShaderProgram.PVMmatrixLocation, 1, GL_FALSE, glm::value_ptr(PVMmatrix));   
	CHECK_GL_ERROR();
	glUniform1i(bannerShaderProgram.texSamplerLocation, 0);
	CHECK_GL_ERROR();

	glBindTexture(GL_TEXTURE_2D, bannerGeometry->texture);
	glBindVertexArray(bannerGeometry->vertexArrayObject);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, bannerGeometry->numTriangles);

	CHECK_GL_ERROR();

	glBindVertexArray(0);
	glUseProgram(0);

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
}

void drawAnimatedBanner ( Object * banner, const glm::mat4 & viewMatrix, const glm::mat4 & projectionMatrix )
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDisable(GL_DEPTH_TEST);

	glUseProgram(animBannerShaderProgram.program);
	CHECK_GL_ERROR();

	glm::mat4 matrix = glm::translate(glm::mat4(1.0f), banner->position);
	matrix = glm::scale(matrix, glm::vec3(banner->size));
	CHECK_GL_ERROR();

	glm::mat4 PVMmatrix = projectionMatrix * viewMatrix * matrix;
	glUniformMatrix4fv(animBannerShaderProgram.PVMmatrixLocation, 1, GL_FALSE, glm::value_ptr(PVMmatrix));   
	glUniform1f(animBannerShaderProgram.timeLocation, banner->currentTime - banner->startTime);
	glUniform1i(animBannerShaderProgram.texSamplerLocation, 0);
	CHECK_GL_ERROR();

	glBindTexture(GL_TEXTURE_2D, animBannerGeometry->texture);
	glBindVertexArray(animBannerGeometry->vertexArrayObject);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, animBannerGeometry->numTriangles);

	CHECK_GL_ERROR();

	glBindVertexArray(0);
	glUseProgram(0);

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
}

void drawFlame ( FlameObject * flame, const glm::mat4 & viewMatrix, const glm::mat4 & projectionMatrix )
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	glUseProgram(flameShaderProgram.program);

	glm::mat4 billboardRotationMatrix = glm::mat4(
		viewMatrix[0],
		viewMatrix[1],
		viewMatrix[2],
		glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
	);
	billboardRotationMatrix = glm::transpose(billboardRotationMatrix);

	glm::mat4 matrix = glm::translate(glm::mat4(1.0f), flame->position);
	matrix = glm::scale(matrix, glm::vec3(flame->size));
	matrix = matrix*billboardRotationMatrix; 

	glm::mat4 PVMmatrix = projectionMatrix * viewMatrix * matrix;
	glUniformMatrix4fv(flameShaderProgram.PVMmatrixLocation, 1, GL_FALSE, glm::value_ptr(PVMmatrix)); 
	glUniformMatrix4fv(flameShaderProgram.VmatrixLocation, 1, GL_FALSE, glm::value_ptr(viewMatrix));  
	glUniform1f(flameShaderProgram.timeLocation, flame->currentTime - flame->startTime);
	glUniform1i(flameShaderProgram.texSamplerLocation, 0);
	glUniform1f(flameShaderProgram.frameDurationLocation, flame->frameDuration);

	glBindVertexArray(flameGeometry->vertexArrayObject);
	glBindTexture(GL_TEXTURE_2D, flameGeometry->texture);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, flameGeometry->numTriangles);

	glBindVertexArray(0);
	glUseProgram(0);

	glDisable(GL_BLEND);
}

void initializeBroom ( void )
{
	if (!loadSingleMesh(BROOM_STICK_FILE, shaderProgram, &broomGeometry))
	{
		std::cerr << "couldn't load broomstick mesh" << std::endl;
	}
	CHECK_GL_ERROR();
}

void initializeCauldron ( void )
{
	if (!loadSingleMesh(CAULDRON_FILE, shaderProgram, &cauldronGeometry))
	{
		std::cerr << "couldn't load cauldron mesh" << std::endl;
	}
	CHECK_GL_ERROR();
}

void initializeCastle ( void )
{
	if (!loadSingleMesh(CASTLE_FILE, shaderProgram, &castleGeometry))
	{
		std::cerr << "couldn't load castle mesh" << std::endl;
	}
	else
		std::cout << "castle loaded" << std::endl;
	CHECK_GL_ERROR();
}

void initializeWand ( void )
{
	if (!loadSingleMesh(WAND_FILE, shaderProgram, &wandGeometry))
	{
		std::cerr << "couldn't load wand mesh" << std::endl;
	}
	CHECK_GL_ERROR();
}

void initializeTable ( void )
{
	if (!loadSingleMesh(WOODEN_TABLE_FILE, shaderProgram, &tableGeometry))
	{
		std::cerr << "couldn't load table mesh" << std::endl;
	}
	CHECK_GL_ERROR();
}

void initializeDoor ( void )
{
	if (!loadSingleMesh(WOODEN_DOOR_FILE, shaderProgram, &doorGeometry))
	{
		std::cerr << "couldn't load door mesh" << std::endl;
	}
	CHECK_GL_ERROR();
}

void initializeOpenedDoor ( void )
{
	if (!loadSingleMesh(WOODEN_DOOR_OPENED_FILE, shaderProgram, &openedDoorGeometry))
	{
		std::cerr << "couldn't load opened door mesh" << std::endl;
	}
	CHECK_GL_ERROR();
}

void initializeGround ( void )
{
	groundGeometry = new MeshGeometry;

	groundGeometry->texture = pgr::createTexture(GROUND_TEXTURE_FILE);
	glBindTexture(GL_TEXTURE_2D, groundGeometry->texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glGenVertexArrays(1, &(groundGeometry->vertexArrayObject));
	glBindVertexArray(groundGeometry->vertexArrayObject);

	glGenBuffers(1, &(groundGeometry->vertexBufferObject));
	glBindBuffer(GL_ARRAY_BUFFER, groundGeometry->vertexBufferObject);
	glBufferData(GL_ARRAY_BUFFER, sizeof(groundVertices), groundVertices, GL_STATIC_DRAW);

	glGenBuffers(1, &(groundGeometry->elementBufferObject));
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, groundGeometry->elementBufferObject);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3 * sizeof(unsigned int) * groundTrianglesCount, groundIndices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(shaderProgram.posLocation);
	glVertexAttribPointer(shaderProgram.posLocation, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0);

	glEnableVertexAttribArray(shaderProgram.texCoordLocation);
	glVertexAttribPointer(shaderProgram.texCoordLocation, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));

	glEnableVertexAttribArray(shaderProgram.normalLocation);
	glVertexAttribPointer(shaderProgram.normalLocation, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));


	groundGeometry->ambient = glm::vec3(0.0f, 0.2f, 0.0f);
	groundGeometry->diffuse = glm::vec3(0.0f, 0.5f, 0.0f);
	groundGeometry->specular = glm::vec3(0.0f, 0.0f, 0.0f);
	groundGeometry->shininess = 1.0f;

	glBindVertexArray(0);

	groundGeometry->numTriangles = groundTrianglesCount;
}

void initializeTree ( void )
{
	treeGeometry = new MeshGeometry;

	treeGeometry->texture = pgr::createTexture(TREE_TEXTURE_FILE);
	glBindTexture(GL_TEXTURE_2D, treeGeometry->texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glGenVertexArrays(1, &(treeGeometry->vertexArrayObject));
	glBindVertexArray(treeGeometry->vertexArrayObject);

	glGenBuffers(1, &(treeGeometry->vertexBufferObject));
	glBindBuffer(GL_ARRAY_BUFFER, treeGeometry->vertexBufferObject);
	glBufferData(GL_ARRAY_BUFFER, sizeof(treeVertices), treeVertices, GL_STATIC_DRAW);

	glGenBuffers(1, &(treeGeometry->elementBufferObject));
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, treeGeometry->elementBufferObject);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3 * sizeof(unsigned int) * treeNTriangles, treeTriangles, GL_STATIC_DRAW);

	glEnableVertexAttribArray(shaderProgram.posLocation);
	glVertexAttribPointer(shaderProgram.posLocation, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0);

	glEnableVertexAttribArray(shaderProgram.texCoordLocation);
	glVertexAttribPointer(shaderProgram.texCoordLocation, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));

	glEnableVertexAttribArray(shaderProgram.normalLocation);
	glVertexAttribPointer(shaderProgram.normalLocation, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));


	treeGeometry->ambient = glm::vec3(0.0f, 0.2f, 0.0f);
	treeGeometry->diffuse = glm::vec3(0.0f, 0.5f, 0.0f);
	treeGeometry->specular = glm::vec3(0.0f, 0.0f, 0.0f);
	treeGeometry->shininess = 1.0f;

	glBindVertexArray(0);

	treeGeometry->numTriangles = treeNTriangles;
}

void initializeBanner ( void )
{
	bannerGeometry = new MeshGeometry;

	bannerGeometry->texture = pgr::createTexture(BANNER_TEXTURE_FILE);
	glBindTexture(GL_TEXTURE_2D, bannerGeometry->texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	glGenVertexArrays(1, &(bannerGeometry->vertexArrayObject));
	glBindVertexArray(bannerGeometry->vertexArrayObject);

	glGenBuffers(1, &(bannerGeometry->vertexBufferObject));
	glBindBuffer(GL_ARRAY_BUFFER, bannerGeometry->vertexBufferObject);
	glBufferData(GL_ARRAY_BUFFER, sizeof(bannerVertexData), bannerVertexData, GL_STATIC_DRAW);

	glEnableVertexAttribArray(bannerShaderProgram.posLocation);
	glEnableVertexAttribArray(bannerShaderProgram.texCoordLocation);
	// vertices of triangles - start at the beginning of the interlaced array
	glVertexAttribPointer(bannerShaderProgram.posLocation, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);
	// texture coordinates of each vertices are stored just after its position
	glVertexAttribPointer(bannerShaderProgram.texCoordLocation, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

	glBindVertexArray(0);

	bannerGeometry->numTriangles = bannerNumQuadVertices;
}

void initializeAnimatedBanner ( void )
{
	animBannerGeometry = new MeshGeometry;

	animBannerGeometry->texture = pgr::createTexture(ANIM_BANNER_TEXTURE_FILE);
	glBindTexture(GL_TEXTURE_2D, animBannerGeometry->texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	glGenVertexArrays(1, &(animBannerGeometry->vertexArrayObject));
	glBindVertexArray(animBannerGeometry->vertexArrayObject);

	glGenBuffers(1, &(animBannerGeometry->vertexBufferObject));
	glBindBuffer(GL_ARRAY_BUFFER, animBannerGeometry->vertexBufferObject);
	glBufferData(GL_ARRAY_BUFFER, sizeof(bannerVertexData), bannerVertexData, GL_STATIC_DRAW);

	glEnableVertexAttribArray(animBannerShaderProgram.posLocation);
	glEnableVertexAttribArray(animBannerShaderProgram.texCoordLocation);
	// vertices of triangles - start at the beginning of the interlaced array
	glVertexAttribPointer(animBannerShaderProgram.posLocation, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);
	// texture coordinates of each vertices are stored just after its position
	glVertexAttribPointer(animBannerShaderProgram.texCoordLocation, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

	glBindVertexArray(0);

	animBannerGeometry->numTriangles = bannerNumQuadVertices;
}

void initializeFlame ( void )
{
	flameGeometry = new MeshGeometry;

	glGenVertexArrays(1, &(flameGeometry->vertexArrayObject));
	glBindVertexArray(flameGeometry->vertexArrayObject);

	glGenBuffers(1, &(flameGeometry->vertexBufferObject));
	glBindBuffer(GL_ARRAY_BUFFER, flameGeometry->vertexBufferObject);
	glBufferData(GL_ARRAY_BUFFER, sizeof(flameVertexData), flameVertexData, GL_STATIC_DRAW);

	glEnableVertexAttribArray(flameShaderProgram.posLocation);
	glVertexAttribPointer(flameShaderProgram.posLocation, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);

	glEnableVertexAttribArray(flameShaderProgram.texCoordLocation);
	glVertexAttribPointer(flameShaderProgram.texCoordLocation, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

	glBindVertexArray(0);

	flameGeometry->texture = pgr::createTexture(FLAME_TEXTURE_FILE);
	flameGeometry->numTriangles = flameNumQuadVertices;
}

void initializeSkybox(GLuint shader, MeshGeometry ** geometry)
{
	*geometry = new MeshGeometry;

	// 2D coordinates of 2 triangles covering the whole screen (NDC), draw using triangle strip
	static const float screenCoords[] = {
		-1.0f, -1.0f,
		1.0f, -1.0f,
		-1.0f,  1.0f,
		1.0f,  1.0f
	};

	glGenVertexArrays(1, &((*geometry)->vertexArrayObject));
	glBindVertexArray((*geometry)->vertexArrayObject);
	CHECK_GL_ERROR();

	// buffer for far plane rendering
	glGenBuffers(1, &((*geometry)->vertexBufferObject));
	glBindBuffer(GL_ARRAY_BUFFER, (*geometry)->vertexBufferObject);
	glBufferData(GL_ARRAY_BUFFER, sizeof(screenCoords), screenCoords, GL_STATIC_DRAW);
	CHECK_GL_ERROR();

	std::cout << skyboxShaderProgram.screenCoordLocation << "\n";

	glEnableVertexAttribArray(skyboxShaderProgram.screenCoordLocation);
	CHECK_GL_ERROR();
	glVertexAttribPointer(skyboxShaderProgram.screenCoordLocation, 2, GL_FLOAT, GL_FALSE, 0, 0);
	CHECK_GL_ERROR();

	glBindVertexArray(0);
	glUseProgram(0);
	CHECK_GL_ERROR();

	(*geometry)->numTriangles = 2;

	glActiveTexture(GL_TEXTURE0);

	glGenTextures(1, &((*geometry)->texture));
	glBindTexture(GL_TEXTURE_CUBE_MAP, (*geometry)->texture);

	GLuint targets[] = 
	{
		GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
		GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
		GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
	};

	std::vector<const GLchar*> faces;

	faces.push_back("vendor/skybox/hills_rt.jpg");
	faces.push_back("vendor/skybox/hills_lf.jpg");
	faces.push_back("vendor/skybox/hills_up.jpg");
	faces.push_back("vendor/skybox/hills_dn.jpg");
	faces.push_back("vendor/skybox/hills_bk.jpg");
	faces.push_back("vendor/skybox/hills_ft.jpg");

	for( int i = 0; i < 6; i++ ) 
	{
		std::string texName = faces[i];

		std::cout << "Loading cube map texture: " << texName << std::endl;

		if(!pgr::loadTexImage2D(texName, targets[i])) 
		{
			pgr::dieWithError("Skybox cube map loading failed!");
		}
	}

	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	// unbind the texture (just in case someone will mess up with texture calls later)
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	CHECK_GL_ERROR();
}

void initializeShaderPrograms( void )
{
	std::vector<GLuint> shaderList;

	// push back shaders for objects
	shaderList.push_back( pgr::createShaderFromFile ( GL_VERTEX_SHADER, "perFrag.vs" ) );
	shaderList.push_back( pgr::createShaderFromFile ( GL_FRAGMENT_SHADER, "perFrag.fs" ) );

	shaderProgram.program = pgr::createProgram ( shaderList );

	//load attrib locations from shaderProgram.program
	shaderProgram.posLocation = glGetAttribLocation(shaderProgram.program, "position");
	shaderProgram.normalLocation = glGetAttribLocation(shaderProgram.program, "normal");
	shaderProgram.texCoordLocation = glGetAttribLocation(shaderProgram.program, "texCoord");

	shaderProgram.PVMmatrixLocation = glGetUniformLocation(shaderProgram.program, "PVMmatrix");
	shaderProgram.VmatrixLocation = glGetUniformLocation(shaderProgram.program, "Vmatrix");
	shaderProgram.MmatrixLocation = glGetUniformLocation(shaderProgram.program, "Mmatrix");
	shaderProgram.normalMatrixLocation = glGetUniformLocation(shaderProgram.program, "normalMatrix");
	shaderProgram.timeLocation = glGetUniformLocation(shaderProgram.program, "time");
	shaderProgram.ambientLocation = glGetUniformLocation(shaderProgram.program, "material.ambient");
	shaderProgram.diffuseLocation = glGetUniformLocation(shaderProgram.program, "material.diffuse");
	shaderProgram.specularLocation = glGetUniformLocation(shaderProgram.program, "material.specular");
	shaderProgram.shininessLocation = glGetUniformLocation(shaderProgram.program, "material.shininess");

	shaderProgram.texSamplerLocation = glGetUniformLocation(shaderProgram.program, "texSampler");
	shaderProgram.useTextureLocation = glGetUniformLocation(shaderProgram.program, "material.useTexture");

	shaderProgram.reflectorPositionLocation = glGetUniformLocation(shaderProgram.program, "reflectorPosition");
	shaderProgram.reflectorDirectionLocation = glGetUniformLocation(shaderProgram.program, "reflectorDirection");
	shaderProgram.reflectorLocation = glGetUniformLocation(shaderProgram.program, "reflectOn");

	shaderProgram.fogLocation = glGetUniformLocation(shaderProgram.program, "fogOn");

	shaderProgram.dirLightLocation = glGetUniformLocation(shaderProgram.program, "dirLight");

	shaderProgram.cauldronLightLocation = glGetUniformLocation(shaderProgram.program, "cauldronLight");

	shaderList.clear();

	shaderList.push_back(pgr::createShaderFromFile(GL_VERTEX_SHADER, "banner.vs"));
	shaderList.push_back(pgr::createShaderFromFile(GL_FRAGMENT_SHADER, "banner.fs"));

	bannerShaderProgram.program = pgr::createProgram(shaderList);

	bannerShaderProgram.posLocation = glGetAttribLocation(bannerShaderProgram.program, "position");
	bannerShaderProgram.texCoordLocation = glGetAttribLocation(bannerShaderProgram.program, "texCoord");

	bannerShaderProgram.PVMmatrixLocation = glGetUniformLocation(bannerShaderProgram.program, "PVMmatrix");
	bannerShaderProgram.texSamplerLocation = glGetUniformLocation(bannerShaderProgram.program, "texSampler");

	shaderList.clear();

	shaderList.push_back(pgr::createShaderFromFile(GL_VERTEX_SHADER, "animBanner.vs"));
	shaderList.push_back(pgr::createShaderFromFile(GL_FRAGMENT_SHADER, "animBanner.fs"));

	animBannerShaderProgram.program = pgr::createProgram(shaderList);

	animBannerShaderProgram.posLocation = glGetAttribLocation(animBannerShaderProgram.program, "position");
	animBannerShaderProgram.texCoordLocation = glGetAttribLocation(animBannerShaderProgram.program, "texCoord");
	animBannerShaderProgram.PVMmatrixLocation = glGetUniformLocation(animBannerShaderProgram.program, "PVMmatrix");
	animBannerShaderProgram.timeLocation = glGetUniformLocation(animBannerShaderProgram.program, "time");
	animBannerShaderProgram.texSamplerLocation = glGetUniformLocation(animBannerShaderProgram.program, "texSampler");

	shaderList.clear();

	shaderList.push_back(pgr::createShaderFromFile(GL_VERTEX_SHADER, "skybox.vs"));
	shaderList.push_back(pgr::createShaderFromFile(GL_FRAGMENT_SHADER, "skybox.fs"));

	skyboxShaderProgram.program = pgr::createProgram(shaderList);

	skyboxShaderProgram.screenCoordLocation = glGetAttribLocation(skyboxShaderProgram.program, "screenCoord");

	skyboxShaderProgram.skyboxSamplerLocation = glGetUniformLocation(skyboxShaderProgram.program, "skyboxSampler");
	skyboxShaderProgram.inversePVmatrixLocation = glGetUniformLocation(skyboxShaderProgram.program, "inversePVmatrix");
	skyboxShaderProgram.fogOnLocation = glGetUniformLocation(skyboxShaderProgram.program, "fogOn");

	shaderList.clear();

	shaderList.push_back(pgr::createShaderFromFile(GL_VERTEX_SHADER, "flame.vs"));
	shaderList.push_back(pgr::createShaderFromFile(GL_FRAGMENT_SHADER, "flame.fs"));

	flameShaderProgram.program = pgr::createProgram(shaderList);
	flameShaderProgram.posLocation = glGetAttribLocation(flameShaderProgram.program, "position");
	flameShaderProgram.texCoordLocation = glGetAttribLocation(flameShaderProgram.program, "texCoord");
	flameShaderProgram.PVMmatrixLocation = glGetUniformLocation(flameShaderProgram.program, "PVMmatrix");
	flameShaderProgram.VmatrixLocation = glGetUniformLocation(flameShaderProgram.program, "Vmatrix");
	flameShaderProgram.timeLocation = glGetUniformLocation(flameShaderProgram.program, "time");
	flameShaderProgram.texSamplerLocation = glGetUniformLocation(flameShaderProgram.program, "texSampler");
	flameShaderProgram.frameDurationLocation = glGetUniformLocation(flameShaderProgram.program, "frameDuration");
	shaderList.clear();
}

void initializeModels( void )
{
	initializeGround ( );
	initializeBroom ( );
	initializeCauldron ( );
	initializeCastle ( );
	initializeWand ( );
	initializeTable ( );
	initializeTree ( );
	initializeDoor ( );	
	initializeOpenedDoor ( );

	initializeSkybox ( skyboxShaderProgram.program, &skyboxGeometry );
	initializeBanner ( );
	initializeAnimatedBanner ( );

	initializeFlame ( );
}

void cleanupShaderPrograms( void )
{
	pgr::deleteProgramAndShaders ( shaderProgram.program );
	pgr::deleteProgramAndShaders ( skyboxShaderProgram.program );
	pgr::deleteProgramAndShaders ( bannerShaderProgram.program );
	pgr::deleteProgramAndShaders ( flameShaderProgram.program );
}

void cleanupGeometry(MeshGeometry * geometry)
{
	glDeleteVertexArrays(1, &(geometry->vertexArrayObject));
	glDeleteBuffers(1, &(geometry->elementBufferObject));
	glDeleteBuffers(1, &(geometry->vertexBufferObject));

	if (geometry->texture)
	{
		glDeleteTextures(1, &(geometry->texture));
	}
}

void cleanupModels( void ) 
{
	cleanupGeometry( castleGeometry );
	cleanupGeometry( skyboxGeometry );
	cleanupGeometry( broomGeometry );
	cleanupGeometry( cauldronGeometry );
	cleanupGeometry( wandGeometry );
	cleanupGeometry( tableGeometry );
	cleanupGeometry( doorGeometry );
	cleanupGeometry( openedDoorGeometry );
	cleanupGeometry( groundGeometry );
	cleanupGeometry( bannerGeometry );
	cleanupGeometry( flameGeometry );	
}