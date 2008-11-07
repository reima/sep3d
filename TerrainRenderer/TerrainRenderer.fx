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
  float3   g_vCamPos;                 // Camera position
  float3   g_vLightPos;               // Light position
  float    g_fWaveHeight;
  float2   g_vWaveScale;
  float2   g_vWaveSpeed;
}

Texture2D g_tWaves;
SamplerState g_ssWaves
{
	Filter = MIN_MAG_MIP_POINT;
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
  float4(0.0, 0.0, 0.0, 1),
  float4(0.125, 0.125, 0.05, 1),
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

struct VS_SFX_OUTPUT
{
  float4 Position   : SV_Position;
  float4 Normal     : TEXCOORD0;
  float  Height     : TEXCOORD1;
  float4 ViewDir    : TEXCOORD2;
  float4 LightDir   : TEXCOORD3;
  float2 WaveCoords : TEXCOORD4;
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
    float4 L = normalize(float4(g_vCamPos, 1) - pos);
    Output.Color *= saturate(dot(L, normalize(In.Normal)));
  }

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

VS_SFX_OUTPUT SFX_VS( VS_INPUT In, uniform bool waves )
{
  VS_SFX_OUTPUT Output;
  float4 pos = float4(In.Position, 1);
  if (waves) {
    float2 foo = g_vWaveScale * pos.xz + g_vWaveSpeed * g_fTime;
    // Wellen erzeugen
    pos.y = g_fWaveHeight * sin(foo.x) * cos(foo.y);
    // Normale der Tangentialebene am Punkt pos
    Output.Normal = float4(
      cross(
        float3(0, -g_vWaveScale.y*g_fWaveHeight*sin(foo.x)*sin(foo.y), 1),
        float3(1,  g_vWaveScale.x*g_fWaveHeight*cos(foo.x)*cos(foo.y), 0)),
      0);
  } else {
    Output.Normal = float4(In.Normal, 0);
  }
  Output.Position = mul(pos, g_mWorldViewProjection);
  Output.Height = In.Position.y;
  Output.ViewDir = float4(g_vCamPos, 1) - pos;
  Output.LightDir = float4(g_vLightPos, 1) - pos;
  Output.WaveCoords = pos.xz;// + 0.1 * g_fTime;
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

float4 SFX_PS( VS_SFX_OUTPUT In, uniform bool waves ) : SV_Target
{
  float4 L = normalize(In.LightDir);
  if (waves) {
    if (In.Height > 0) discard;
    float3 base_color = float3(0, 0.5, 1);
    float4 normal_offset = float4(
        g_tWaves.Sample(g_ssWaves, In.WaveCoords).rgb, 0);
    float4 N = normalize(In.Normal + normal_offset);
    float NdotL = saturate(dot(N, L));
    float4 V = normalize(In.ViewDir);
    float4 R = normalize(reflect(-L, N));
    float RdotV = saturate(dot(R, V));
    float fresnel = 0.5*saturate(1-pow(dot(N, V), 3))+0.1;
    return float4(NdotL * base_color + pow(RdotV, 100), fresnel);
  } else {
    float4 N = normalize(In.Normal);
    float NdotL = saturate(dot(N, L));
    return NdotL * GetColorFromHeight(In.Height);
  }
}

BlendState SrcColorBlendingAdd
{
  BlendEnable[0] = TRUE;
  BlendEnable[1] = TRUE;
  SrcBlend = SRC_ALPHA;
  DestBlend = INV_SRC_ALPHA;
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

technique10 SpecialFX
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, SFX_VS(false) ) );
    SetGeometryShader( NULL );
    SetPixelShader( CompileShader( ps_4_0, SFX_PS(false) ) );
  }
  pass P1
  {
    SetVertexShader( CompileShader( vs_4_0, SFX_VS(true) ) );
    SetGeometryShader( NULL );
    SetPixelShader( CompileShader( ps_4_0, SFX_PS(true) ) );
    SetBlendState( SrcColorBlendingAdd, float4(0,0,0,0), 0xffffffff );
  }
}