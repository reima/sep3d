//--------------------------------------------------------------------------------------
// File: TerrainRenderer.fx
//--------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
cbuffer cb0
{
  float    g_fTime;                   // App's time in seconds
  float4   g_vCamPos;                 // Camera position
  float4x4 g_mWorld;                  // World matrix for object
  float4x4 g_mWorldViewProjection;    // World * View * Projection matrix
}

#define NUM_SPOTS 8
const int g_nSpots = NUM_SPOTS;
const float g_Spots[NUM_SPOTS] = {
  -1.0,      // Tiefes Wasser
  -0.25,     // Seichtes Wasser
   0.0,      // Küste
   0.0625,   // Strand
   0.125,    // Gras
   0.375,    // Wald
   0.75,     // Gestein
   1.0       // Schnee
};

const float4 g_Colors[NUM_SPOTS] = {
  float4(0, 0, 0.625, 1),
  float4(0, 0.25, 1, 1),
  float4(0, 0.5, 1, 1),
  float4(0.9375, 0.9375, 0.25, 1),
  float4(0.125, 0.625, 0, 1),
  float4(0, 0.375, 0, 1),
  float4(0.5, 0.5, 0.5, 1),
  float4(1, 1, 1, 1)
};

//--------------------------------------------------------------------------------------
// Vertex shader input structures
//--------------------------------------------------------------------------------------
struct VS_INPUT
{
  float3 Position   : POSITION;   // vertex position
  float3 Normal     : NORMAL;     // vertex normal
};

//--------------------------------------------------------------------------------------
// Vertex shader output structures
//--------------------------------------------------------------------------------------
// VSC (Vertex Shader Coloring)
struct VS_VSC_OUTPUT
{
  float4 Position   : SV_Position;   // vertex position
  float4 Color      : COLOR;
};

// PSC (Pixel Shader Coloring)
struct VS_PSC_OUTPUT
{
  float4 Position   : SV_Position;   // vertex position
  float  Height     : HEIGHT;
};

//--------------------------------------------------------------------------------------
// Global functions
//--------------------------------------------------------------------------------------
float4 GetColorFromHeight(float height)
{
  height = clamp(height, g_Spots[0], g_Spots[g_nSpots - 1]);
  float4 color = float4(0, 0, 0, 1);
  for (int i = 0; i < g_nSpots - 1; ++i) {
    if (height <= g_Spots[i+1]) {
      color = lerp(g_Colors[i], g_Colors[i+1],
                   (height - g_Spots[i]) / (g_Spots[i+1] - g_Spots[i]));
      break;
    }
  }
  return color;
}

//--------------------------------------------------------------------------------------
// Vertex Shaders
//--------------------------------------------------------------------------------------
VS_VSC_OUTPUT RenderScene_VSC_VS( VS_INPUT In )
{
  VS_VSC_OUTPUT Output;
  float4 pos = float4(In.Position, 1.0f);
  Output.Color = GetColorFromHeight(pos.y);
 
  // Diffuse Phong
  float4 L = normalize(g_vCamPos - pos);
  Output.Color *= dot(L, normalize(In.Normal));

  // Create water plane with waves
/*  if (pos.y < 0) {
    pos.y = 0.05 * sin(3 * pos.x + g_fTime * 2) * cos(3 * pos.z + g_fTime * 3);
  }*/

  // Transform the position from object space to homogeneous projection space
  Output.Position = mul(pos, g_mWorldViewProjection);

  return Output;
}

VS_PSC_OUTPUT RenderScene_PSC_VS( VS_INPUT In )
{
  VS_PSC_OUTPUT Output;
  float4 pos = float4(In.Position, 1.0f);
  Output.Height = pos.y;
  // Transform the position from object space to homogeneous projection space
  Output.Position = mul(pos, g_mWorldViewProjection);

  return Output;
}

VS_VSC_OUTPUT RenderScene_NC_VS( VS_INPUT In )
{
  VS_VSC_OUTPUT Output;
  Output.Color = float4(normalize(In.Normal), 0);
  float4 pos = float4(In.Position, 1.0f);
  // Transform the position from object space to homogeneous projection space
  Output.Position = mul(pos, g_mWorldViewProjection);

  return Output;
}

//--------------------------------------------------------------------------------------
// Pixel Shaders
//--------------------------------------------------------------------------------------
float4 RenderScene_VSC_PS( VS_VSC_OUTPUT In ) : SV_Target
{ 
  return In.Color;
}

float4 RenderScene_PSC_PS( VS_PSC_OUTPUT In ) : SV_Target
{
  return GetColorFromHeight(In.Height);
}

//--------------------------------------------------------------------------------------
// Renders scene 
//--------------------------------------------------------------------------------------
technique10 VertexShaderColoring
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, RenderScene_VSC_VS() ) );
    SetGeometryShader( NULL );
    SetPixelShader( CompileShader( ps_4_0, RenderScene_VSC_PS() ) );
  }
}

technique10 PixelShaderColoring
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, RenderScene_PSC_VS() ) );
    SetGeometryShader( NULL );
    SetPixelShader( CompileShader( ps_4_0, RenderScene_PSC_PS() ) );
  }
}

technique10 NormalColoring
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, RenderScene_NC_VS() ) );
    SetGeometryShader( NULL );
    SetPixelShader( CompileShader( ps_4_0, RenderScene_VSC_PS() ) );
  }
}
