//--------------------------------------------------------------------------------------
// File: TerrainRenderer.fx
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
#define PI (3.14159265)

cbuffer cbPerPointEmitter
{
  float3 g_vPEPosition;
  float3 g_vPEDirection;
  float g_fPESpread;
}

cbuffer cbPerBoxEmitter
{
  float3 g_vBEMinVertex;
  float3 g_vBEMaxVertex;
}

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
  float    g_fElapsedTime;            // Elapsed time since last frame
  float4x4 g_mWorld;                  // World matrix for object
  float4x4 g_mWorldViewProjection;    // World * View * Projection matrix
  float3   g_vCamPos;                 // Camera position
  float4x4 g_mViewInv;
  // Lights
  float3   g_vPointLight_Position[8];
  float3   g_vDirectionalLight_Direction[8]; // Guaranteed to be normalized
  float3   g_vSpotLight_Position[8];
  float4x4 g_mDirectionalLightSpaceTransform;
  float4x4 g_mDirectionalTrapezoidToSquare;
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
Texture2D g_tGrass;
Texture2D g_tNoise;
Texture1D g_tRandom;

SamplerState g_ssLinear
{
  Filter = MIN_MAG_MIP_LINEAR;
  AddressU = Wrap;
  AddressV = Wrap;
  AddressW = Clamp;
};

SamplerState g_ssPoint
{
  Filter = MIN_MAG_MIP_POINT;
  AddressU = Wrap;
  AddressV = Wrap;
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
const float4 g_vWaterMaterial = float4(0.05, 0.15, 0.8, 200);
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
  float4 TSMPos         : TSMPOS;
  float  Height         : HEIGHT;
  float3 Normal         : NORMAL;
  float2 TexCoord       : TEXCOORD0;
  float2 Wave0          : TEXCOORD2;
  float2 Wave1          : TEXCOORD3;
  float2 Wave2          : TEXCOORD4;
  float2 Wave3          : TEXCOORD5;
  float4 Material       : MATERIAL; // = { k_a, k_d, k_s, n }
};

struct VS_DIRECTIONALSHADOW_OUTPUT
{
  float4 Position      : SV_Position;
  float4 LightSpacePos : LIGHTSPACEPOS;
};

struct VS_TREES_OUTPUT
{
  float4 Position      : SV_Position;
  float3 WorldPosition : WORLDPOS;
  float4 LightSpacePos : LIGHTSPACEPOS;
  float4 TSMPos        : TSMPOS;
  float3 Normal        : NORMAL;
  float2 TexCoord      : TEXCOORD0;
};

struct VS_RTS_OUTPUT
{
  float4 Position : SV_Position;
  float4 Color    : COLOR;
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

struct VS_SEED
{
  float3 Position : POSITION;
  float  Rotation : ROTATION;
  float  Size     : SIZE;
  float3 Normal   : NORMAL;
  uint   VertexID : SV_VertexID;
};

struct GS_SEED
{
  float3 Position : POSITION;
  float  Rotation : ROTATION;
  float  Size     : SIZE;
  float3 Normal   : NORMAL;
  uint   PlantID  : PLANTID;
};

struct PLANT_VERTEX
{
  float4 Position       : SV_Position;
  //float3 Normal         : NORMAL;
  //float3 WorldPosition  : WORLDPOS;
  //float4 LightSpacePos  : LIGHTSPACEPOS;
  //float4 TSMPos         : TSMPOS;
  PHONG  Phong          : PHONG;
  float2 TexCoord       : TEXCOORD0;
  uint   PlantID        : PLANTID;
};

struct PARTICLE
{
  float3 Position : POSITION;
  float3 Velocity : VELOCITY;
  float  Age      : AGE;
};

struct PARTICLE_BILLBOARD
{
  float4 Position : SV_Position;
  float2 TexCoord : TEXCOORD0;
  float  Age      : AGE;
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

PHONG PhongLighting(float3 vPos, float3 vNormal, float4 vMaterial, uniform bool bDeferShadowed=true)
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
    if (bDeferShadowed && g_bShadowedPointLight && i == g_iShadowedPointLight) continue;
    d = length(vPos - g_vPointLight_Position[i]);
    fAttenuation = 1 / dot(vAttenuation, float3(1, d, d*d));
    vPhong = Phong(vPos, g_vPointLight_Position[i] - vPos, vNormal, vMaterial);
    vDiffuseLight += vPhong.x * g_vPointLight_Color[i] * fAttenuation;
    vSpecularLight += vPhong.y * g_vPointLight_Color[i] * fAttenuation;
  }

  // Directional lights
  for (i = 0; i < g_nDirectionalLights; i++) {
    // Defer shadowed lighting
    if (bDeferShadowed && g_bShadowedDirectionalLight &&i == g_iShadowedDirectionalLight) continue;
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
                    float4 vTSMPos,
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
    vTSMPos /= vTSMPos.w;
    vTSMPos.x = 0.5 * vTSMPos.x + 0.5;
    vTSMPos.y = 1 - (0.5 * vTSMPos.y + 0.5);
    g_tDirectionalShadowMap.GetDimensions(uiWidth, uiHeight);
    vTSMPos.xy *= uint2(uiWidth-1, uiHeight-1);

    if (g_bPCF) {
      nInShadow = 0;
      [unroll] for (int x = -1; x <= 1; ++x) {
        [unroll] for (int y = -1; y <= 1; ++y) {
          fShadowMap = g_tDirectionalShadowMap.Load(int3(vTSMPos.xy + int2(x, y), 0));
          if (fShadowMap < fLightDist) nInShadow++;
        }
      }
      fLightScale = 1 - nInShadow / 9.0f;
    } else {
      fShadowMap = g_tDirectionalShadowMap.Load(int3(vTSMPos.xy, 0));
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
  Output.TSMPos = mul(Output.LightSpacePos, g_mDirectionalTrapezoidToSquare);
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

VS_TREES_OUTPUT Trees_VS( VS_TREES_INPUT Input ) {
  VS_TREES_OUTPUT Output;

  float4 vPos = float4(Input.Position, 1);
  float4x4 trans = Input.mTransform;
  //trans._14 = sin(g_fTime)/30;
  vPos = mul(vPos, trans);
  Output.Position = mul(vPos, g_mWorldViewProjection);
  Output.WorldPosition = Input.Position;
  Output.LightSpacePos = mul(vPos, g_mDirectionalLightSpaceTransform);
  Output.TSMPos = mul(Output.LightSpacePos, g_mDirectionalTrapezoidToSquare);
  Output.Normal = normalize(Input.Normal);
  Output.TexCoord = Input.TexCoord;

  return Output;
}

VS_TREES_OUTPUT TreesShadowMap_VS( VS_TREES_INPUT Input ) {
  VS_TREES_OUTPUT Output;

  float4 vPos = float4(Input.Position, 1);
  float4x4 trans = Input.mTransform;
  //trans._14 = sin(g_fTime)/30;
  vPos = mul(vPos, trans);
  Output.LightSpacePos = mul(vPos, g_mDirectionalLightSpaceTransform);
  Output.Position = mul(Output.LightSpacePos, g_mDirectionalTrapezoidToSquare);
  Output.TexCoord = Input.TexCoord;

  return Output;
}

float4 Environment_VS( float3 vPosition : POSITION ) : SV_Position
{
  return float4(vPosition, 1);
}

VS_DIRECTIONALSHADOW_OUTPUT DirectionalShadow_VS( float2 vPosition : POSITION )
{
  VS_DIRECTIONALSHADOW_OUTPUT Output;
  float4 vPos = GetTileVertex(vPosition);
  Output.LightSpacePos = mul(vPos, g_mDirectionalLightSpaceTransform);
  Output.Position = mul(Output.LightSpacePos, g_mDirectionalTrapezoidToSquare);
  return Output;
}

float4 PointShadow_VS( float2 vPosition : POSITION ) : SV_Position
{
  return GetTileVertex(vPosition);
}

VS_RTS_OUTPUT RenderToScreen_VS( float2 vPosition : POSITION, uint iVertex : SV_VertexID )
{
  VS_RTS_OUTPUT Output;
  Output.Position = float4(vPosition, 0, 1);
  Output.Color = g_LODColors[iVertex % 6];
  return Output;
}

GS_SEED Grass_VS(VS_SEED Input)
{
  GS_SEED Output;
  Output.Position = Input.Position;
  Output.Rotation = Input.Rotation;
  Output.Size = Input.Size;
  Output.Normal = Input.Normal;
  Output.PlantID = Input.VertexID;
  return Output;
}

PARTICLE Particles_VS(PARTICLE Input)
{
  return Input;
}

float4 ParticlePoint_VS(PARTICLE Input) : SV_Position
{
  return mul(float4(Input.Position, 1), g_mWorldViewProjection);
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

void CreatePlantQuad(float3 vBase, float3 vUp, float3 vRight, PLANT_VERTEX Output,
                     inout TriangleStream <PLANT_VERTEX> PlantStream,
                     float4x4 mTransform)
{
  float4 vVertex;
  vVertex.w = 1;

  // Links oben
  vVertex.xyz = vBase - vRight + vUp;
  Output.Position = mul(vVertex, mTransform);
  Output.TexCoord = float2(0,0);
  PlantStream.Append(Output);
  // Rechts oben
  vVertex.xyz = vBase + vRight + vUp;
  Output.Position = mul(vVertex, mTransform);
  Output.TexCoord = float2(1,0);
  PlantStream.Append(Output);
  // Links unten
  vVertex.xyz = vBase - vRight;
  Output.Position = mul(vVertex, mTransform);
  Output.TexCoord = float2(0,1);
  PlantStream.Append(Output);
  // Rechts unten
  vVertex.xyz = vBase + vRight;
  Output.Position = mul(vVertex, mTransform);
  Output.TexCoord = float2(1,1);
  PlantStream.Append(Output);

  PlantStream.RestartStrip();
}

[MaxVertexCount(8)]
void Grass_GS(point GS_SEED input[1], inout TriangleStream <PLANT_VERTEX> PlantStream)
{
  // Culling
  float4 vSeed = mul(float4(input[0].Position, 1), g_mWorldViewProjection);
  vSeed /= vSeed.w;
  if (vSeed.z < 0 || vSeed.z > 1 ||
      vSeed.x < -1.2 || vSeed.x > 1.2 ||
      vSeed.y < -2.0 || vSeed.y > 1.2) return;

  PLANT_VERTEX Output;
  Output.PlantID = input[0].PlantID;
  //Output.Normal = input[0].Normal;
  //Output.WorldPosition = input[0].Position;
  //Output.LightSpacePos = mul(float4(input[0].Position, 1), g_mDirectionalLightSpaceTransform);
  //Output.TSMPos = mul(Output.LightSpacePos, g_mDirectionalTrapezoidToSquare);
  Output.Phong = PhongLighting(input[0].Position, input[0].Normal, float4(0.05, 0.9, 0.05, 50), false);

  const float s = 0.5*sin(input[0].Rotation);
  const float c = 0.5*cos(input[0].Rotation);

  const float skew = 0.05*(sin(1*(input[0].Position.x+0.5*g_fTime)) +
                           sin(2*(input[0].Position.x+0.5*g_fTime)) +
                           sin(4*(input[0].Position.x+0.5*g_fTime)) +
                           sin(1*(input[0].Position.y+0.5*g_fTime)) +
                           sin(2*(input[0].Position.y+0.5*g_fTime)) +
                           sin(4*(input[0].Position.y+0.5*g_fTime)));

  const float billboardSize = input[0].Size;

  CreatePlantQuad(input[0].Position, float3(skew, 1, 0) * billboardSize,
                  float3(s, 0, c) * billboardSize, Output,
                  PlantStream, g_mWorldViewProjection);
  CreatePlantQuad(input[0].Position, float3(skew, 1, 0) * billboardSize,
                  float3(-c, 0, s) * billboardSize, Output,
                  PlantStream, g_mWorldViewProjection);
}

PARTICLE EulerStep(PARTICLE In)
{
  In.Position += g_fElapsedTime * In.Velocity;
  In.Velocity += g_fElapsedTime * float3(0, -0.6, 0);
  return In;
}

PARTICLE CollisionDetect(PARTICLE In)
{
  float2 vTexCoord = (In.Position.xz - g_vTileTranslate) / g_fTileScale;
  if (all(vTexCoord >= float2(0, 0) && vTexCoord <= float2(1, 1))) {
    float4 vHeightMap = g_tTerrain.SampleLevel(g_ssLinear, vTexCoord, 0);
    if (vHeightMap.w >= In.Position.y) {
      In.Position.y = vHeightMap.w;
      float fLength = length(In.Velocity);
      float3 vDir = In.Velocity/fLength;
      In.Velocity = reflect(vDir, vHeightMap.xyz) * (fLength * 0.8);
    }
  }
  return In;
}

float Random(float fOffset)
{
  return g_tRandom.SampleLevel(g_ssPoint, (g_fTime + fOffset)/4096, 0);
}

const float g_fMaxParticleAge = 20;

PARTICLE PointEmitterCreate(float fRandomOffset)
{
  PARTICLE p;
  p.Position = g_vPEPosition;
  float fAzimuth = Random(fRandomOffset)*PI*2;
  float fZenith = Random(fRandomOffset+23)*g_fPESpread;
  float3 vDir = float3(cos(fAzimuth)*sin(fZenith), cos(fZenith), sin(fAzimuth)*sin(fZenith));
  p.Velocity = vDir;
  p.Age = -Random(fRandomOffset+107)*g_fMaxParticleAge;
  return p;
}

PARTICLE BoxEmitterCreate(float fRandomOffset)
{
  PARTICLE p;
  p.Position = lerp(g_vBEMinVertex,
                    g_vBEMaxVertex,
                    float3(Random(fRandomOffset),
                           Random(fRandomOffset+17),
                           Random(fRandomOffset+109)));
  p.Velocity = float3(0, 0, 0);
  p.Age = -Random(fRandomOffset+107)*g_fMaxParticleAge;
  return p;
}

[MaxVertexCount(1)]
void Particles_GS(point PARTICLE input[1], uint ID : SV_PrimitiveID,
                  inout PointStream<PARTICLE> ParticleStream)
{
  PARTICLE p = input[0];
  p.Age += g_fElapsedTime;
  if (p.Age > g_fMaxParticleAge || p.Position.y <= 0) {
    p = BoxEmitterCreate(ID);
  } else if (p.Age >= 0) {
    p = EulerStep(p);
    p = CollisionDetect(p);
  } else {
    //p.Position = g_vPEPosition;
  }
  ParticleStream.Append(p);
}

[MaxVertexCount(4)]
void ParticleBillboard_GS(point PARTICLE input[1], inout TriangleStream<PARTICLE_BILLBOARD> ParticleStream)
{
  // Culling
  //float4 vParticle = mul(float4(input[0].Position, 1), g_mWorldViewProjection);
  //vParticle /= vParticle.w;
  //if (vParticle.z < 0 || vParticle.z > 1 ||
  //    vParticle.x < -1.2 || vParticle.x > 1.2 ||
  //    vParticle.y < -2.0 || vParticle.y > 1.2) return;

  PARTICLE_BILLBOARD Output;
  Output.Age = input[0].Age / g_fMaxParticleAge;
  float4 vVertex;
  vVertex.w = 1;
  const float fParticleSize = 0.05;
  float3 vBase = input[0].Position;
  float3 vRight = float3(1, 0, 0) * fParticleSize;
  float3 vUp = float3(0, 1, 0) * fParticleSize;

  // Links oben
  vVertex.xyz = vBase + mul(-vRight + 2*vUp, (float3x3)g_mViewInv);
  Output.Position = mul(vVertex, g_mWorldViewProjection);
  Output.TexCoord = float2(0,0);
  ParticleStream.Append(Output);
  // Rechts oben
  vVertex.xyz = vBase + mul(vRight + 2*vUp, (float3x3)g_mViewInv);
  Output.Position = mul(vVertex, g_mWorldViewProjection);
  Output.TexCoord = float2(1,0);
  ParticleStream.Append(Output);
  // Links unten
  vVertex.xyz = vBase + mul(-vRight, (float3x3)g_mViewInv);
  Output.Position = mul(vVertex, g_mWorldViewProjection);
  Output.TexCoord = float2(0,1);
  ParticleStream.Append(Output);
  // Rechts unten
  vVertex.xyz = vBase + mul(vRight, (float3x3)g_mViewInv);
  Output.Position = mul(vVertex, g_mWorldViewProjection);
  Output.TexCoord = float2(1,1);
  ParticleStream.Append(Output);

  ParticleStream.RestartStrip();
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
      N = normalize(float3(0, 1, 0) + vNormalPertubation * 0.1);
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
                               In.TSMPos,
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

  if (vColor.a < 0.15) discard;

  vColor.rgb = FullLighting(vColor.rgb,
                            In.WorldPosition,
                            In.LightSpacePos,
                            In.TSMPos,
                            N,
                            g_vTreeMaterial);

  return vColor;
}

float TreesShadowMap_PS( VS_TREES_OUTPUT In ) : SV_Depth
{
  float4 vColor = g_tMesh.Sample(g_ssLinear, In.TexCoord);
  if (vColor.a < 0.05) discard;

  return In.LightSpacePos.z/In.LightSpacePos.w;
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

float DirectionalShadow_PS( VS_DIRECTIONALSHADOW_OUTPUT In ) : SV_Depth
{
  return In.LightSpacePos.z/In.LightSpacePos.w;
}

float PointShadow_PS( GS_POINTSHADOW_OUTPUT In ) : SV_Depth
{
  return In.Depth.x/In.Depth.y;
}

float4 Color_PS( VS_RTS_OUTPUT In ) : SV_Target
{
  return In.Color;
}

float4 Grass_PS( PLANT_VERTEX In ) : SV_Target
{
  float fNoise = g_tNoise.Sample(g_ssLinear, In.TexCoord);
  const int type = In.PlantID % 4;
  In.TexCoord.x += type;
  In.TexCoord.x *= 0.25;
  float4 vColor = g_tGrass.Sample(g_ssLinear, In.TexCoord);
  vColor.a *= saturate(fNoise+0.1);

  //vColor.rgb = FullLighting(vColor.rgb,
  //                          In.WorldPosition,
  //                          In.LightSpacePos,
  //                          In.TSMPos,
  //                          In.Normal,
  //                          float4(0.05, 0.9, 0.0, 50));

  //PHONG phong = PhongLighting(In.WorldPosition, In.Normal, float4(0.05, 0.9, 0.05, 50));
  //vColor.rgb = vColor.rgb * 0.05 + vColor.rgb * phong.DiffuseLight + phong.SpecularLight;
  vColor.rgb = vColor.rgb * 0.05 + vColor.rgb * In.Phong.DiffuseLight + In.Phong.SpecularLight;

  return vColor;
}

float4 ParticlePoint_PS( ) : SV_Target
{
  return float4(1, 0, 0, 0);
}

float4 ParticleBillboard_PS( PARTICLE_BILLBOARD In ) : SV_Target
{
  float l = length(In.TexCoord * 2 - 1);
  if (l > 1 || In.Age < 0) discard;
  return float4(1 - In.Age, In.Age, In.Age, 1 - l);
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

DepthStencilState dssDisableDepthStencil
{
  DepthEnable = FALSE;
  StencilEnable = FALSE;
};

DepthStencilState dssEnableDepthNoWrite
{
  DepthEnable = TRUE;
  DepthWriteMask = 0;
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

BlendState bsSrcAlphaBlendingAdd
{
  BlendEnable[0] = TRUE;
  SrcBlend = SRC_ALPHA;
  DestBlend = INV_SRC_ALPHA;
  BlendOp = ADD;
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
    SetDepthStencilState( NULL, 0 );
    SetRasterizerState( NULL );
    SetBlendState( NULL, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
  }
}

technique10 LODColoring
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, PhongShading_VS() ) );
    SetGeometryShader( NULL );
    SetPixelShader( CompileShader( ps_4_0, PhongShading_PS(true) ) );
    SetDepthStencilState( NULL, 0 );
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
    SetDepthStencilState( NULL, 0 );
    SetRasterizerState( rsCullNone );
    SetBlendState( bsAlphaToCov, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
  }
}

technique10 TreesShadowMap
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, TreesShadowMap_VS() ) );
    SetGeometryShader( NULL );
    SetPixelShader( CompileShader( ps_4_0, TreesShadowMap_PS() ) );
    SetDepthStencilState( dssEnableDepth, 0 );
    SetRasterizerState( rsCullNone );
    SetBlendState( bsNoColorWrite, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
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

// Nur für Debug-Zwecke gebraucht
technique10 RenderToScreen
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, RenderToScreen_VS() ) );
    SetGeometryShader( NULL );
    SetPixelShader( CompileShader( ps_4_0, Color_PS() ) );
    SetDepthStencilState( NULL, 0 );
    SetRasterizerState( rsCullNone );
    SetBlendState( NULL, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
  }
}

technique10 Grass
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, Grass_VS() ) );
    SetGeometryShader( CompileShader( gs_4_0, Grass_GS() ) );
    SetPixelShader( CompileShader( ps_4_0, Grass_PS() ) );
    SetDepthStencilState( dssEnableDepthNoWrite, 0 );
    SetRasterizerState( rsCullNone );
    SetBlendState( bsSrcAlphaBlendingAdd, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
  }
}

GeometryShader gsParticleSim = ConstructGSWithSO(
  CompileShader( gs_4_0, Particles_GS() ),
  "POSITION.xyz; VELOCITY.xyz; AGE.x");

technique10 Particles
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, Particles_VS() ) );
    SetGeometryShader( gsParticleSim );
    SetPixelShader( NULL );
    SetDepthStencilState( dssDisableDepthStencil, 0 );
  }
}

technique10 RenderParticlesPoint
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, ParticlePoint_VS() ) );
    SetGeometryShader( NULL );
    SetPixelShader( CompileShader( ps_4_0, ParticlePoint_PS() ) );
    SetDepthStencilState( dssEnableDepthNoWrite, 0 );
    SetRasterizerState( rsCullNone );
    SetBlendState( NULL, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
  }
}

technique10 RenderParticlesBillboard
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, Particles_VS() ) );
    SetGeometryShader( CompileShader( gs_4_0, ParticleBillboard_GS() ) );
    SetPixelShader( CompileShader( ps_4_0, ParticleBillboard_PS() ) );
    SetDepthStencilState( dssEnableDepthNoWrite, 0 );
    SetRasterizerState( rsCullNone );
    SetBlendState( bsSrcAlphaBlendingAdd, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
  }
}