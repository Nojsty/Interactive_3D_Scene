#version 140

uniform mat4 PVMmatrix;     
uniform float time;         

in vec3 position;           // vertex position in world space
in vec2 texCoord;           // incoming texture coordinates

smooth out vec2 texCoord_v; // outgoing texture coordinates

float decay = 0.3;


void main ( ) 
{
  gl_Position = PVMmatrix * vec4(position, 1.0);   // outgoing vertex in clip coordinates
  
  vec2 offset;
  float localTime = time * decay;

  if ( localTime >= 0.5f )
  	offset = vec2(0.0, -0.5f);
  else
	offset = vec2 ( 0.0, ( floor( localTime ) - localTime ) * 3 + 1.0f );

  texCoord_v = texCoord + offset;
}
