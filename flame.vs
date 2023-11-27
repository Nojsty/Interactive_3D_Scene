#version 140

uniform mat4 PVMmatrix;    

in vec3 position;           
in vec2 texCoord;    

smooth out vec2 texCoord_v; 

void main ( ) 
{	
	gl_Position = PVMmatrix * vec4(position, 1.0f);
	texCoord_v = texCoord;
}
