#version 140

struct Material 
{          
	vec3  ambient;           
	vec3  diffuse;            
	vec3  specular;           
	float shininess;         
	bool  useTexture;         
};

struct Light 
{
	vec3  ambient;
	vec3  diffuse;
	vec3  specular;

	vec3  position;

	vec3  spotDirection;
	float spotCosCutOff;
	float spotExponent;
	vec3 attenuation;
};

// uniforms
uniform sampler2D texSampler;

uniform Material material;

uniform mat4 Vmatrix;
uniform mat4 Mmatrix;
uniform mat4 normalMatrix;

uniform vec3 reflectorPosition;
uniform vec3 reflectorDirection;
uniform bool reflectOn;

uniform float time;

uniform bool dirLight;
uniform bool fogOn;
uniform int cauldronLight;

// input vectors from vertex shader
smooth in vec4 color_v;        
smooth in vec2 texCoord_v;     
smooth in vec3 fragPositionCamera;
smooth in vec3 fragNormalCamera;

// output fragment color
out vec4 color_f;


vec4 ReflectorLight ( Light light, Material material ) 
{
	vec3 spotPosition = vec3(0.0);
	vec3 spotDirection = normalize((Vmatrix * vec4(light.spotDirection, 0.0)).xyz);

	vec3 L = normalize(spotPosition - fragPositionCamera);
	vec3 R = reflect(-L, fragNormalCamera);
	vec3 V = normalize(-fragPositionCamera);

	vec3 diffuse = max(dot(L,normalize(fragNormalCamera)), 0.0) * material.diffuse * light.diffuse;
	vec3 specular = pow(max(dot(R,V), 0.0), material.shininess) * material.specular * light.specular;
    vec3 ambient = light.ambient * material.ambient;

	vec3 ret = diffuse + specular + ambient;

	float alpha = dot(-L, spotDirection);
	float coef = max(0.0, alpha);

	if (coef < light.spotCosCutOff) 
	{
		ret *= 0.0;
	}
	else 
	{
		ret *= pow(coef, light.spotExponent);
	}

	float dist = distance(spotPosition, fragPositionCamera);
	float attenuation = 1.0 / (light.attenuation.x + light.attenuation.y*dist + light.attenuation.z*pow(dist, 2.0));

	ret *= attenuation;

	return vec4(ret, 1.0);
}

vec4 DirectLight ( Light light, Material material )
{
	 vec3 ret = vec3(0.0);

	 vec3 L = normalize(light.position - fragPositionCamera);
	 vec3 R = reflect(-L, fragNormalCamera);
	 vec3 V = normalize(-fragPositionCamera);

	 vec3 diffuse = max(dot(L,normalize(fragNormalCamera)),0) * material.diffuse * light.diffuse;
	 vec3 specular = pow(max(dot(R,V),0), material.shininess) * material.specular * light.specular;
	 vec3 ambient = light.ambient * material.ambient;

	 ret += diffuse + specular + ambient;

	 return vec4(ret, 1.0);
}

vec4 PointLight(Light light, Material material) 
{
	vec3 ret = vec3(0.0f);
	vec3 lightPosCamera = (Vmatrix * vec4(light.position, 1.0f)).xyz;

	vec3 L = normalize(lightPosCamera - fragPositionCamera);
	vec3 R = reflect(-L, fragNormalCamera);
    vec3 V = normalize(-fragPositionCamera);

	vec3 diffuse = max(dot(L,fragNormalCamera),0) * material.diffuse * light.diffuse;
    vec3 specular = pow(max(dot(R,V),0), material.shininess) * material.specular * light.specular;
    vec3 ambient = light.ambient * material.ambient;

	float dist = distance(fragPositionCamera, lightPosCamera);
	float attenuation = 1.0f / (light.attenuation.x + light.attenuation.y * dist + light.attenuation.z * pow(dist,2));

	ret += attenuation * (diffuse + specular + ambient);

	return vec4(ret, 1.0f);
}


Light sun;
Light point;
Light point2;
Light reflector;

void SetLights ( void )
{
	sun.position = vec3(1,1,0);
	sun.diffuse = vec3 ( 0.55f );
	sun.ambient = vec3 ( 0.1f );
	sun.specular = vec3(0.1f);

	point.position = vec3 ( 4.1f, 1.2f, -22.3f );
	point.diffuse = vec3 ( 1.0f, 0.4f, 0.0f );
	point.ambient = vec3 ( 0.1f );
	point.specular = vec3 ( 0.1f );
	point.attenuation = vec3 ( 0.0f, 0.2f, 0.15f );

	point2.position = vec3 ( 8.3f, 1.2f, -21.2f );
	point2.diffuse = vec3 ( 1.0f, 0.4f, 0.0f );
	point2.ambient = vec3 ( 0.1f );
	point2.specular = vec3 ( 0.1f );
	point2.attenuation = vec3 ( 0.0f, 0.2f, 0.15f );	

	reflector.ambient = vec3 ( 1.0f );
	reflector.diffuse = vec3 ( 2.0f );
	reflector.specular = vec3 ( 2.0f );
	reflector.spotCosCutOff = 0.7f;
	reflector.spotExponent  = 60.0f;
	reflector.attenuation = vec3 ( 0.0f, 0.15f, 0.03f );
	reflector.position = ( Vmatrix * vec4( reflectorPosition, 1.0 ) ).xyz;
	reflector.spotDirection = reflectorDirection;
}

vec4 addFog ( vec4 outputColor )
{
	float distance = length(fragPositionCamera);
	float density = 0.25f;
	float gradient = 2.5f;

	float t = time/3.0f;

	if ( ( t - int(t) ) < 0.5f )
		density = density + ( t-int(t) ) / 10.0f;
	else
		density = density + (1 - (t-int(t)) ) / 10.0f;
	
	float blendFactor = exp(-pow((density*distance),gradient));
	blendFactor = clamp(blendFactor, 0.0f,1.0f);

	outputColor = outputColor * blendFactor + (1-blendFactor)*vec4(0.8f);

	vec4 tmpColor = outputColor;
	
	if ( (t-int(t)) < 0.5f )
		tmpColor = tmpColor + (t-int(t)) / 30.0f;
	else
		tmpColor = tmpColor + (1 - (t-int(t))) / 30.0f;

	outputColor = tmpColor;
	return outputColor;
}

void main()
{

    SetLights ( );

    vec3 globalAmbientLight = vec3 ( 0.16f );
	vec4 outputColor = vec4 ( material.ambient * globalAmbientLight, 0.0f );
	vec4 tmpColor;

	float t = time / 1.8f;

	// directional light
	if(dirLight)
		outputColor += DirectLight(sun, material);
	
	// 1st point light scope
	{
	tmpColor = PointLight(point, material);
	if ( ( t-int(t) ) < 0.5f )
		tmpColor = tmpColor + ( t - int(t) ) / 20.0f;
	else
		tmpColor = tmpColor + (1 - ( t - int(t) ) ) / 20.0f;

	outputColor += tmpColor;
	}

	// 2nd point light scope
	{
	tmpColor = PointLight(point2, material);
	if ( ( t-int(t) ) < 0.5f )
		tmpColor = tmpColor + ( t - int(t) ) / 20.0f;
	else
		tmpColor = tmpColor + (1 - ( t - int(t) ) ) / 20.0f;

	outputColor += tmpColor;
	}
	
	// reflector light	
	if ( reflectOn )
		outputColor += ReflectorLight(reflector, material);

	// use texture
    if ( material.useTexture )
	{
		color_f = outputColor * texture ( texSampler, texCoord_v );
		
		if ( fogOn )
			color_f = addFog ( color_f );
	}
	else
		color_f = addFog ( outputColor );
 
}