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

cbuffer cb1
{
  float4   g_vCamPos;                 // Camera position
  float4   g_vLightPos;               // Light position
}

Texture2D g_tWaves;
SamplerState g_ssWaves
{
	Filter = MIN_MAG_MIP_LINEAR;
};

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
  float4(0.125, 0.125, 0.025, 1),
  float4(0.25, 0.25, 0.05, 1),
  float4(0.5, 0.5, 0.1, 1),
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

struct VS_PHONG_OUTPUT
{
  float4 Position   : SV_Position;
  float2 WaveCoords : TEXCOORD0;
  float4 Normal     : NORMAL;
  float  Height     : HEIGHT;
  float4 LightDir   : LIGHTDIR;
  float4 ViewDir    : VIEWDIR;
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
VS_VSC_OUTPUT VertexColoring_VS( VS_INPUT In, uniform bool phong )
{
  VS_VSC_OUTPUT Output;
  float4 pos = float4(In.Position, 1.0f);
  Output.Color = GetColorFromHeight(pos.y);
 
  // Diffuse Phong
  if (phong) {
    float4 L = normalize(g_vCamPos - pos);
    Output.Color *= dot(L, normalize(In.Normal));
  }

  // Create water plane with waves
/*  if (pos.y < 0) {
    pos.y = 0.05 * sin(3 * pos.x + g_fTime * 2) * cos(3 * pos.z + g_fTime * 3);
  }*/

  // Transform the position from object space to homogeneous projection space
  Output.Position = mul(pos, g_mWorldViewProjection);

  return Output;
}

VS_PSC_OUTPUT PixelColoring_VS( VS_INPUT In )
{
  VS_PSC_OUTPUT Output;
  float4 pos = float4(In.Position, 1.0f);
  Output.Height = pos.y;
  // Transform the position from object space to homogeneous projection space
  Output.Position = mul(pos, g_mWorldViewProjection);

  return Output;
}

VS_VSC_OUTPUT NormalColoring_VS( VS_INPUT In )
{
  VS_VSC_OUTPUT Output;
  Output.Color = float4(normalize(In.Normal), 0);
  float4 pos = float4(In.Position, 1.0f);
  // Transform the position from object space to homogeneous projection space
  Output.Position = mul(pos, g_mWorldViewProjection);

  return Output;
}

VS_PHONG_OUTPUT Phong_VS( VS_INPUT In, uniform bool waves )
{
  VS_PHONG_OUTPUT Output;
  float4 pos = float4(In.Position, 1.0f);
  Output.Height = pos.y;
  Output.ViewDir = g_vCamPos - pos;
  Output.LightDir = float4(5*sin(0.5*g_fTime), 5, 5*cos(0.5*g_fTime), 1.0f) - pos;
//  Output.LightDir = float4(5, 5, 5, 1) - pos;
//  Output.LightDir = g_vLightPos - pos;
  Output.Normal = float4(In.Normal, 0.0f);
  if (waves && pos.y < 0) {
    float a = 0.05;
    float sx = 3;
    float sz = 5;
    float x_ = sx*pos.x+2*g_fTime;
    float z_ = sz*pos.z+3*g_fTime;
    pos.y = a * sin(x_) * cos(z_);
    Output.Normal = float4(cross(float3(0,-sz*a*sin(x_)*sin(z_), 1),
                                 float3(1, sx*a*cos(x_)*cos(z_), 0)), 0);    
  }
  Output.Position = mul(pos, g_mWorldViewProjection);
  Output.WaveCoords = pos.xz + 0.1*float2(g_fTime, g_fTime);

  return Output;
}

//--------------------------------------------------------------------------------------
// Pixel Shaders
//--------------------------------------------------------------------------------------
float4 VertexColoring_PS( VS_VSC_OUTPUT In ) : SV_Target
{ 
  return In.Color;
}

float4 PixelColoring_PS( VS_PSC_OUTPUT In ) : SV_Target
{
  return GetColorFromHeight(In.Height);
}

float4 Phong_PS( VS_PHONG_OUTPUT In, uniform bool waves ) : SV_Target
{
  if (waves && In.Height >= 0) discard;
  float4 color = GetColorFromHeight(In.Height);
  float4 L = normalize(In.LightDir);
  float4 normal_offset = float4(0, 0, 0, 0);
  if (In.Height < 0 && waves) {
    normal_offset += g_tWaves.Sample(g_ssWaves, In.WaveCoords).xzyw;
    color = float4(0, 0.5, 1, 1);
  }  
  float4 N = normalize(normalize(In.Normal) + normal_offset);
  float4 V = normalize(In.ViewDir);
  float4 H = normalize(L + V);
  float4 diffuse = saturate(dot(N, L)) * color;
  float4 specular = float4(0, 0, 0, 0);
  if (In.Height < 0 && waves) {
    specular = pow(saturate(dot(N, H)), 20) * float4(1,1,1,1);
  }

  return float4((diffuse + specular).rgb, 1);
}

BlendState SrcColorBlendingAdd
{
  BlendEnable[0] = TRUE;
  BlendEnable[1] = TRUE;
  SrcBlend = SRC_COLOR;
  DestBlend = DEST_COLOR;
  BlendOp = ADD;
  SrcBlendAlpha = ZERO;
  DestBlendAlpha = ZERO;
  BlendOpAlpha = ADD;
  RenderTargetWriteMask[0] = 0x0F;
};

//--------------------------------------------------------------------------------------
// Renders scene 
//--------------------------------------------------------------------------------------
technique10 VertexShaderColoring
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, VertexColoring_VS(false) ) );
    SetGeometryShader( NULL );
    SetPixelShader( CompileShader( ps_4_0, VertexColoring_PS() ) );
  }
}

technique10 VertexShaderColoringPhong
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, VertexColoring_VS(true) ) );
    SetGeometryShader( NULL );
    SetPixelShader( CompileShader( ps_4_0, VertexColoring_PS() ) );
  }
}

technique10 PixelShaderColoring
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, PixelColoring_VS() ) );
    SetGeometryShader( NULL );
    SetPixelShader( CompileShader( ps_4_0, PixelColoring_PS() ) );
  }
}

technique10 NormalColoring
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, NormalColoring_VS() ) );
    SetGeometryShader( NULL );
    SetPixelShader( CompileShader( ps_4_0, VertexColoring_PS() ) );
  }
}

technique10 Phong
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, Phong_VS(false) ) );
    SetGeometryShader( NULL );
    SetPixelShader( CompileShader( ps_4_0, Phong_PS(false) ) );
  }
  pass P1
  {
    SetVertexShader( CompileShader( vs_4_0, Phong_VS(true) ) );
    SetGeometryShader( NULL );
    SetPixelShader( CompileShader( ps_4_0, Phong_PS(true) ) );
    SetBlendState( SrcColorBlendingAdd, float4(0,0,0,0), 0xffffffff );    
  }
}