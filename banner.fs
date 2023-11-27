#version 140

uniform sampler2D texSampler;  // sampler for texture access

smooth in vec2 texCoord_v;     // incoming fragment texture coordinates
out vec4 color_f;              // outgoing fragment color

void main() 
{
  // fragment color is given only by the texture
  color_f = texture(texSampler, texCoord_v);
}