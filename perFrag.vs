#version 140

struct Material 
{      
  vec3  ambient;   
  vec3  diffuse;   
  vec3  specular;  
  float shininess; 

  bool  useTexture;
};

in vec3 position;
in vec3 normal;
in vec2 texCoord;

uniform sampler2D texSampler;  

uniform Material material;    
 
uniform int cauldronLight;
uniform bool dirLight;
uniform float time;

uniform mat4 PVMmatrix;     
uniform mat4 Vmatrix;       
uniform mat4 Mmatrix;       
uniform mat4 normalMatrix;

uniform vec3 reflectorPosition;
uniform vec3 reflectorDirection;
uniform bool reflectOn;

smooth out vec2 texCoord_v;  
smooth out vec3 fragPositionCamera;
smooth out vec3 fragNormalCamera;

void main ( void ) 
{
  fragPositionCamera = (Vmatrix * Mmatrix * vec4(position, 1.0)).xyz;  
  fragNormalCamera = normalize((Vmatrix * Mmatrix * vec4(normal, 0.0)).xyz);
  
  texCoord_v = texCoord;

  gl_Position = PVMmatrix * vec4(position, 1);  
}
