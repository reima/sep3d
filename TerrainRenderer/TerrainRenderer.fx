//--------------------------------------------------------------------------------------
// File: TerrainRenderer.fx
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
cbuffer cbPerTilePass
{
  float    g_fTileScale;
  float2   g_vTileTranslate;
  uint     g_uiTileLOD;
}

cbuffer cbPerFrame
{
  // Misc
  float    g_fTime;                   // App's time in seconds
  float4x4 g_mWorld;                  // World matrix for object
  float4x4 g_mWorldViewProjection;    // World * View * Projection matrix
  float3   g_vCamPos;                 // Camera position
  // Lights
  float3   g_vPointLight_Position[8];
  float3   g_vDirectionalLight_Direction[8]; // Guaranteed to be normalized
  float3   g_vSpotLight_Position[8];
  float4x4 g_mDirectionalLightSpaceTransform;
  float4x4 g_mPointLightSpaceTransform[6];
  // Environment
  float4x4 g_mWorldViewInv;
  float    g_fCameraFOV;  
}

cbuffer cbOptions
{
  bool     g_bDynamicMinMax;
  bool     g_bWaveNormals;
  bool     g_bPCF;
  float    g_fZEpsilon;
}

cbuffer cbStatic
{
  // Terrain
  float  g_fMinHeight;
  float  g_fMaxHeight;
  uint   g_uiTerrainSize;
  // Lights
  uint   g_nPointLights;
  uint   g_nDirectionalLights;
  uint   g_nSpotLights;
  uint   g_iShadowedPointLight;
  uint   g_iShadowedDirectionalLight;
  bool   g_bShadowedPointLight;
  bool   g_bShadowedDirectionalLight;
  float3 g_vPointLight_Color[8];
  float3 g_vDirectionalLight_Color[8];
  float3 g_vSpotLight_Color[8];
  float2 g_fSpotLight_AngleExp[8];
  float3 g_vSpotLight_Direction[8]; // Guaranteed to be normalized
  float4 g_vMaterialParameters; // = { k_a, k_d, k_s, n }
  // Misc
  uint2  g_vBackBufferSize;
}

Texture2D g_tWaves;
Texture2D g_tGround;
Texture2D g_tTerrain; // Terrain heights
Texture3D g_tGround3D;
Texture2D g_tMesh; // Tree texture
TextureCube g_tCubeMap;
Texture2D g_tDirectionalShadowMap;
Texture2DArray g_tPointShadowMap;

SamplerState g_ssLinear
{
  Filter = MIN_MAG_MIP_LINEAR;
  AddressU = Wrap;
  AddressV = Wrap;
  AddressW = Clamp;
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
  float4(0.0625, 0.0625, 0.025, 1),
  float4(0.125, 0.125, 0.05, 1),
  float4(0.5, 0.5, 0.1, 1),
  float4(0.9375, 0.9375, 0.25, 1),
  float4(0.125, 0.625, 0, 1),
  float4(0, 0.375, 0, 1),
  float4(0.5, 0.5, 0.5, 1),
  float4(1, 1, 1, 1)
};

const float4 g_LODColors[] = {
  float4(1, 0, 0, 1),
  float4(0, 1, 0, 1),
  float4(0, 0, 1, 1),
  float4(1, 1, 0, 1),
  float4(0, 1, 1, 1),
  float4(1, 0, 1, 1),
};

const float3 vAttenuation = float3(0, 0, 1); // Constant, linear, quadratic

const float4 g_vWaterColor = float4(0, 0.25, 0.5, 1.0);
const float4 g_vWaterMaterial = float4(0.05, 0.7, 0.25, 200);
const float4 g_vTreeMaterial = float4(0.05, 0.8, 0.15, 10);

//--------------------------------------------------------------------------------------
// Vertex shader input structures
//--------------------------------------------------------------------------------------
struct VS_INPUT
{
  float3 Position   : POSITION;   // vertex position
};

struct VS_TREES_INPUT
{
  float3 Position : POSITION;
  float3 Normal : NORMAL;
  float2 TexCoord : TEXTURE; 
  row_major float4x4 mTransform: mTransform;
};

//--------------------------------------------------------------------------------------
// Vertex shader output structures
//--------------------------------------------------------------------------------------
struct VS_GOURAUD_SHADING_OUTPUT
{
  float4 Position       : SV_Position;
  float  Height         : HEIGHT;
  float3 DiffuseLight   : DIFFUSELIGHT;
  float3 SpecularLight  : SPECULARLIGHT;
};

struct VS_PHONG_SHADING_OUTPUT
{
  float4 Position       : SV_Position;
  float3 WorldPosition  : WORLDPOS;
  float4 LightSpacePos  : LIGHTSPACEPOS;
  float  Height         : HEIGHT;
  float3 Normal         : NORMAL;
  float2 TexCoord       : TEXCOORD0;
  float2 Wave0          : TEXCOORD2;
  float2 Wave1          : TEXCOORD3;
  float2 Wave2          : TEXCOORD4;
  float2 Wave3          : TEXCOORD5;
  float4 Material       : MATERIAL; // = { k_a, k_d, k_s, n }
};

struct VS_TREES_OUTPUT
{
  float4 Position      : SV_Position;
  float3 WorldPosition : WORLDPOS;
  float4 LightSpacePos : LIGHTSPACEPOS;
  float3 Normal        : NORMAL;
  float2 TexCoord      : TEXCOORD0;
};

struct GS_POINTSHADOW_INPUT
{
  float4 Position   : SV_Position;
};

struct GS_POINTSHADOW_OUTPUT
{
  float4 Position   : SV_Position;
  float2 Depth      : DEPTH;
  uint   RTI        : SV_RenderTargetArrayIndex;
};

struct PHONG
{
  float3 DiffuseLight;
  float3 SpecularLight;
};

//--------------------------------------------------------------------------------------
// Global functions
//--------------------------------------------------------------------------------------
float4 GetColorFromHeight(float height)
{
  if (g_bDynamicMinMax) {
    height = (height - g_fMinHeight) / (g_fMaxHeight - g_fMinHeight) *
      (g_Spots[g_nSpots - 1] - g_Spots[0]) + g_Spots[0];
  } else {
    height = clamp(height, g_Spots[0], g_Spots[g_nSpots - 1]);
  }
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

float2 Phong(float3 vPos, float3 vLightDir, float3 vNormal, float4 vMaterial)
{
  // Diffuse
  float3 L = normalize(vLightDir);
  float3 N = normalize(vNormal);
  float NdotL = saturate(dot(N, L));
  float fDiffuse = NdotL * vMaterial.y;

  // Specular
  float3 R = normalize(reflect(-L, N));
  float3 V = normalize(g_vCamPos - vPos);
  float RdotV = saturate(dot(R, V));
  float fSpecular = pow(RdotV, vMaterial.w) * vMaterial.z;

  return float2(fDiffuse, fSpecular);
}

PHONG PhongLighting(float3 vPos, float3 vNormal, float4 vMaterial)
{
  float3 vDiffuseLight = float3(0, 0, 0);
  float3 vSpecularLight = float3(0, 0, 0);
  // Point lights
  uint i;
  float d;
  float fAttenuation;
  float2 vPhong;
  for (i = 0; i < g_nPointLights; i++) {
    // Defer shadowed lighting
    if (g_bShadowedPointLight && i == g_iShadowedPointLight) continue;
    d = length(vPos - g_vPointLight_Position[i]);
    fAttenuation = 1 / dot(vAttenuation, float3(1, d, d*d));
    vPhong = Phong(vPos, g_vPointLight_Position[i] - vPos, vNormal, vMaterial);
    vDiffuseLight += vPhong.x * g_vPointLight_Color[i] * fAttenuation;
    vSpecularLight += vPhong.y * g_vPointLight_Color[i] * fAttenuation;
  }

  // Directional lights
  for (i = 0; i < g_nDirectionalLights; i++) {
    // Defer shadowed lighting
    if (g_bShadowedDirectionalLight &&i == g_iShadowedDirectionalLight) continue;
    vPhong = Phong(vPos, g_vDirectionalLight_Direction[i], vNormal,
                          vMaterial);
    vDiffuseLight += vPhong.x * g_vDirectionalLight_Color[i];
    vSpecularLight += vPhong.y * g_vDirectionalLight_Color[i];
  }

  // Spot lights
  float3 I;
  float IdotL, theta, angle;
  for (i = 0; i < g_nSpotLights; i++) {
    d = length(vPos - g_vSpotLight_Position[i]);
    I = (g_vSpotLight_Position[i] - vPos) / d;
    IdotL = dot(-I, g_vSpotLight_Direction[i]);
    theta = abs(g_fSpotLight_AngleExp[i].x);
    angle = acos(IdotL);
    if (angle < theta) {
      fAttenuation = 1 / dot(vAttenuation, float3(1, d, d*d));
      fAttenuation *= 1 - pow(angle/theta, g_fSpotLight_AngleExp[i].y);
      vPhong = Phong(vPos, g_vSpotLight_Position[i] - vPos, vNormal, vMaterial);
      vDiffuseLight += vPhong.x * g_vSpotLight_Color[i] * fAttenuation;
      vSpecularLight += vPhong.y * g_vSpotLight_Color[i] * fAttenuation; 
    }
  }

  PHONG ret;
  ret.DiffuseLight = vDiffuseLight;
  ret.SpecularLight = vSpecularLight;
  return ret;
}

float3 FullLighting(float3 vColor,
                    float3 vWorldPosition,
                    float4 vLightSpacePos,
                    float3 N,
                    float4 vMaterial)
{
  PHONG phong = PhongLighting(vWorldPosition, N, vMaterial);

  float fShadowMap;
  uint nInShadow;
  float fLightDist;
  float2 vPhong;
  float fLightScale;
  uint uiWidth, uiHeight, uiElements;

  //
  // Shadowed directional light
  //
  if (g_bShadowedDirectionalLight) {
    vLightSpacePos /= vLightSpacePos.w;
    fLightDist = vLightSpacePos.z - g_fZEpsilon;
    vLightSpacePos.x = 0.5 * vLightSpacePos.x + 0.5;
    vLightSpacePos.y = 1 - (0.5 * vLightSpacePos.y + 0.5);
    g_tDirectionalShadowMap.GetDimensions(uiWidth, uiHeight);
    vLightSpacePos.xy *= uint2(uiWidth-1, uiHeight-1);

    if (g_bPCF) {
      nInShadow = 0;
      [unroll] for (int x = -1; x <= 1; ++x) {
        [unroll] for (int y = -1; y <= 1; ++y) {
          fShadowMap = g_tDirectionalShadowMap.Load(int3(vLightSpacePos.xy + int2(x, y), 0));
          if (fShadowMap < fLightDist) nInShadow++;
        }
      }
      fLightScale = 1 - nInShadow / 9.0f;
    } else {
      fShadowMap = g_tDirectionalShadowMap.Load(int3(vLightSpacePos.xy, 0));
      if (fShadowMap < fLightDist) fLightScale = 0.0f;
      else fLightScale = 1.0f;
    }

    if (fLightScale > 0) {
      uint i = g_iShadowedDirectionalLight;
      vPhong = Phong(vWorldPosition,
                     g_vDirectionalLight_Direction[i],
                     N, vMaterial);      
      phong.DiffuseLight +=
          fLightScale * vPhong.x * g_vDirectionalLight_Color[i];
      phong.SpecularLight +=
          fLightScale * vPhong.y * g_vDirectionalLight_Color[i];
    }
  }

  //
  // Shadowed point light
  //
  if (g_bShadowedPointLight) {
    const uint i = g_iShadowedPointLight;
    float3 vLightDir = vWorldPosition - g_vPointLight_Position[i];
    float3 vLightDirAbs = abs(vLightDir);
    float fMaxCoord = max(vLightDirAbs.x, max(vLightDirAbs.y, vLightDirAbs.z));
    uint uiArraySlice;
    if (vLightDirAbs.x == fMaxCoord) {
      if (vLightDir.x < 0) uiArraySlice = 0;
      else uiArraySlice = 1;
    } else if (vLightDirAbs.y == fMaxCoord) {
      if (vLightDir.y < 0) uiArraySlice = 2;
      else uiArraySlice = 3;
    } else if (vLightDirAbs.z == fMaxCoord) {
      if (vLightDir.z < 0) uiArraySlice = 4;
      else uiArraySlice = 5;
    }
    // Transform in light space
    vLightSpacePos = mul(float4(vWorldPosition, 1), g_mPointLightSpaceTransform[uiArraySlice]);
    // Dehomogenize
    vLightSpacePos /= vLightSpacePos.w;
    fLightDist = vLightSpacePos.z - g_fZEpsilon;
    vLightSpacePos.x = 0.5 * vLightSpacePos.x + 0.5;
    vLightSpacePos.y = 1 - (0.5 * vLightSpacePos.y + 0.5);
    g_tPointShadowMap.GetDimensions(uiWidth, uiHeight, uiElements);
    vLightSpacePos.xy *= uint2(uiWidth-1, uiHeight-1);

    if (g_bPCF) {
      nInShadow = 0;
      [unroll] for (int x = -1; x <= 1; ++x) {
        [unroll] for (int y = -1; y <= 1; ++y) {
          fShadowMap = g_tPointShadowMap.Load(int4(vLightSpacePos.xy + int2(x, y), uiArraySlice, 0));
          if (fShadowMap < fLightDist) nInShadow++;
        }
      }
      fLightScale = 1 - nInShadow / 9.0f;
    } else {
      fShadowMap = g_tPointShadowMap.Load(int4(vLightSpacePos.xy, uiArraySlice, 0));
      if (fShadowMap < fLightDist) fLightScale = 0.0f;
      else fLightScale = 1.0f;
    }

    if (fLightScale > 0) {
      float d = length(vWorldPosition - g_vPointLight_Position[i]);
      float fAttenuation = 1 / dot(vAttenuation, float3(1, d, d*d));
      vPhong = Phong(vWorldPosition,
                     g_vPointLight_Position[i] - vWorldPosition,
                     N,
                     vMaterial);
      phong.DiffuseLight +=
          fLightScale * vPhong.x * g_vPointLight_Color[i] * fAttenuation;
      phong.SpecularLight +=
          fLightScale * vPhong.y * g_vPointLight_Color[i] * fAttenuation;
    }
  }

  return vColor * vMaterial.x +
      vColor * phong.DiffuseLight +
      phong.SpecularLight;
}

float4 GetTileData(float2 vPosition)
{
  return g_tTerrain.Load(float3(vPosition.xy, 0) * (g_uiTerrainSize - 1));
}

float4 GetTileVertex(float2 vPosition)
{
  float4 vVertexData = GetTileData(vPosition);
  vPosition *= g_fTileScale;
  vPosition += g_vTileTranslate;
  return float4(vPosition.x, vVertexData.w, vPosition.y, 1);
}

float4 GetTileVertexAndNormal(float2 vPosition, out float4 vNormal)
{
  float4 vVertexData = GetTileData(vPosition);
  vNormal = float4(vVertexData.xyz, 0);
  vPosition *= g_fTileScale;
  vPosition += g_vTileTranslate;
  return float4(vPosition.x, vVertexData.w, vPosition.y, 1);  
}

//--------------------------------------------------------------------------------------
// Vertex Shaders
//--------------------------------------------------------------------------------------
VS_GOURAUD_SHADING_OUTPUT GouraudShading_VS( float2 vPosition : POSITION )
{
  VS_GOURAUD_SHADING_OUTPUT Output;
  float4 vNormal;
  float4 vPos = GetTileVertexAndNormal(vPosition, vNormal);
  Output.Height = vPos.y;  
  // Transform the position from object space to homogeneous projection space
  Output.Position = mul(vPos, g_mWorldViewProjection);
  PHONG phong = PhongLighting(vPos, vNormal,
                              g_vMaterialParameters);
  Output.DiffuseLight = phong.DiffuseLight;
  Output.SpecularLight = phong.SpecularLight;

  return Output;
}

VS_PHONG_SHADING_OUTPUT PhongShading_VS( float2 vPosition : POSITION )
{
  VS_PHONG_SHADING_OUTPUT Output;
  float4 vPos = GetTileVertexAndNormal(vPosition, Output.Normal);
  if (vPos.y <= 0) {
    Output.Material = g_vWaterMaterial;
  } else {
    Output.Material = g_vMaterialParameters;
  }
  // Transform the position from object space to homogeneous projection space
  Output.Position = mul(vPos, g_mWorldViewProjection);
  Output.Height = vPos.y;
  Output.WorldPosition = vPos;
  Output.LightSpacePos = mul(vPos, g_mDirectionalLightSpaceTransform);
  Output.TexCoord = vPos.xz * 5;

  if (g_bWaveNormals) {
    float2 vTexCoord = vPos.xz * 0.5;
    float2 vTrans = g_fTime * 0.01;
    Output.Wave0 = vTexCoord * 1 + vTrans * 2;
    Output.Wave1 = vTexCoord * 2 - vTrans * 4;
    Output.Wave2 = vTexCoord * 4 + vTrans * 2;
    Output.Wave3 = vTexCoord * 8 - vTrans * 1;
  }

  return Output;
}

VS_TREES_OUTPUT Trees_VS( VS_TREES_INPUT Input )
{
VS_TREES_OUTPUT Output;

  float4 vPos = float4(Input.Position, 1);
  float4x4 trans = Input.mTransform;
  trans._14 = sin(g_fTime)/30;
  vPos = mul(vPos, trans);
  Output.Position = mul(vPos, g_mWorldViewProjection);
  Output.WorldPosition = Input.Position;
  Output.LightSpacePos = mul(vPos, g_mDirectionalLightSpaceTransform);
  Output.Normal = normalize(Input.Normal); // korrekt?
  Output.TexCoord = Input.TexCoord;
 
  return Output; 
}

float4 Environment_VS( float3 vPosition : POSITION ) : SV_Position
{
  return float4(vPosition, 1);
}

float4 DirectionalShadow_VS( float2 vPosition : POSITION ) : SV_Position
{
  float4 vPos = GetTileVertex(vPosition);
  return mul(vPos, g_mDirectionalLightSpaceTransform);
}

float4 PointShadow_VS( float2 vPosition : POSITION ) : SV_Position
{
  return GetTileVertex(vPosition);
}

//--------------------------------------------------------------------------------------
// Geometry Shaders
//--------------------------------------------------------------------------------------
[MaxVertexCount(18)]
void PointShadow_GS( triangle GS_POINTSHADOW_INPUT In[3],
                     inout TriangleStream<GS_POINTSHADOW_OUTPUT> MeshStream )
{
  GS_POINTSHADOW_OUTPUT Output;
  [unroll] for (uint i = 0; i < 6; ++i) {
    Output.RTI = i;
    [unroll] for (uint j = 0; j < 3; ++j) {
      Output.Position = mul(In[j].Position, g_mPointLightSpaceTransform[i]);
      Output.Depth = Output.Position.zw;
      MeshStream.Append(Output);
    }
    MeshStream.RestartStrip();
  }
}

//--------------------------------------------------------------------------------------
// Pixel Shaders
//--------------------------------------------------------------------------------------
float4 GouraudShading_PS( VS_GOURAUD_SHADING_OUTPUT In ) : SV_Target
{
  float4 vTerrainColor = GetColorFromHeight(In.Height);
  return vTerrainColor * g_vMaterialParameters.x +
    vTerrainColor * float4(In.DiffuseLight, 1) +
    float4(In.SpecularLight, 1);
}

float4 PhongShading_PS( VS_PHONG_SHADING_OUTPUT In, uniform bool bLODColoring ) : SV_Target
{
  float3 N = normalize(In.Normal);
  float4 vTerrainColor = 0;
  if (In.Height <= 0) {
    if (g_bWaveNormals) {
      // Waves bump map lookups
      float3 vWave0 = g_tWaves.Sample(g_ssLinear, In.Wave0);
      float3 vWave1 = g_tWaves.Sample(g_ssLinear, In.Wave1);
      float3 vWave2 = g_tWaves.Sample(g_ssLinear, In.Wave2);
      float3 vWave3 = g_tWaves.Sample(g_ssLinear, In.Wave3);
      // Combine waves to get normal pertubation vector
      float3 vNormalPertubation = normalize(vWave0 + vWave1 +
                                            vWave2 + vWave3 - 2);
      vNormalPertubation = vNormalPertubation.xzy;
      N = normalize(float3(0, 1, 0) + vNormalPertubation * 0.1f);
    }
    vTerrainColor = g_vWaterColor;
  } else {
    //vTerrainColor = GetColorFromHeight(In.Height);
    //vTerrainColor = g_tDirectionalShadowMap.Sample(g_ssLinear, In.TexCoord);
    //vTerrainColor = g_tGround.SampleLevel(g_ssLinear, In.TexCoord, 0);
    float fHeightNormalized = In.Height / g_fMaxHeight + 0.1;
    vTerrainColor = g_tGround3D.SampleLevel(g_ssLinear,
                                            float3(In.TexCoord,
                                                   fHeightNormalized),
                                            0);
  }

  if (bLODColoring) {
    vTerrainColor = g_LODColors[g_uiTileLOD % 6];
  }

  float3 vColor = FullLighting(vTerrainColor,
                               In.WorldPosition,
                               In.LightSpacePos,
                               N,
                               In.Material);

  float3 I = normalize(In.WorldPosition - g_vCamPos);
  float3 R = reflect(I, N);
  float4 vReflect = g_tCubeMap.Sample(g_ssLinear, R);

  return float4(vColor, 1) + In.Material.z * vReflect;
}

float4 Trees_PS( VS_TREES_OUTPUT In ) : SV_Target
{
  float3 N = normalize(In.Normal);
  float4 vColor = g_tMesh.Sample(g_ssLinear, In.TexCoord);

  if (vColor.a < 0.05) discard;
  
  vColor.rgb = FullLighting(vColor.rgb,
                            In.WorldPosition,
                            In.LightSpacePos,
                            N,
                            g_vTreeMaterial);

  return vColor;
}

float4 Environment_PS( float4 vPos : SV_Position ) : SV_Target
{
  // Transform vPos.xy in NDC
  vPos.xy /= float2(g_vBackBufferSize.x - 1, g_vBackBufferSize.y - 1);
  vPos.x = 2*vPos.x-1;
  vPos.y = 1-2*vPos.y;
  float fRatio = g_vBackBufferSize.x / (float)g_vBackBufferSize.y;
  // Calculate view vector in view space
  float3 vView = float3(fRatio * tan(0.5f * g_fCameraFOV) * vPos.x,
                        tan(0.5f * g_fCameraFOV) * vPos.y,
                        1.0);
  // Transform view vector in world space
  vView = mul(vView, g_mWorldViewInv);
  return g_tCubeMap.Sample(g_ssLinear, vView);
}

float DirectionalShadow_PS( float4 vPos : SV_Position ) : SV_Depth
{
  return vPos.z/vPos.w;
}

float PointShadow_PS( GS_POINTSHADOW_OUTPUT In ) : SV_Depth
{
  return In.Depth.x/In.Depth.y;
}

//--------------------------------------------------------------------------------------
// States
//--------------------------------------------------------------------------------------
DepthStencilState dssEnableDepth
{
  DepthEnable = TRUE;
  DepthWriteMask = ALL;
  DepthFunc = LESS;
};

BlendState bsNoColorWrite
{
  RenderTargetWriteMask[0] = 0;
  RenderTargetWriteMask[1] = 0;
  RenderTargetWriteMask[2] = 0;
  RenderTargetWriteMask[3] = 0;
  RenderTargetWriteMask[4] = 0;
  RenderTargetWriteMask[5] = 0;
  RenderTargetWriteMask[6] = 0;
  RenderTargetWriteMask[7] = 0;
};

BlendState bsAlphaToCov
{
  AlphaToCoverageEnable = TRUE;
  RenderTargetWriteMask[0] = 0x0F;
};

RasterizerState rsCullNone
{
  CullMode = None;
};

RasterizerState rsWireframe
{
  CullMode = Back;
  FillMode = Wireframe;
};

//--------------------------------------------------------------------------------------
// Techniques
//--------------------------------------------------------------------------------------
//technique10 GouraudShading
//{
//  pass P0
//  {
//    SetVertexShader( CompileShader( vs_4_0, GouraudShading_VS() ) );
//    SetGeometryShader( NULL );
//    SetPixelShader( CompileShader( ps_4_0, GouraudShading_PS() ) );
//    SetRasterizerState( NULL );
//    SetBlendState( NULL, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
//  }
//}

technique10 PhongShading
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, PhongShading_VS() ) );
    SetGeometryShader( NULL );
    SetPixelShader( CompileShader( ps_4_0, PhongShading_PS(false) ) );
    SetRasterizerState( NULL );
    SetBlendState( NULL, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
  }
}

technique10 Trees
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, Trees_VS() ) );
    SetGeometryShader( NULL );
    SetPixelShader( CompileShader( ps_4_0, Trees_PS() ) );
    SetRasterizerState( rsCullNone );
    SetBlendState( bsAlphaToCov, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
  }
}

technique10 LODColoring
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, PhongShading_VS() ) );
    SetGeometryShader( NULL );
    SetPixelShader( CompileShader( ps_4_0, PhongShading_PS(true) ) );
    SetRasterizerState( NULL );
    SetBlendState( NULL, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
  }
}

technique10 Environment
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, Environment_VS() ) );
    SetGeometryShader( NULL );
    SetPixelShader( CompileShader( ps_4_0, Environment_PS() ) );
    SetRasterizerState( NULL );
    SetBlendState( NULL, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
  }
}

technique10 DirectionalShadowMap
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, DirectionalShadow_VS() ) );
    SetGeometryShader( NULL );
    SetPixelShader( CompileShader( ps_4_0, DirectionalShadow_PS() ) );
    SetDepthStencilState( dssEnableDepth, 0 );
    SetRasterizerState( rsCullNone );
    SetBlendState( bsNoColorWrite, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
  }
}

technique10 PointShadowMap
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, PointShadow_VS() ) );
    SetGeometryShader( CompileShader( gs_4_0, PointShadow_GS() ) );
    SetPixelShader( CompileShader( ps_4_0, PointShadow_PS() ) );
    SetDepthStencilState( dssEnableDepth, 0 );
    SetRasterizerState( rsCullNone );
    SetBlendState( bsNoColorWrite, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
  }
}
