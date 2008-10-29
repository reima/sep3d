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

  // encode color depending on z-coordinate (approx. the height)
  float maxZ = 4.0;
  float minZ = 0.0;
  float c = (In.Position.z - minZ) / maxZ;
  Output.Color = float4(c, c, c, 1);

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
