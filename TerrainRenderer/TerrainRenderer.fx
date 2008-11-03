//--------------------------------------------------------------------------------------
// File: TerrainRenderer.fx
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

// Uncomment following line for additional fun!
//  pos.y *= sin(g_fTime);

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

  float h = clamp(pos.y, -1, 1);
  for (int i = 0; i < 7; ++i) {
    if (h <= spots[i+1]) {
      Output.Color = lerp(colors[i], colors[i+1],
                          (h - spots[i]) / (spots[i+1] - spots[i]));
      break;
    }
  }
  
  // Create water plane with waves
  if (pos.y < 0) {
    pos.y = 0.05 * sin(3 * pos.x + g_fTime * 2) * cos(3 * pos.z + g_fTime * 3);
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
