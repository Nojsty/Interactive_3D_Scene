#version 140

uniform float time;           
uniform mat4 Vmatrix;         
uniform sampler2D texSampler; 

// number of frames in an image
uniform ivec2 pattern = ivec2(4, 4);
uniform float frameDuration = 0.1f;
uniform int frame;

smooth in vec3 position_v;    
smooth in vec2 texCoord_v;    

out vec4 color_f;

vec4 sampleTexture ( int frame ) 
{	
	vec2 texCoordStart = texCoord_v / vec2(pattern);
	vec2 texCoordFinal = texCoordStart + vec2(frame % pattern.x, pattern.y - 1 - frame/pattern.y)/vec2(pattern);
	return texture ( texSampler, texCoordFinal );
}

void main ( ) 
{
	int frame = int( time / frameDuration );
	color_f = sampleTexture ( frame );	
}