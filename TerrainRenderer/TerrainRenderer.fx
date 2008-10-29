//--------------------------------------------------------------------------------------
// File: SimpleSample.fx
//
// The effect file for the SimpleSample sample.  
// 
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
cbuffer cb0
{
  float    g_fTime;                   // App's time in seconds
  float4x4 g_mWorld;                  // World matrix for object
  float4x4 g_mWorldViewProjection;    // World * View * Projection matrix
}

//--------------------------------------------------------------------------------------
// Vertex shader input structure
//--------------------------------------------------------------------------------------
struct VS_INPUT
{
  float3 Position   : POSITION;   // vertex position
};

//--------------------------------------------------------------------------------------
// Vertex shader output structure
//--------------------------------------------------------------------------------------
struct VS_OUTPUT
{
  float4 Position   : SV_Position;   // vertex position
  float4 Color      : COLOR;
};


//--------------------------------------------------------------------------------------
// This shader computes standard transform and lighting
//--------------------------------------------------------------------------------------
VS_OUTPUT RenderSceneVS( VS_INPUT In)
{
  VS_OUTPUT Output;

  float4 pos = float4(In.Position, 1.0f);

  // encode color depending on y-coordinate (approx. the height)
  float spots[] = {
    -1.0,      // Tiefes Wasser
    -0.25,     // Seichtes Wasser
     0.0,      // Küste
     0.0625,   // Strand
     0.125,    // Gras
     0.375,    // Wald
     0.75,     // Gestein
     1.0       // Schnee
  };

  float4 colors[] = {
    float4(0, 0, 0.625, 1),
    float4(0, 0.25, 1, 1),
    float4(0, 0.5, 1, 1),
    float4(0.9375, 0.9375, 0.25, 1),
    float4(0.125, 0.625, 0, 1),
    float4(0, 0.375, 0, 1),
    float4(0.5, 0.5, 0.5, 1),
    float4(1, 1, 1, 1)
  };

  float v = In.Position.y;
  if (v < spots[0]) Output.Color = colors[0];
  else if (v > spots[7]) Output.Color = colors[7];
  else for (int i = 0; i < 7; ++i) {
    if (v < spots[i+1]) {
      Output.Color = lerp(colors[i], colors[i+1],
                          (v - spots[i]) / (spots[i+1] - spots[i]));
      break;
    }
  }
  
  // Create water plane
  if (pos.y < 0) {
    pos.y = 0.025 * sin(5 * pos.x + g_fTime * 2) * cos(5 * pos.z + g_fTime * 2);
  }

  // Transform the position from object space to homogeneous projection space
  Output.Position = mul(pos, g_mWorldViewProjection);

  return Output;
}

//--------------------------------------------------------------------------------------
// This shader outputs the pixel's color by modulating the texture's
// color with diffuse material color
//--------------------------------------------------------------------------------------
float4 RenderScenePS( VS_OUTPUT In ) : SV_Target
{ 
  return In.Color;
}


//--------------------------------------------------------------------------------------
// Renders scene 
//--------------------------------------------------------------------------------------
technique10 RenderScene
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, RenderSceneVS() ) );
    SetGeometryShader( NULL );
    SetPixelShader( CompileShader( ps_4_0, RenderScenePS() ) );
  }
}
