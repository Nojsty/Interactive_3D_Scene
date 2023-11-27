#version 140

uniform mat4 PVMmatrix;    

in vec3 position;           // vertex position in world space
in vec2 texCoord;           // incoming texture coordinates

smooth out vec2 texCoord_v; // outgoing texture coordinates

void main() {

  gl_Position = PVMmatrix * vec4(position, 1.0);   // outgoing vertex in clip coordinates
  
  texCoord_v = texCoord;
}