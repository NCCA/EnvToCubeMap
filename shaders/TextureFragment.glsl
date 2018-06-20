#version 330 core
/// @brief our output fragment colour
layout (location =0) out vec4 fragColour;
// texture sampler
uniform samplerCube tex;
// uv from input
in vec2 vertUV;
uniform int face;
// https://stackoverflow.com/questions/38543155/opengl-render-face-of-cube-map-to-a-quad

void main ()

{
  vec2 mapCoord = 2.0 * vertUV - 1.0;
  if (face==0)     fragColour=texture(tex,vec3(1.0, mapCoord.xy));
  else if(face==1) fragColour=texture(tex, vec3(-1.0, mapCoord.xy));
  else if(face==2) fragColour=texture(tex, vec3(mapCoord.x, 1.0, mapCoord.y));
  else if(face==3) fragColour=texture(tex, vec3(mapCoord.x, -1.0, mapCoord.y));
  else if(face==4) fragColour=texture(tex, vec3(mapCoord.xy, 1.0));
  else if(face==5) fragColour=texture(tex, vec3(mapCoord.xy, -1.0));

  //fragColour.rgb=vec3(1.0, vertUV);
}

