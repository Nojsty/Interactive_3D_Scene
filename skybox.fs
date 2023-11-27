#version 140

in vec3 texCoord_v;

out vec4 color_f;

uniform samplerCube skyboxSampler;
uniform bool fogOn;

vec4 addFog ( vec4 outputColor )
{
	float distance = length(vec4(1.0f));
	float density = 1.0f;
	float gradient = 2.0f;

	float blendFactor = exp(-distance*distance*density*density);

	blendFactor = clamp(blendFactor, 0.0f,1.0f);

	outputColor = outputColor * blendFactor + (1-blendFactor)*vec4(0.8f);
	outputColor = vec4(0.8f);

	return outputColor;
}

void main() 
{
	color_f = texture(skyboxSampler, texCoord_v);

	if ( fogOn )
		color_f = addFog ( color_f ); 
}