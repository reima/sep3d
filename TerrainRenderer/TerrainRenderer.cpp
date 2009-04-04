//--------------------------------------------------------------------------------------
// File: TerrainRenderer.cpp
//
// Starting point for new Direct3D 10 samples.  For a more basic starting point,
// use the EmptyProject10 sample instead.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "DXUT.h"
#include "DXUTgui.h"
#include "DXUTmisc.h"
#include "DXUTCamera.h"
#include "DXUTSettingsDlg.h"
#include "SDKmisc.h"
#include "resource.h"

#include "Terrain.h"
#include "LODSelector.h"
#include "FixedLODSelector.h"
#include "DynamicLODSelector.h"
#include "Scene.h"
#include "Environment.h"
#include "ShadowedDirectionalLight.h"
#include "ShadowedPointLight.h"
#include "PointEmitter.h"
#include "BoxEmitter.h"
#include "VolcanoEmitter.h"
#include "RainEmitter.h"

//#define DEBUG_VS   // Uncomment this line to debug D3D9 vertex shaders
//#define DEBUG_PS   // Uncomment this line to debug D3D9 pixel shaders

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
int   g_nTerrainN = 7;
float g_fTerrainR = 1.0f;
int   g_nTerrainLOD = 2;
float g_fTerrainScale = 50.0f;

extern const float g_fFOV = D3DX_PI / 4;

CFirstPersonCamera          g_Camera;               // A first person camera
CDXUTDialogResourceManager  g_DialogResourceManager; // manager for shared resources of dialogs
CD3DSettingsDlg             g_SettingsDlg;          // Device settings dialog
CDXUTTextHelper*            g_pTxtHelper = NULL;
CDXUTDialog                 g_HUD;                  // dialog for standard controls
CDXUTDialog                 g_SampleUI;             // dialog for sample specific controls
CDXUTDialog                 g_TerrainUI;

// Direct3D 10 resources
ID3DX10Font*                g_pFont10 = NULL;
ID3DX10Sprite*              g_pSprite10 = NULL;
ID3D10Effect*               g_pEffect10 = NULL;
ID3D10EffectTechnique*      g_pTechnique = NULL;
ID3D10EffectMatrixVariable* g_pmWorldViewProj = NULL;
ID3D10EffectMatrixVariable* g_pmWorldViewProjInv = NULL;
ID3D10EffectMatrixVariable* g_pmWorldViewProjLastFrame = NULL;
D3DXMATRIX mWorldViewProjectionLastFrame;
ID3D10EffectMatrixVariable* g_pmWorld = NULL;
ID3D10EffectScalarVariable* g_pfTime = NULL;
ID3D10EffectScalarVariable* g_pfMinHeight = NULL;
ID3D10EffectScalarVariable* g_pfMaxHeight = NULL;
ID3D10EffectScalarVariable* g_pbDynamicMinMax = NULL;
ID3D10EffectShaderResourceVariable* g_ptWaves = NULL;
ID3D10ShaderResourceView*   g_pWavesRV = NULL;
ID3D10EffectShaderResourceVariable* g_ptGround = NULL;
ID3D10ShaderResourceView*   g_pGroundRV = NULL;
ID3D10EffectShaderResourceVariable* g_ptGround3D = NULL;
ID3D10ShaderResourceView*   g_pGround3DRV = NULL;
ID3D10EffectShaderResourceVariable* g_ptCubeMap = NULL;
ID3D10ShaderResourceView*   g_pCubeMapRV = NULL;
ID3D10EffectScalarVariable* g_pbWaveNormals = NULL;
ID3D10EffectScalarVariable* g_pfZEpsilon = NULL;
ID3D10EffectScalarVariable* g_pbPCF = NULL;

LODSelector*                g_pLODSelector = NULL;
Scene*                      g_pScene = NULL;

ShadowedDirectionalLight*   g_pShadowedDirectionalLight = NULL;
ShadowedPointLight*         g_pShadowedPointLight = NULL;

bool                        g_bShowSettings = false;
bool                        g_bWireframe = false;
bool                        g_bPaused = false;
float                       g_fScreenError = 1.0f;
UINT                        g_uiScreenHeight = 600;
ID3D10RasterizerState*      g_pRSWireframe = NULL;
bool                        g_bTSM = false;
bool                        g_bDrawGUI = true;
//bool                        g_bDrawParticlePoints = false;
bool                        g_bPointEmitter = false;
bool                        g_bBoxEmitter = true;
PointEmitter*               g_pPointEmitter = NULL;
BoxEmitter*                 g_pBoxEmitter = NULL;





struct SCREEN_VERTEX
{
    D3DXVECTOR4 pos;
    D3DXVECTOR2 tex;

    static const DWORD FVF;
};


//--------------------------------------------------------------------------------------
//  POSTPROCESSING
//--------------------------------------------------------------------------------------
#define NUM_TONEMAP_TEXTURES 5
bool g_bHDRenabled = true;
bool g_bDOFenabled = false;
bool g_bMotionBlurenabled = true;

ID3D10DepthStencilView* g_pDSV = NULL;
ID3D10Texture2D* g_pDepthStencil = NULL;        
ID3D10ShaderResourceView* g_pDepthStencilRV = NULL;
ID3D10EffectShaderResourceVariable* g_tDepth;

ID3D10InputLayout* g_pQuadLayout = NULL;
ID3D10Buffer* g_pScreenQuadVB = NULL;
//ID3D10DepthStencilView* g_pDSV = NULL;
//ID3D10Texture2D* g_pDepthStencil = NULL;

ID3D10Texture2D* g_pHDRTarget0 = NULL;        
ID3D10ShaderResourceView* g_pHDRTarget0RV = NULL;
ID3D10RenderTargetView* g_pHDRTarget0RTV = NULL;
ID3D10EffectShaderResourceVariable* g_tHDRTarget0;

ID3D10Texture2D* g_pHDRTarget1 = NULL;        
ID3D10ShaderResourceView* g_pHDRTarget1RV = NULL;
ID3D10RenderTargetView* g_pHDRTarget1RTV = NULL;
ID3D10EffectShaderResourceVariable* g_tHDRTarget1;

ID3D10Texture2D* g_pToneMap[NUM_TONEMAP_TEXTURES]; 
ID3D10ShaderResourceView* g_pToneMapRV[NUM_TONEMAP_TEXTURES];
ID3D10RenderTargetView* g_pToneMapRTV[NUM_TONEMAP_TEXTURES];
ID3D10EffectShaderResourceVariable* g_tToneMap;


ID3D10Texture2D* g_pHDRBrightPass = NULL;     
ID3D10ShaderResourceView* g_pHDRBrightPassRV = NULL;
ID3D10RenderTargetView* g_pHDRBrightPassRTV = NULL;
ID3D10EffectShaderResourceVariable* g_tHDRBrightPass;

ID3D10Texture2D* g_pHDRBloom = NULL;     
ID3D10ShaderResourceView* g_pHDRBloomRV = NULL;
ID3D10RenderTargetView* g_pHDRBloomRTV = NULL;
ID3D10EffectShaderResourceVariable* g_tHDRBloom;

ID3D10Texture2D* g_pHDRBloom2 = NULL;     
ID3D10ShaderResourceView* g_pHDRBloom2RV = NULL;
ID3D10RenderTargetView* g_pHDRBloom2RTV = NULL;
ID3D10EffectShaderResourceVariable* g_tHDRBloom2;

ID3D10Texture2D* g_pDOFTex1 = NULL;     
ID3D10ShaderResourceView* g_pDOFTex1RV = NULL;
ID3D10RenderTargetView* g_pDOFTex1RTV = NULL;
ID3D10EffectShaderResourceVariable* g_tDOFTex1;

ID3D10Texture2D* g_pDOFTex2 = NULL;     
ID3D10ShaderResourceView* g_pDOFTex2RV = NULL;
ID3D10RenderTargetView* g_pDOFTex2RTV = NULL;
ID3D10EffectShaderResourceVariable* g_tDOFTex2;



//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
#define IDC_TOGGLEFULLSCREEN        1
#define IDC_TOGGLEREF               2
#define IDC_CHANGEDEVICE            3
#define IDC_NEWTERRAIN              4
#define IDC_LODSLIDER               5
#define IDC_LODSLIDER_S             6
#define IDC_PAUSED                  7
#define IDC_WIREFRAME               8
#define IDC_DYNAMICMINMAX           9
#define IDC_WAVENORMALS             10
#define IDC_TECHNIQUE               11
#define IDC_SHADOWMAPS_RESOLUTION   12
#define IDC_SHADOWMAPS_RESOLUTION_S 13
#define IDC_SHADOWMAPS_PRECISION    14
#define IDC_SHADOWMAPS_ZEPSILON     15
#define IDC_SHADOWMAPS_ZEPSILON_S   16
#define IDC_SHADOWMAPS_PCF          17
#define IDC_CAMERA_SPEED            18
#define IDC_CAMERA_SPEED_S          19
#define IDC_FLY_MODE                20
#define IDC_SCREEN_ERROR            21
#define IDC_SCREEN_ERROR_S          22
#define IDC_POINT_EMITTER           23
#define IDC_BOX_EMITTER             24

#define IDC_NEWTERRAIN_LOD          100
#define IDC_NEWTERRAIN_SIZE         101
#define IDC_NEWTERRAIN_ROUGHNESS    102
#define IDC_NEWTERRAIN_SCALE        103
#define IDC_NEWTERRAIN_LOD_S        104
#define IDC_NEWTERRAIN_SIZE_S       105
#define IDC_NEWTERRAIN_ROUGHNESS_S  106
#define IDC_NEWTERRAIN_SCALE_S      107
#define IDC_NEWTERRAIN_OK           108

#define IDC_HDR_ENABLED             201
#define IDC_DOF_ENABLED             202
#define IDC_MOTIONBLUR_ENABLED      203

//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
                         bool* pbNoFurtherProcessing, void* pUserContext);
void CALLBACK OnKeyboard(UINT nChar, bool bKeyDown, bool bAltDown,
                         void* pUserContext);
void CALLBACK OnGUIEvent(UINT nEvent, int nControlID, CDXUTControl* pControl,
                         void* pUserContext);
void CALLBACK OnFrameMove(double fTime, float fElapsedTime, void* pUserContext);
bool CALLBACK ModifyDeviceSettings(DXUTDeviceSettings* pDeviceSettings,
                                   void* pUserContext);

bool CALLBACK IsD3D10DeviceAcceptable(UINT Adapter, UINT Output,
                                      D3D10_DRIVER_TYPE DeviceType,
                                      DXGI_FORMAT BackBufferFormat,
                                      bool bWindowed, void* pUserContext);
HRESULT CALLBACK OnD3D10CreateDevice(ID3D10Device* pd3dDevice,
                                     const DXGI_SURFACE_DESC*
                                        pBackBufferSurfaceDesc,
                                     void* pUserContext );
HRESULT CALLBACK OnD3D10ResizedSwapChain(ID3D10Device* pd3dDevice,
                                         IDXGISwapChain* pSwapChain,
                                         const DXGI_SURFACE_DESC*
                                            pBackBufferSurfaceDesc,
                                         void* pUserContext);
void CALLBACK OnD3D10FrameRender(ID3D10Device* pd3dDevice, double fTime,
                                 float fElapsedTime, void* pUserContext);
void CALLBACK OnD3D10ReleasingSwapChain(void* pUserContext);
void CALLBACK OnD3D10DestroyDevice(void* pUserContext);

void InitApp();
void RenderText();

void DrawFullscreenQuad( ID3D10Device* pd3dDevice, ID3D10EffectTechnique* pTech, UINT Width, UINT Height );
HRESULT GetSampleOffsets_Bloom_D3D10( DWORD dwD3DTexSize, float afTexCoordOffset[15],
                                      D3DXVECTOR4* avColorWeight, float fDeviation, float fMultiplier );


ID3D10Effect* LoadEffect(ID3D10Device* pd3dDevice,
                         const std::wstring& filename,
                         const D3D10_SHADER_MACRO *Shader_Macros = NULL,
                         const bool bDebugCompile = false);

void ResetVolcano(void) {
  SAFE_DELETE(g_pPointEmitter);
  D3DXVECTOR3 volcano = g_pScene->GetTerrain()->GetHighestPoint();
  volcano.y += 0.5f;  
  g_pPointEmitter = new VolcanoEmitter(volcano, D3DXVECTOR3(0, 1, 0), 0.5*D3DX_PI);
  g_pPointEmitter->CreateBuffers(DXUTGetD3D10Device());
  g_pPointEmitter->GetShaderHandles(g_pEffect10);
}

void MakeItRain(void) {
  SAFE_DELETE(g_pBoxEmitter);
  float f = g_fTerrainScale*0.5f;
  g_pBoxEmitter = new RainEmitter(D3DXVECTOR3(-f, 10, -f), D3DXVECTOR3(f, 15, f), (UINT)(150*f*f));
  g_pBoxEmitter->CreateBuffers(DXUTGetD3D10Device());
  g_pBoxEmitter->GetShaderHandles(g_pEffect10);
}

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    LPWSTR lpCmdLine, int nCmdShow) {
  // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
  // Set DXUT callbacks
  DXUTSetCallbackMsgProc(MsgProc);
  DXUTSetCallbackKeyboard(OnKeyboard);
  DXUTSetCallbackFrameMove(OnFrameMove);
  DXUTSetCallbackDeviceChanging(ModifyDeviceSettings);

  DXUTSetCallbackD3D10DeviceAcceptable(IsD3D10DeviceAcceptable);
  DXUTSetCallbackD3D10DeviceCreated(OnD3D10CreateDevice);
  DXUTSetCallbackD3D10SwapChainResized(OnD3D10ResizedSwapChain);
  DXUTSetCallbackD3D10SwapChainReleasing(OnD3D10ReleasingSwapChain);
  DXUTSetCallbackD3D10DeviceDestroyed(OnD3D10DestroyDevice);
  DXUTSetCallbackD3D10FrameRender(OnD3D10FrameRender);

  InitApp();
  DXUTInit(true, true, NULL); // Parse the command line, show msgboxes on error,
                              // no extra command line params
  DXUTSetCursorSettings(true, true);
  DXUTCreateWindow(L"Terrain Renderer");
  DXUTCreateDevice(true, 800, 600);
  DXUTMainLoop(); // Enter into the DXUT render loop

  return DXUTGetExitCode();
}


//--------------------------------------------------------------------------------------
// Initialize the app
//--------------------------------------------------------------------------------------
void InitApp() {
  g_SettingsDlg.Init(&g_DialogResourceManager);
  g_HUD.Init(&g_DialogResourceManager);
  g_SampleUI.Init(&g_DialogResourceManager);
  g_TerrainUI.Init(&g_DialogResourceManager);

  g_HUD.SetCallback(OnGUIEvent);
  int iY = 10;
  g_HUD.AddButton(IDC_TOGGLEFULLSCREEN, L"Toggle full screen", 35, iY, 125, 22);
  g_HUD.AddButton(IDC_TOGGLEREF, L"Toggle REF (F3)", 35, iY += 24, 125, 22,
                  VK_F3);
  g_HUD.AddButton(IDC_CHANGEDEVICE, L"Change device (F2)", 35, iY += 24, 125,
                  22, VK_F2);

  /**
   * Sample UI
   */
  g_SampleUI.SetCallback(OnGUIEvent);
  iY = 10;
  g_SampleUI.AddButton(IDC_NEWTERRAIN, L"New Terrain...", 35, iY, 125,
                       22, VK_F2);
  //g_SampleUI.AddCheckBox(IDC_WIREFRAME, L"Wireframe (F5)", 35, iY += 24, 125,
  //                       22, g_bWireframe, VK_F5);
  g_SampleUI.AddCheckBox(IDC_PAUSED, L"Paused (F6)", 35, iY += 24, 125, 22,
                         g_bPaused, VK_F6);
  //g_SampleUI.AddCheckBox(IDC_DYNAMICMINMAX, L"Dynamic Min/Max (F7)", 35,
  //                       iY += 24, 125, 22, false, VK_F7);
  g_SampleUI.AddCheckBox(IDC_WAVENORMALS, L"Wave normals (F8)", 35, iY += 24,
                         125, 22, true, VK_F8);


  WCHAR sz[100];
  //StringCchPrintf(sz, 100, L"LOD (+/-): %d", 0);
  //g_SampleUI.AddStatic(IDC_LODSLIDER_S, sz, 35, iY += 24, 125, 22);
  //g_SampleUI.AddSlider(IDC_LODSLIDER, 35, iY += 24, 125, 22, 0, g_nTerrainLOD, 0);

  StringCchPrintf(sz, 100, L"Camera speed: %.2f", 0.25f);
  g_SampleUI.AddStatic(IDC_CAMERA_SPEED_S, sz, 35, iY += 24, 125, 22);
  g_SampleUI.AddSlider(IDC_CAMERA_SPEED, 35, iY += 24, 125, 22, 0, 1000, 100);

  g_SampleUI.AddCheckBox(IDC_FLY_MODE, L"Fly mode", 35, iY += 24, 125, 22, true);

  StringCchPrintf(sz, 100, L"Screen error: %.1f", 1.0f);
  g_SampleUI.AddStatic(IDC_SCREEN_ERROR_S, sz, 35, iY += 24, 125, 22);
  g_SampleUI.AddSlider(IDC_SCREEN_ERROR, 35, iY += 24, 125, 22, 0, 100, (int)(10*g_fScreenError));

  g_SampleUI.AddStatic(0, L"Technique:", 35, iY += 24, 125, 22);
  g_SampleUI.AddComboBox(IDC_TECHNIQUE, 35, iY += 24, 125, 22);
  g_SampleUI.GetComboBox(IDC_TECHNIQUE)->AddItem(L"PhongShading", "PhongShading");
  g_SampleUI.GetComboBox(IDC_TECHNIQUE)->AddItem(L"LOD Coloring", "LODColoring");

  StringCchPrintf(sz, 100, L"SM Res.: %dx%d", 1024, 1024);
  g_SampleUI.AddStatic(IDC_SHADOWMAPS_RESOLUTION_S, sz, 35, iY += 24, 125, 22);
  g_SampleUI.AddSlider(IDC_SHADOWMAPS_RESOLUTION, 35, iY += 24, 125, 22, 4, 11, 10);

  g_SampleUI.AddCheckBox(IDC_SHADOWMAPS_PRECISION, L"High Prec. SM", 35,
                         iY += 24, 125, 22, true);

  StringCchPrintf(sz, 100, L"Z epsilon: %.4f", 0.010f);
  g_SampleUI.AddStatic(IDC_SHADOWMAPS_ZEPSILON_S, sz, 35, iY += 24, 125, 22);
  g_SampleUI.AddSlider(IDC_SHADOWMAPS_ZEPSILON, 35, iY += 24, 125, 22, 0, 1000, 100);

  g_SampleUI.AddCheckBox(IDC_SHADOWMAPS_PCF, L"3x3 PCF", 35,
                         iY += 24, 125, 22, true);

  g_SampleUI.AddCheckBox(IDC_BOX_EMITTER, L"Rain", 35,
                         iY += 24, 125, 22, g_bBoxEmitter);
  g_SampleUI.AddCheckBox(IDC_POINT_EMITTER, L"Volcano", 35,
                         iY += 24, 125, 22, g_bPointEmitter);

  g_SampleUI.AddCheckBox(IDC_HDR_ENABLED, L"HDR", 35,
                         iY += 24, 125, 22, g_bHDRenabled);

  g_SampleUI.AddCheckBox(IDC_DOF_ENABLED, L"DOF", 35,
                         iY += 24, 125, 22, g_bDOFenabled);
  g_SampleUI.AddCheckBox(IDC_MOTIONBLUR_ENABLED, L"MotionBlur", 35,
                         iY += 24, 125, 22, g_bMotionBlurenabled);
  

  /**
   * Terrain UI
   */
  g_TerrainUI.SetCallback(OnGUIEvent);
  iY = 10;
  StringCchPrintf(sz, 100, L"Size: %dx%d", (1<<g_nTerrainN)+1, (1<<g_nTerrainN)+1);
  g_TerrainUI.AddStatic(IDC_NEWTERRAIN_SIZE_S, sz, 0, iY, 125, 22);
  g_TerrainUI.AddSlider(IDC_NEWTERRAIN_SIZE, 0, iY += 24, 125, 22, 1, 10,
                  g_nTerrainN);

  StringCchPrintf(sz, 100, L"Roughness: %.2f", g_fTerrainR);
  g_TerrainUI.AddStatic(IDC_NEWTERRAIN_ROUGHNESS_S, sz, 0, iY += 24, 125, 22);
  g_TerrainUI.AddSlider(IDC_NEWTERRAIN_ROUGHNESS, 0, iY += 24, 125, 22, 0, 1000,
                  (int)(g_fTerrainR*100));

  StringCchPrintf(sz, 100, L"LOD Levels: %d", g_nTerrainLOD);
  g_TerrainUI.AddStatic(IDC_NEWTERRAIN_LOD_S, sz, 0, iY += 24, 125, 22);
  g_TerrainUI.AddSlider(IDC_NEWTERRAIN_LOD, 0, iY += 24, 125, 22, 0, 5,
                  g_nTerrainLOD);

  StringCchPrintf(sz, 100, L"Scale: %.1f", g_fTerrainScale);
  g_TerrainUI.AddStatic(IDC_NEWTERRAIN_SCALE_S, sz, 0, iY += 24, 125, 22);
  g_TerrainUI.AddSlider(IDC_NEWTERRAIN_SCALE, 0, iY += 24, 125, 22, 10, 1000,
                  (int)(10*g_fTerrainScale));

  g_TerrainUI.AddButton(IDC_NEWTERRAIN_OK, L"Generate", 0, iY += 24, 125, 22);

  g_TerrainUI.SetVisible(false);
}


//--------------------------------------------------------------------------------------
// Render the help and statistics text. This function uses the ID3DXFont interface for
// efficient text rendering.
//--------------------------------------------------------------------------------------
void RenderText() {
  g_pTxtHelper->Begin();
  g_pTxtHelper->SetInsertionPos(5, 5);
  g_pTxtHelper->SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 0.0f, 1.0f));
  g_pTxtHelper->DrawTextLine(DXUTGetFrameStats(DXUTIsVsyncEnabled()));
  g_pTxtHelper->DrawTextLine(DXUTGetDeviceStats());

  if (g_bShowSettings) {
    g_pTxtHelper->SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f));
    g_pTxtHelper->DrawTextLine(L"Terrain Settings:");
    WCHAR sz[100];
    StringCchPrintf(sz, 100, L"Size: %dx%d", (1<<g_nTerrainN)+1, (1<<g_nTerrainN)+1);
    g_pTxtHelper->DrawTextLine(sz);
    StringCchPrintf(sz, 100, L"Roughness: %.2f", g_fTerrainR);
    g_pTxtHelper->DrawTextLine(sz);
    StringCchPrintf(sz, 100, L"LOD Levels: %d", g_nTerrainLOD);
    g_pTxtHelper->DrawTextLine(sz);
    D3DXVECTOR3 cam_pos = *g_Camera.GetEyePt();
    StringCchPrintf(sz, 100, L"Camera: (%f, %f, %f)", cam_pos.x, cam_pos.y, cam_pos.z);
    g_pTxtHelper->DrawTextLine(sz);
    StringCchPrintf(sz, 100, L"Trees: %d", g_pScene->GetTerrain()->GetNumTrees());
    g_pTxtHelper->DrawTextLine(sz);
    if (g_bTSM) {
      g_pTxtHelper->DrawTextLine(L"Shadow Mapping Technique: Trapezoidal (EXPERIMENTAL)");
    } else {
      g_pTxtHelper->DrawTextLine(L"Shadow Mapping Technique: naive");
    }
    D3DXVECTOR3 pe_pos = *g_pPointEmitter->GetPosition();
    StringCchPrintf(sz, 100, L"Volcano: (%f, %f, %f)", pe_pos.x, pe_pos.y, pe_pos.z);
    g_pTxtHelper->DrawTextLine(sz);
  }

  g_pTxtHelper->End();
}


//--------------------------------------------------------------------------------------
// Reject any D3D10 devices that aren't acceptable by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D10DeviceAcceptable(UINT Adapter, UINT Output,
                                      D3D10_DRIVER_TYPE DeviceType,
                                      DXGI_FORMAT BackBufferFormat,
                                      bool bWindowed, void* pUserContext) {
  return true;
}

//--------------------------------------------------------------------------------------
// Create any D3D10 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D10CreateDevice(ID3D10Device* pd3dDevice,
                                     const DXGI_SURFACE_DESC*
                                        pBackBufferSurfaceDesc,
                                     void* pUserContext) {
  HRESULT hr;

  V_RETURN(D3DX10CreateSprite(pd3dDevice, 500, &g_pSprite10));
  V_RETURN(g_DialogResourceManager.OnD3D10CreateDevice(pd3dDevice));
  V_RETURN(g_SettingsDlg.OnD3D10CreateDevice(pd3dDevice));
  V_RETURN(D3DX10CreateFont(pd3dDevice, 15, 0, FW_BOLD, 1, FALSE,
                            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                            L"Arial", &g_pFont10));
  g_pTxtHelper = new CDXUTTextHelper(NULL, NULL, g_pFont10, g_pSprite10, 15);

  // Read the D3DX effect file
  g_pEffect10 = LoadEffect(pd3dDevice, L"TerrainRenderer.fx", NULL, false);
  g_pTechnique = g_pEffect10->GetTechniqueByIndex(0);

  // Load wave normal map
  V_RETURN(D3DX10CreateShaderResourceViewFromFile(pd3dDevice,
      L"Textures/waves.dds", NULL, NULL, &g_pWavesRV, NULL));

  // Load ground texture
  V_RETURN(D3DX10CreateShaderResourceViewFromFile(pd3dDevice,
     L"Textures/checker1.png", NULL, NULL, &g_pGroundRV, NULL));

  // Load ground texture 3d
  V_RETURN(D3DX10CreateShaderResourceViewFromFile(pd3dDevice,
     L"Textures/terrain.dds", NULL, NULL, &g_pGround3DRV, NULL));

  // Load cube map
  V_RETURN(D3DX10CreateShaderResourceViewFromFile(pd3dDevice,
      L"Textures/SouthernSea.dds", NULL, NULL, &g_pCubeMapRV, NULL));

  // Get effects variables
  g_pmWorldViewProj =
      g_pEffect10->GetVariableByName("g_mWorldViewProjection")->AsMatrix();
  g_pmWorldViewProjInv =
      g_pEffect10->GetVariableByName("g_mViewInv")->AsMatrix();
  g_pmWorldViewProjLastFrame =
      g_pEffect10->GetVariableByName("g_mWorldViewProjectionLastFrame")->AsMatrix();
  
  g_pmWorld = g_pEffect10->GetVariableByName("g_mWorld")->AsMatrix();
  g_pfTime = g_pEffect10->GetVariableByName("g_fTime")->AsScalar();
  g_ptWaves = g_pEffect10->GetVariableByName("g_tWaves")->AsShaderResource();
  V_RETURN(g_ptWaves->SetResource(g_pWavesRV));
  g_ptGround = g_pEffect10->GetVariableByName("g_tGround")->AsShaderResource();
  V_RETURN(g_ptGround->SetResource(g_pGroundRV));
  g_ptGround3D = g_pEffect10->GetVariableByName("g_tGround3D")->AsShaderResource();
  V_RETURN(g_ptGround3D->SetResource(g_pGround3DRV));
  g_ptCubeMap =
      g_pEffect10->GetVariableByName("g_tCubeMap")->AsShaderResource();
  V_RETURN(g_ptCubeMap->SetResource(g_pCubeMapRV));
  g_pfMinHeight = g_pEffect10->GetVariableByName("g_fMinHeight")->AsScalar();
  g_pfMaxHeight = g_pEffect10->GetVariableByName("g_fMaxHeight")->AsScalar();
  g_pbDynamicMinMax =
      g_pEffect10->GetVariableByName("g_bDynamicMinMax")->AsScalar();
  g_pbWaveNormals =
      g_pEffect10->GetVariableByName("g_bWaveNormals")->AsScalar();
  g_pfZEpsilon =
      g_pEffect10->GetVariableByName("g_fZEpsilon")->AsScalar();
  g_pbPCF =
      g_pEffect10->GetVariableByName("g_bPCF")->AsScalar();

  // Flush effect vars/init GUI text
  //OnGUIEvent(0, IDC_DYNAMICMINMAX, NULL, NULL);
  OnGUIEvent(0, IDC_WAVENORMALS, NULL, NULL);
  OnGUIEvent(0, IDC_SHADOWMAPS_ZEPSILON, NULL, NULL);
  OnGUIEvent(0, IDC_SHADOWMAPS_PCF, NULL, NULL);
  OnGUIEvent(0, IDC_CAMERA_SPEED, NULL, NULL);
  OnGUIEvent(0, IDC_SCREEN_ERROR, NULL, NULL);

  // Setup the camera's view parameters
  D3DXVECTOR3 vecEye(0.0f, 0.1f, 0.0f);
  D3DXVECTOR3 vecAt (0.0f, 0.1f, 1.0f);
  g_Camera.SetViewParams(&vecEye, &vecAt);

  // RasterizerState für Wireframe erzeugen
  D3D10_RASTERIZER_DESC rast_desc = {
    D3D10_FILL_WIREFRAME, D3D10_CULL_BACK,
    FALSE,
    0, 0.0f, 0.0f,
    TRUE, FALSE, FALSE, FALSE
  };
  V_RETURN(pd3dDevice->CreateRasterizerState(&rast_desc, &g_pRSWireframe));

  // Szene erstellen
  g_pScene = new Scene();
  g_pScene->OnCreateDevice(pd3dDevice);
  g_pScene->GetShaderHandles(g_pEffect10);
  g_pScene->SetMaterial(0.05f, 0.9f, 0.05f, 50);
  g_pScene->SetCamera(&g_Camera);
  g_pScene->SetLODSelector(g_pLODSelector);  

  // Terrain erzeugen
  g_pScene->CreateTerrain(g_nTerrainN, g_fTerrainR, g_nTerrainLOD, g_fTerrainScale);
  Terrain *terrain = g_pScene->GetTerrain();
  g_pfMinHeight->SetFloat(terrain->GetMinHeight());
  g_pfMaxHeight->SetFloat(terrain->GetMaxHeight());

  // Lichter hinzufügen
  //g_pScene->AddPointLight(D3DXVECTOR3(-2.5f, 0.0f, 0.0f),
  //                        D3DXVECTOR3(0, 0, 1),
  //                        D3DXVECTOR3(0, 0, 1));
  //g_pScene->AddPointLight(D3DXVECTOR3(0.0f, 0.0f, -2.5f),
  //                        D3DXVECTOR3(0, 1, 0),
  //                        D3DXVECTOR3(1, 0, 0));
  //g_pScene->AddDirectionalLight(D3DXVECTOR3(1.0f, 1.0f, 0.0f),
  //                              D3DXVECTOR3(1, 0.75f, 0.5f),
  //                              D3DXVECTOR3(0, 1, 0));
  //g_pScene->AddSpotLight(D3DXVECTOR3(2.5f, 3.0f, 0.0f),
  //                       D3DXVECTOR3(0, -1, 0),
  //                       D3DXVECTOR3(1, 1, 0),
  //                       D3DXVECTOR3(0, 1, 0),
  //                       .5f, 5);
  //g_pScene->AddSpotLight(D3DXVECTOR3(-2.5f, 3.0f, 0.0f),
  //                       D3DXVECTOR3(0, -1, 0),
  //                       D3DXVECTOR3(0, 1, 1),
  //                       D3DXVECTOR3(0, 1, 0),
  //                       .5f, 5);
  //g_pScene->AddSpotLight(D3DXVECTOR3(0.0f, 3.0f, 2.5f),
  //                       D3DXVECTOR3(0, -1, 0),
  //                       D3DXVECTOR3(1, 0, 1),
  //                       D3DXVECTOR3(0, 1, 0),
  //                       .5f, 5);
  //g_pScene->AddSpotLight(D3DXVECTOR3(0.0f, 3.0f, -2.5f),
  //                       D3DXVECTOR3(0, -1, 0),
  //                       D3DXVECTOR3(1, 1, 1),
  //                       D3DXVECTOR3(0, 1, 0),
  //                       .5f, 5);
  g_pScene->AddDirectionalLight(
      D3DXVECTOR3(1.0f, 0.8f, 0.0f),
      D3DXVECTOR3(0.25, 0.25f, 0.25f),
      D3DXVECTOR3(0, 0.05f, 0),
      true);
  //g_pScene->AddPointLight(
  //    D3DXVECTOR3(-1, 1, 0),
  //    D3DXVECTOR3(1, 0, 0),
  //    D3DXVECTOR3(0, 1, 0),
  //    true);

  ResetVolcano();
  MakeItRain();
  
  /*DXGI_FORMAT fmt;  // steht schon in resizeswapchain
      fmt = DXGI_FORMAT_R16G16B16A16_FLOAT;
  

  // Create the render target texture
  D3D10_TEXTURE2D_DESC Desc;
  ZeroMemory( &Desc, sizeof( D3D10_TEXTURE2D_DESC ) );
  Desc.ArraySize = 1;
  Desc.BindFlags = D3D10_BIND_RENDER_TARGET | D3D10_BIND_SHADER_RESOURCE;
  Desc.Usage = D3D10_USAGE_DEFAULT;
  Desc.Format = fmt;
  Desc.Width = pBackBufferSurfaceDesc->Width;
  Desc.Height = pBackBufferSurfaceDesc->Height;
  Desc.MipLevels = 1;
  Desc.SampleDesc.Count = 1;
  V_RETURN( pd3dDevice->CreateTexture2D( &Desc, NULL, &g_pHDRTarget0 ) );

  // Create the render target view
  D3D10_RENDER_TARGET_VIEW_DESC DescRT;
  DescRT.Format = Desc.Format;
  DescRT.ViewDimension = D3D10_RTV_DIMENSION_TEXTURE2D;
  DescRT.Texture2D.MipSlice = 0;
  V_RETURN( pd3dDevice->CreateRenderTargetView( g_pHDRTarget0, &DescRT, &g_pHDRTarget0RTV ) );

  // Create the resource view
  D3D10_SHADER_RESOURCE_VIEW_DESC DescRV;
  DescRV.Format = Desc.Format;
  DescRV.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
  DescRV.Texture2D.MipLevels = 1;
  DescRV.Texture2D.MostDetailedMip = 0;
  V_RETURN( pd3dDevice->CreateShaderResourceView( g_pHDRTarget0, &DescRV, &g_pHDRTarget0RV ) );


  g_tHDRTarget0 = g_pEffect10->GetVariableByName( "g_tHDRTarget0" )->AsShaderResource();
  g_tHDRTarget0->SetResource(g_pHDRTarget0RV);



  // Create the bright pass texture
  Desc.Width /= 8;
  Desc.Height /= 8;
  Desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  V_RETURN( pd3dDevice->CreateTexture2D( &Desc, NULL, &g_pHDRBrightPass ) );

  // Create the render target view
  DescRT.Format = Desc.Format;
  DescRT.ViewDimension = D3D10_RTV_DIMENSION_TEXTURE2D;
  DescRT.Texture2D.MipSlice = 0;
  V_RETURN( pd3dDevice->CreateRenderTargetView( g_pHDRBrightPass, &DescRT, &g_pHDRBrightPassRTV ) );

  // Create the resource view
  DescRV.Format = Desc.Format;
  DescRV.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
  DescRV.Texture2D.MipLevels = 1;
  DescRV.Texture2D.MostDetailedMip = 0;
  V_RETURN( pd3dDevice->CreateShaderResourceView( g_pHDRBrightPass, &DescRV, &g_pHDRBrightPassRV ) );

  g_tHDRBrightPass = g_pEffect10->GetVariableByName( "g_tHDRBrightPass" )->AsShaderResource();
  g_tHDRBrightPass->SetResource(g_pHDRBrightPassRV);

  
  V_RETURN( pd3dDevice->CreateTexture2D( &Desc, NULL, &g_pHDRBloom ) );
  V_RETURN( pd3dDevice->CreateRenderTargetView( g_pHDRBloom, &DescRT, &g_pHDRBloomRTV ) );
  V_RETURN( pd3dDevice->CreateShaderResourceView( g_pHDRBloom, &DescRV, &g_pHDRBloomRV ) );

  g_tHDRBloom = g_pEffect10->GetVariableByName( "g_tHDRBloom" )->AsShaderResource();
  g_tHDRBloom->SetResource(g_pHDRBloomRV);

  V_RETURN( pd3dDevice->CreateTexture2D( &Desc, NULL, &g_pHDRBloom2 ) );
  V_RETURN( pd3dDevice->CreateRenderTargetView( g_pHDRBloom2, &DescRT, &g_pHDRBloom2RTV ) );
  V_RETURN( pd3dDevice->CreateShaderResourceView( g_pHDRBloom2, &DescRV, &g_pHDRBloom2RV ) );

  g_tHDRBloom2 = g_pEffect10->GetVariableByName( "g_tHDRBloom2" )->AsShaderResource();
  g_tHDRBloom2->SetResource(g_pHDRBloom2RV);*/

  // Create a screen quad for all render to texture operations
  SCREEN_VERTEX svQuad[4];
  svQuad[0].pos = D3DXVECTOR4( -1.0f, 1.0f, 0.5f, 1.0f );
  svQuad[0].tex = D3DXVECTOR2( 0.0f, 0.0f );
  svQuad[1].pos = D3DXVECTOR4( 1.0f, 1.0f, 0.5f, 1.0f );
  svQuad[1].tex = D3DXVECTOR2( 1.0f, 0.0f );
  svQuad[2].pos = D3DXVECTOR4( -1.0f, -1.0f, 0.5f, 1.0f );
  svQuad[2].tex = D3DXVECTOR2( 0.0f, 1.0f );
  svQuad[3].pos = D3DXVECTOR4( 1.0f, -1.0f, 0.5f, 1.0f );
  svQuad[3].tex = D3DXVECTOR2( 1.0f, 1.0f );

  D3D10_BUFFER_DESC vbdesc =
  {
    4 * sizeof( SCREEN_VERTEX ),
    D3D10_USAGE_DEFAULT,
    D3D10_BIND_VERTEX_BUFFER,
    0,
    0
  };

  D3D10_SUBRESOURCE_DATA InitData;
  InitData.pSysMem = svQuad;
  InitData.SysMemPitch = 0;
  InitData.SysMemSlicePitch = 0;
  V_RETURN( pd3dDevice->CreateBuffer( &vbdesc, &InitData, &g_pScreenQuadVB ) );


  // Create our quad input layout
  const D3D10_INPUT_ELEMENT_DESC quadlayout[] =
  {
      { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 },
      { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D10_INPUT_PER_VERTEX_DATA, 0 },
  };

  D3D10_PASS_DESC pass_desc;
  g_pEffect10->GetTechniqueByName("HDR_Luminosity")->GetPassByIndex(0)->GetDesc(&pass_desc);

  V_RETURN( pd3dDevice->CreateInputLayout( quadlayout, 2, pass_desc.pIAInputSignature,
                                           pass_desc.IAInputSignatureSize, &g_pQuadLayout ) );



 /* int nSampleLen = 1;
  for( int i = 0; i < NUM_TONEMAP_TEXTURES; i++ )
  {
    D3D10_TEXTURE2D_DESC tmdesc;
    ZeroMemory( &tmdesc, sizeof( D3D10_TEXTURE2D_DESC ) );
    tmdesc.ArraySize = 1;
    tmdesc.BindFlags = D3D10_BIND_RENDER_TARGET | D3D10_BIND_SHADER_RESOURCE;
    tmdesc.Usage = D3D10_USAGE_DEFAULT;
    tmdesc.Format = fmt;
    tmdesc.Width = nSampleLen;
    tmdesc.Height = nSampleLen;
    tmdesc.MipLevels = 1;
    tmdesc.SampleDesc.Count = 1;

    V_RETURN( pd3dDevice->CreateTexture2D( &tmdesc, NULL, &g_pToneMap[i] ) );

    // Create the render target view
    D3D10_RENDER_TARGET_VIEW_DESC DescRT;
    DescRT.Format = tmdesc.Format;
    DescRT.ViewDimension = D3D10_RTV_DIMENSION_TEXTURE2D;
    DescRT.Texture2D.MipSlice = 0;
    V_RETURN( pd3dDevice->CreateRenderTargetView( g_pToneMap[i], &DescRT, &g_pToneMapRTV[i] ) );

    // Create the shader resource view
    D3D10_SHADER_RESOURCE_VIEW_DESC DescRV;
    DescRV.Format = tmdesc.Format;
    DescRV.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
    DescRV.Texture2D.MipLevels = 1;
    DescRV.Texture2D.MostDetailedMip = 0;
    V_RETURN( pd3dDevice->CreateShaderResourceView( g_pToneMap[i], &DescRV, &g_pToneMapRV[i] ) );

    nSampleLen *= 3;
  }

  g_tToneMap = g_pEffect10->GetVariableByName( "g_tToneMap" )->AsShaderResource();*/

  return S_OK;
}


//--------------------------------------------------------------------------------------
// Create any D3D10 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D10ResizedSwapChain(ID3D10Device* pd3dDevice,
                                         IDXGISwapChain* pSwapChain,
                                         const DXGI_SURFACE_DESC*
                                            pBackBufferSurfaceDesc,
                                         void* pUserContext) {
  HRESULT hr;

  V_RETURN(g_DialogResourceManager.OnD3D10ResizedSwapChain(
      pd3dDevice,
      pBackBufferSurfaceDesc));
  V_RETURN(g_SettingsDlg.OnD3D10ResizedSwapChain(pd3dDevice,
                                                 pBackBufferSurfaceDesc));


  // Setup the camera's projection parameters
  float fAspectRatio = pBackBufferSurfaceDesc->Width /
      (FLOAT)pBackBufferSurfaceDesc->Height;
  g_Camera.SetProjParams(g_fFOV, fAspectRatio, 0.01f, 200.0f);

  g_HUD.SetLocation(pBackBufferSurfaceDesc->Width - 170, 0);
  g_HUD.SetSize(170, 170);
  g_SampleUI.SetLocation(pBackBufferSurfaceDesc->Width - 170, 80);
  g_SampleUI.SetSize(170, 300);
  g_TerrainUI.SetLocation(pBackBufferSurfaceDesc->Width - 320, 0);
  g_TerrainUI.SetSize(150, 300);

  g_uiScreenHeight = pBackBufferSurfaceDesc->Height;
  SAFE_DELETE(g_pLODSelector);
  g_pLODSelector = new DynamicLODSelector(g_fFOV, g_uiScreenHeight,
                                          g_fScreenError);
  if (g_pScene) {
    g_pScene->SetLODSelector(g_pLODSelector);
    g_pScene->OnResizedSwapChain(pBackBufferSurfaceDesc->Width,
                                 pBackBufferSurfaceDesc->Height);
  }



    // Create depth stencil texture.
    //
    D3D10_TEXTURE2D_DESC smtex;
    smtex.Width = pBackBufferSurfaceDesc->Width;
    smtex.Height = pBackBufferSurfaceDesc->Height;
    smtex.MipLevels = 1;
    smtex.ArraySize = 1;
    smtex.Format = DXGI_FORMAT_R32_TYPELESS; //DXGI_FORMAT_R32_TYPELESS
    smtex.SampleDesc.Count = 1;
    smtex.SampleDesc.Quality = 0;
    smtex.Usage = D3D10_USAGE_DEFAULT;
    smtex.BindFlags = D3D10_BIND_DEPTH_STENCIL | D3D10_BIND_SHADER_RESOURCE;
    smtex.CPUAccessFlags = 0;
    smtex.MiscFlags = 0;
    V_RETURN( pd3dDevice->CreateTexture2D( &smtex, NULL, &g_pDepthStencil ) );

    D3D10_SHADER_RESOURCE_VIEW_DESC DescRV;
    DescRV.Format = DXGI_FORMAT_R32_FLOAT;
    DescRV.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
    DescRV.Texture2D.MipLevels = 1;
    DescRV.Texture2D.MostDetailedMip = 0;
    V_RETURN( pd3dDevice->CreateShaderResourceView( g_pDepthStencil, &DescRV, &g_pDepthStencilRV ) );

    // Create the depth stencil view
    D3D10_DEPTH_STENCIL_VIEW_DESC DescDS;
    DescDS.Format = DXGI_FORMAT_D32_FLOAT;
    DescDS.ViewDimension = D3D10_DSV_DIMENSION_TEXTURE2D;
    DescDS.Texture2D.MipSlice = 0;
    V_RETURN( pd3dDevice->CreateDepthStencilView( g_pDepthStencil, &DescDS, &g_pDSV ) );

   // ID3D10RenderTargetView* pRTV = DXUTGetD3D10RenderTargetView();
   // pd3dDevice->OMSetRenderTargets( 1, &pRTV, g_pDSV );


    g_pEffect10->GetVariableByName( "g_tDepth" )->AsShaderResource()->SetResource(g_pDepthStencilRV);


     //g_tDepth->SetResource(


   /* ID3D10DepthStencilView* pDSV = DXUTGetD3D10DepthStencilView();

    D3D10_SHADER_RESOURCE_VIEW_DESC DescRV;
    DescRV.Format = DXGI_FORMAT_R32_TYPELESS;
    DescRV.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
    DescRV.Texture2D.MipLevels = 1;
    DescRV.Texture2D.MostDetailedMip = 0;
    V_RETURN( pd3dDevice->CreateShaderResourceView( pDSV->GetResource(), &DescRV, &g_pDepthStencilRV ) );

*/
  //HDR resize

  DXGI_FORMAT fmt;
      fmt = DXGI_FORMAT_R16G16B16A16_FLOAT;
  
 
  // Create the render target texture
  D3D10_TEXTURE2D_DESC Desc;
  ZeroMemory( &Desc, sizeof( D3D10_TEXTURE2D_DESC ) );
  Desc.ArraySize = 1;
  Desc.BindFlags = D3D10_BIND_RENDER_TARGET | D3D10_BIND_SHADER_RESOURCE;
  Desc.Usage = D3D10_USAGE_DEFAULT;
  Desc.Format = fmt;
  Desc.Width = pBackBufferSurfaceDesc->Width;
  Desc.Height = pBackBufferSurfaceDesc->Height;
  Desc.MipLevels = 1;
  Desc.SampleDesc.Count = 1;
  V_RETURN( pd3dDevice->CreateTexture2D( &Desc, NULL, &g_pHDRTarget0 ) );

  // Create the render target view
  D3D10_RENDER_TARGET_VIEW_DESC DescRT;
  DescRT.Format = Desc.Format;
  DescRT.ViewDimension = D3D10_RTV_DIMENSION_TEXTURE2D;
  DescRT.Texture2D.MipSlice = 0;
  V_RETURN( pd3dDevice->CreateRenderTargetView( g_pHDRTarget0, &DescRT, &g_pHDRTarget0RTV ) );

  // Create the resource view
  DescRV.Format = Desc.Format;
  V_RETURN( pd3dDevice->CreateShaderResourceView( g_pHDRTarget0, &DescRV, &g_pHDRTarget0RV ) );


  g_tHDRTarget0 = g_pEffect10->GetVariableByName( "g_tHDRTarget0" )->AsShaderResource();
  g_tHDRTarget0->SetResource(g_pHDRTarget0RV);

  V_RETURN( pd3dDevice->CreateTexture2D( &Desc, NULL, &g_pHDRTarget1 ) );
  V_RETURN( pd3dDevice->CreateRenderTargetView( g_pHDRTarget1, &DescRT, &g_pHDRTarget1RTV ) );
  V_RETURN( pd3dDevice->CreateShaderResourceView( g_pHDRTarget1, &DescRV, &g_pHDRTarget1RV ) );
  g_tHDRTarget1 = g_pEffect10->GetVariableByName( "g_tHDRTarget1" )->AsShaderResource();
  g_tHDRTarget1->SetResource(g_pHDRTarget1RV);


  Desc.Width /= 2;
  Desc.Height /= 2;

  V_RETURN( pd3dDevice->CreateTexture2D( &Desc, NULL, & g_pDOFTex1 ) );
  V_RETURN( pd3dDevice->CreateRenderTargetView(  g_pDOFTex1, &DescRT, & g_pDOFTex1RTV ) );
  V_RETURN( pd3dDevice->CreateShaderResourceView(  g_pDOFTex1, &DescRV, & g_pDOFTex1RV ) );

  g_tDOFTex1 = g_pEffect10->GetVariableByName( "g_tDOFTex1" )->AsShaderResource();
  g_tDOFTex1->SetResource(g_pDOFTex1RV);

  V_RETURN( pd3dDevice->CreateTexture2D( &Desc, NULL, & g_pDOFTex2 ) );
  V_RETURN( pd3dDevice->CreateRenderTargetView(  g_pDOFTex2, &DescRT, & g_pDOFTex2RTV ) );
  V_RETURN( pd3dDevice->CreateShaderResourceView(  g_pDOFTex2, &DescRV, & g_pDOFTex2RV ) );

  g_tDOFTex2 = g_pEffect10->GetVariableByName( "g_tDOFTex2" )->AsShaderResource();
  g_tDOFTex2->SetResource(g_pDOFTex2RV);
 


  // Create the bright pass texture
  Desc.Width /= 4;
  Desc.Height /= 4;
  Desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  V_RETURN( pd3dDevice->CreateTexture2D( &Desc, NULL, &g_pHDRBrightPass ) );

  // Create the render target view
  DescRT.Format = Desc.Format;
  DescRT.ViewDimension = D3D10_RTV_DIMENSION_TEXTURE2D;
  DescRT.Texture2D.MipSlice = 0;
  V_RETURN( pd3dDevice->CreateRenderTargetView( g_pHDRBrightPass, &DescRT, &g_pHDRBrightPassRTV ) );

  // Create the resource view
  DescRV.Format = Desc.Format;
  DescRV.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
  DescRV.Texture2D.MipLevels = 1;
  DescRV.Texture2D.MostDetailedMip = 0;
  V_RETURN( pd3dDevice->CreateShaderResourceView( g_pHDRBrightPass, &DescRV, &g_pHDRBrightPassRV ) );

  g_tHDRBrightPass = g_pEffect10->GetVariableByName( "g_tHDRBrightPass" )->AsShaderResource();
  g_tHDRBrightPass->SetResource(g_pHDRBrightPassRV);

  
  V_RETURN( pd3dDevice->CreateTexture2D( &Desc, NULL, &g_pHDRBloom ) );
  V_RETURN( pd3dDevice->CreateRenderTargetView( g_pHDRBloom, &DescRT, &g_pHDRBloomRTV ) );
  V_RETURN( pd3dDevice->CreateShaderResourceView( g_pHDRBloom, &DescRV, &g_pHDRBloomRV ) );

  g_tHDRBloom = g_pEffect10->GetVariableByName( "g_tHDRBloom" )->AsShaderResource();
  g_tHDRBloom->SetResource(g_pHDRBloomRV);

  V_RETURN( pd3dDevice->CreateTexture2D( &Desc, NULL, &g_pHDRBloom2 ) );
  V_RETURN( pd3dDevice->CreateRenderTargetView( g_pHDRBloom2, &DescRT, &g_pHDRBloom2RTV ) );
  V_RETURN( pd3dDevice->CreateShaderResourceView( g_pHDRBloom2, &DescRV, &g_pHDRBloom2RV ) );

  g_tHDRBloom2 = g_pEffect10->GetVariableByName( "g_tHDRBloom2" )->AsShaderResource();
  g_tHDRBloom2->SetResource(g_pHDRBloom2RV);

  
  int nSampleLen = 1;
  for( int i = 0; i < NUM_TONEMAP_TEXTURES; i++ )
  {
    D3D10_TEXTURE2D_DESC tmdesc;
    ZeroMemory( &tmdesc, sizeof( D3D10_TEXTURE2D_DESC ) );
    tmdesc.ArraySize = 1;
    tmdesc.BindFlags = D3D10_BIND_RENDER_TARGET | D3D10_BIND_SHADER_RESOURCE;
    tmdesc.Usage = D3D10_USAGE_DEFAULT;
    tmdesc.Format = fmt;
    tmdesc.Width = nSampleLen;
    tmdesc.Height = nSampleLen;
    tmdesc.MipLevels = 1;
    tmdesc.SampleDesc.Count = 1;

    V_RETURN( pd3dDevice->CreateTexture2D( &tmdesc, NULL, &g_pToneMap[i] ) );

    // Create the render target view
    D3D10_RENDER_TARGET_VIEW_DESC DescRT;
    DescRT.Format = tmdesc.Format;
    DescRT.ViewDimension = D3D10_RTV_DIMENSION_TEXTURE2D;
    DescRT.Texture2D.MipSlice = 0;
    V_RETURN( pd3dDevice->CreateRenderTargetView( g_pToneMap[i], &DescRT, &g_pToneMapRTV[i] ) );

    // Create the shader resource view
    D3D10_SHADER_RESOURCE_VIEW_DESC DescRV;
    DescRV.Format = tmdesc.Format;
    DescRV.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
    DescRV.Texture2D.MipLevels = 1;
    DescRV.Texture2D.MostDetailedMip = 0;
    V_RETURN( pd3dDevice->CreateShaderResourceView( g_pToneMap[i], &DescRV, &g_pToneMapRV[i] ) );

    nSampleLen *= 3;
  }

  g_tToneMap = g_pEffect10->GetVariableByName( "g_tToneMap" )->AsShaderResource();

  return S_OK;
}


//--------------------------------------------------------------------------------------
// Render the scene using the D3D10 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D10FrameRender(ID3D10Device* pd3dDevice, double fTime,
                                 float fElapsedTime, void* pUserContext) {
  D3DXMATRIX mWorldViewProjection;
  D3DXMATRIX mWorld;
  D3DXMATRIX mView;
  D3DXMATRIX mProj;

  const DXGI_SURFACE_DESC* pBackBufDesc = DXUTGetDXGIBackBufferSurfaceDesc();
    
  float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
  ID3D10RenderTargetView* pRTV = DXUTGetD3D10RenderTargetView();
  pd3dDevice->ClearRenderTargetView(pRTV, ClearColor);

  // Clear the depth stencil
  pd3dDevice->ClearDepthStencilView(g_pDSV, D3D10_CLEAR_DEPTH, 1.0, 0);

  // If the settings dialog is being shown, then render it instead of rendering the app's scene
  if (g_SettingsDlg.IsActive()) {
    g_SettingsDlg.OnRender(fElapsedTime);
    return;
  }

  // Get the projection & view matrix from the camera class
  mWorld = *g_Camera.GetWorldMatrix();
  mProj = *g_Camera.GetProjMatrix();
  mView = *g_Camera.GetViewMatrix();
  mWorldViewProjection = mView * mProj;

  // Update the effect's variables
  g_pmWorldViewProj->SetMatrix((float*)&mWorldViewProjection);
  g_pmWorldViewProjLastFrame->SetMatrix((float*)&mWorldViewProjectionLastFrame);
  mWorldViewProjectionLastFrame=mWorldViewProjection;

  D3DXMATRIX view_inv;
  D3DXMatrixInverse(&view_inv, NULL, &mWorldViewProjection);
  g_pmWorldViewProjInv->SetMatrix((float*)&view_inv);

  g_pmWorld->SetMatrix((float*)&mWorld);
  g_pfTime->SetFloat((float)fTime);

  //
  // Render the scene.
  //
  if (g_bWireframe) pd3dDevice->RSSetState(g_pRSWireframe);

  
  // Store off original render targets
  ID3D10RenderTargetView* pOrigRTV = NULL;
  ID3D10DepthStencilView* pOrigDSV = NULL;
  pd3dDevice->OMGetRenderTargets( 1, &pOrigRTV, &pOrigDSV );

  // Setup the HDR render target -> g_tHDRTarget0

  g_tHDRTarget0->SetResource(g_pHDRTarget0RV);

  ID3D10RenderTargetView* aRTViews[ 1 ] = { g_pHDRTarget0RTV };
  //pd3dDevice->OMSetRenderTargets( 1, aRTViews, pOrigDSV );
  
  pd3dDevice->OMSetRenderTargets( 1, aRTViews, g_pDSV );

  pd3dDevice->ClearRenderTargetView( g_pDOFTex1RTV, ClearColor );
  pd3dDevice->ClearRenderTargetView( g_pDOFTex2RTV, ClearColor );
  pd3dDevice->ClearRenderTargetView( g_pHDRTarget0RTV, ClearColor );
  pd3dDevice->ClearRenderTargetView( g_pHDRTarget1RTV, ClearColor );

  DXUT_BeginPerfEvent(DXUT_PERFEVENTCOLOR, L"Scene");
  g_pScene->Draw(g_pTechnique);
  DXUT_EndPerfEvent();


  //Motion Blur

  if (g_bMotionBlurenabled) { 
    D3D10_TEXTURE2D_DESC descScreen;
    g_pHDRTarget1->GetDesc( &descScreen );
   
    aRTViews[0] =  g_pHDRTarget1RTV;
    pd3dDevice->OMSetRenderTargets( 1, aRTViews, NULL );

    DrawFullscreenQuad( pd3dDevice, g_pEffect10->GetTechniqueByName("Motion_Blur"), descScreen.Width, descScreen.Height );
     
    ID3D10Texture2D* textemp = g_pHDRTarget0;     
    ID3D10ShaderResourceView* textempRV = g_pHDRTarget0RV;
    ID3D10RenderTargetView* textempRTV = g_pHDRTarget0RTV;
    
    g_pHDRTarget0 = g_pHDRTarget1;     
    g_pHDRTarget0RV = g_pHDRTarget1RV;
    g_pHDRTarget0RTV = g_pHDRTarget1RTV;

    g_pHDRTarget1 = textemp;     
    g_pHDRTarget1RV = textempRV;
    g_pHDRTarget1RTV = textempRTV;

    g_tHDRTarget1->SetResource(g_pHDRTarget1RV);
    g_tHDRTarget0->SetResource(g_pHDRTarget0RV);

    pd3dDevice->ClearRenderTargetView( g_pHDRTarget1RTV, ClearColor );
  }

  // DOF 
  // bloomH
  if (g_bDOFenabled) {

    D3D10_TEXTURE2D_DESC descScreen;
    g_pDOFTex1->GetDesc( &descScreen );
   
    aRTViews[0] =  g_pDOFTex1RTV;
    pd3dDevice->OMSetRenderTargets( 1, aRTViews, NULL );

    DrawFullscreenQuad( pd3dDevice, g_pEffect10->GetTechniqueByName("DOF_BloomH"), descScreen.Width, descScreen.Height );
   
    // bloomV
    aRTViews[0] =  g_pDOFTex2RTV;
    pd3dDevice->OMSetRenderTargets( 1, aRTViews, NULL );

    DrawFullscreenQuad( pd3dDevice, g_pEffect10->GetTechniqueByName("DOF_BloomV"), descScreen.Width, descScreen.Height );
    
    // DoF final

    g_pHDRTarget1->GetDesc( &descScreen );
    pd3dDevice->ClearRenderTargetView( g_pDOFTex1RTV, ClearColor );
    aRTViews[0] =  g_pHDRTarget1RTV;
    pd3dDevice->OMSetRenderTargets( 1, aRTViews, NULL );

    DrawFullscreenQuad( pd3dDevice, g_pEffect10->GetTechniqueByName("DOF_Final"), descScreen.Width, descScreen.Height );


    g_tHDRTarget0->SetResource(g_pHDRTarget1RV); 
  }

  if (g_bHDRenabled) { // hdr = on
    ID3D10ShaderResourceView* pTexSrc = NULL;
    ID3D10ShaderResourceView* pTexDest = NULL;
    ID3D10RenderTargetView* pSurfDest = NULL;
    D3D10_TEXTURE2D_DESC descSrc;
    g_pHDRTarget0->GetDesc( &descSrc );
    D3D10_TEXTURE2D_DESC descDest;
    g_pToneMap[NUM_TONEMAP_TEXTURES - 1]->GetDesc( &descDest );

    aRTViews[0] =  g_pToneMapRTV[NUM_TONEMAP_TEXTURES-1];
    pd3dDevice->OMSetRenderTargets( 1, aRTViews, NULL );
    
    //luminosity erzeugen
    DrawFullscreenQuad( pd3dDevice, g_pEffect10->GetTechniqueByName("HDR_Luminosity"), descDest.Width, descDest.Height );
//

    //downsamplen 
    for( int i = NUM_TONEMAP_TEXTURES - 1; i > 0; i-- )
      {
        pTexSrc = g_pToneMapRV[i];
        pTexDest = g_pToneMapRV[i - 1];
        pSurfDest = g_pToneMapRTV[i - 1];

        D3D10_TEXTURE2D_DESC desc;
        g_pToneMap[i]->GetDesc( &desc );

        aRTViews[0] = pSurfDest;
        pd3dDevice->OMSetRenderTargets( 1, aRTViews, NULL );

        g_tToneMap->SetResource( pTexSrc );

        DrawFullscreenQuad( pd3dDevice, g_pEffect10->GetTechniqueByName("HDR_3x3_Downsampling"), desc.Width / 3, desc.Height / 3 );

        g_tToneMap->SetResource( NULL );
      }

    // bright pass filter
    aRTViews[0] = g_pHDRBrightPassRTV;
    pd3dDevice->OMSetRenderTargets( 1, aRTViews, NULL );
   
    g_tToneMap->SetResource( g_pToneMapRV[0] );

    DrawFullscreenQuad( pd3dDevice,  g_pEffect10->GetTechniqueByName("HDR_BrightPass"), pBackBufDesc->Width / 8,
                          pBackBufDesc->Height / 8 );

    //bloom
    aRTViews[0] = g_pHDRBloomRTV;
    pd3dDevice->OMSetRenderTargets( 1, aRTViews, NULL );
    DrawFullscreenQuad( pd3dDevice,  g_pEffect10->GetTechniqueByName("HDR_BloomH"), pBackBufDesc->Width / 8,
                          pBackBufDesc->Height / 8 );

    aRTViews[0] = g_pHDRBloom2RTV;
    pd3dDevice->OMSetRenderTargets( 1, aRTViews, NULL );
    DrawFullscreenQuad( pd3dDevice,  g_pEffect10->GetTechniqueByName("HDR_BloomV"), pBackBufDesc->Width / 8,
                          pBackBufDesc->Height / 8 );
    

    //auf screen rendern 
    aRTViews[ 0 ] =  pOrigRTV ;
    //pd3dDevice->OMSetRenderTargets( 1, aRTViews, pOrigDSV );
    pd3dDevice->OMSetRenderTargets( 1, aRTViews, NULL );   
   
    DrawFullscreenQuad( pd3dDevice,  g_pEffect10->GetTechniqueByName("HDR_FinalPass"), pBackBufDesc->Width ,
                          pBackBufDesc->Height);
  }
  else //hdr = off
  {
    aRTViews[ 0 ] =  pOrigRTV ;
    pd3dDevice->OMSetRenderTargets( 1, aRTViews, NULL );
    //pd3dDevice->OMSetRenderTargets( 1, aRTViews, pOrigDSV ); 
    DrawFullscreenQuad( pd3dDevice,  g_pEffect10->GetTechniqueByName("HDR_FinalPass_disabled"), pBackBufDesc->Width ,
                        pBackBufDesc->Height);
  }

  if (g_bPointEmitter) g_pPointEmitter->Draw();
  if (g_bBoxEmitter) g_pBoxEmitter->Draw();
  
  if (g_bWireframe) pd3dDevice->RSSetState(NULL);

  DXUT_BeginPerfEvent(DXUT_PERFEVENTCOLOR, L"HUD / Stats");
  if (g_bDrawGUI) {
    RenderText();
    g_HUD.OnRender(fElapsedTime);
    g_SampleUI.OnRender(fElapsedTime);
    g_TerrainUI.OnRender(fElapsedTime);
  }
  DXUT_EndPerfEvent();

  SAFE_RELEASE(pOrigRTV);
  SAFE_RELEASE(pOrigDSV);

}


//--------------------------------------------------------------------------------------
// Release D3D10 resources created in OnD3D10ResizedSwapChain
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D10ReleasingSwapChain(void* pUserContext) {
  g_DialogResourceManager.OnD3D10ReleasingSwapChain();

   
  SAFE_RELEASE(g_pHDRTarget0);        
  SAFE_RELEASE(g_pHDRTarget0RV);
  SAFE_RELEASE(g_pHDRTarget0RTV);

  SAFE_RELEASE(g_pHDRTarget1);        
  SAFE_RELEASE(g_pHDRTarget1RV);
  SAFE_RELEASE(g_pHDRTarget1RTV);

  //SAFE_DELETE(g_tHDRTarget0);
  g_tHDRTarget0 = NULL;

  for( int i = 0; i < NUM_TONEMAP_TEXTURES; i++ )
  {
    SAFE_RELEASE( g_pToneMap[i] );
    SAFE_RELEASE( g_pToneMapRV[i] );
    SAFE_RELEASE( g_pToneMapRTV[i] );
  }

  g_tToneMap = NULL;


  SAFE_RELEASE(g_pHDRBrightPass);   
  SAFE_RELEASE(g_pHDRBrightPassRV);
  SAFE_RELEASE(g_pHDRBrightPassRTV);
  g_tHDRBrightPass = NULL;

  SAFE_RELEASE(g_pHDRBloom);    
  SAFE_RELEASE(g_pHDRBloomRV);
  SAFE_RELEASE(g_pHDRBloomRTV);
  g_tHDRBloom = NULL;


  SAFE_RELEASE(g_pHDRBloom2);    
  SAFE_RELEASE(g_pHDRBloom2RV);
  SAFE_RELEASE(g_pHDRBloom2RTV);
  g_tHDRBloom2 = NULL;

  SAFE_RELEASE(g_pDOFTex1);    
  SAFE_RELEASE(g_pDOFTex1RV);
  SAFE_RELEASE(g_pDOFTex1RTV);
  g_tDOFTex1 = NULL;

  SAFE_RELEASE(g_pDOFTex2);    
  SAFE_RELEASE(g_pDOFTex2RV);
  SAFE_RELEASE(g_pDOFTex2RTV);
  g_tDOFTex2 = NULL;

  SAFE_RELEASE(g_pDSV);    
  SAFE_RELEASE(g_pDepthStencil);
  SAFE_RELEASE(g_pDepthStencilRV);
}


//--------------------------------------------------------------------------------------
// Release D3D10 resources created in OnD3D10CreateDevice
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D10DestroyDevice(void* pUserContext) {
  g_DialogResourceManager.OnD3D10DestroyDevice();
  g_SettingsDlg.OnD3D10DestroyDevice();
  SAFE_RELEASE(g_pFont10);
  SAFE_RELEASE(g_pEffect10);

  SAFE_RELEASE(g_pSprite10);
  SAFE_RELEASE(g_pRSWireframe);
  SAFE_RELEASE(g_pGroundRV);
  SAFE_RELEASE(g_pGround3DRV);
  SAFE_RELEASE(g_pWavesRV);
  SAFE_RELEASE(g_pCubeMapRV);
  SAFE_DELETE(g_pTxtHelper);
  SAFE_DELETE(g_pLODSelector);
  SAFE_DELETE(g_pScene);
  SAFE_DELETE(g_pBoxEmitter);
  SAFE_DELETE(g_pPointEmitter);

  SAFE_RELEASE(g_pQuadLayout);
  SAFE_RELEASE(g_pScreenQuadVB);

}


//--------------------------------------------------------------------------------------
// Called right before creating a D3D9 or D3D10 device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings(DXUTDeviceSettings* pDeviceSettings,
                                   void* pUserContext) {
  // For the first device created if its a REF device, optionally display a warning dialog box
  static bool s_bFirstTime = true;
  if (s_bFirstTime) {
    s_bFirstTime = false;
    if ((DXUT_D3D9_DEVICE == pDeviceSettings->ver &&
         pDeviceSettings->d3d9.DeviceType == D3DDEVTYPE_REF) ||
        (DXUT_D3D10_DEVICE == pDeviceSettings->ver &&
        pDeviceSettings->d3d10.DriverType == D3D10_DRIVER_TYPE_REFERENCE)) {
      DXUTDisplaySwitchingToREFWarning(pDeviceSettings->ver);
    }
  }
  pDeviceSettings->d3d10.SyncInterval = 0;

  return true;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove(double fTime, float fElapsedTime,
                          void* pUserContext) {
  // Update the camera's position based on user input
  g_Camera.FrameMove(fElapsedTime);
  if (g_bPaused) fElapsedTime = 0;
  g_pScene->OnFrameMove(fElapsedTime);

  if (g_bPointEmitter) g_pPointEmitter->SimulationStep(fElapsedTime);
  if (g_bBoxEmitter) g_pBoxEmitter->SimulationStep(fElapsedTime);
}


//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
                         bool* pbNoFurtherProcessing, void* pUserContext) {
  // Pass messages to dialog resource manager calls so GUI state is updated correctly
  *pbNoFurtherProcessing = g_DialogResourceManager.MsgProc(hWnd,
                                                           uMsg,
                                                           wParam,
                                                           lParam);
  if (*pbNoFurtherProcessing)
    return 0;

  // Pass messages to settings dialog if its active
  if (g_SettingsDlg.IsActive()) {
    g_SettingsDlg.MsgProc(hWnd, uMsg, wParam, lParam);
    return 0;
  }

  // Give the dialogs a chance to handle the message first
  *pbNoFurtherProcessing = g_HUD.MsgProc(hWnd, uMsg, wParam, lParam);
  if (*pbNoFurtherProcessing)
    return 0;
  *pbNoFurtherProcessing = g_SampleUI.MsgProc(hWnd, uMsg, wParam, lParam);
  if (*pbNoFurtherProcessing)
    return 0;
  *pbNoFurtherProcessing = g_TerrainUI.MsgProc(hWnd, uMsg, wParam, lParam);
  if (*pbNoFurtherProcessing)
    return 0;

  // Pass all remaining windows messages to camera so it can respond to user input
  g_Camera.HandleMessages(hWnd, uMsg, wParam, lParam);

  return 0;
}

//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard(UINT nChar, bool bKeyDown, bool bAltDown,
                         void* pUserContext) {
  if (!bKeyDown) return;
  switch (nChar) {
    case 'h':
    case 'H':
      g_bShowSettings = !g_bShowSettings;
      break;
    case 't':
    case 'T':
      g_bTSM = !g_bTSM;
      break;
    case 'g':
    case 'G':
      g_bDrawGUI = !g_bDrawGUI;
      break;
    //case 'p':
    //case 'P':
    //  g_bDrawParticlePoints = !g_bDrawParticlePoints;
    //  break;
  }
}


//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent(UINT nEvent, int nControlID, CDXUTControl* pControl,
                         void* pUserContext) {
  WCHAR sz[100];
  switch (nControlID) {
    case IDC_TOGGLEFULLSCREEN:
      DXUTToggleFullScreen(); break;
    case IDC_TOGGLEREF:
      DXUTToggleREF(); break;
    case IDC_CHANGEDEVICE:
      g_SettingsDlg.SetActive(!g_SettingsDlg.IsActive()); break;

    /**
     * Sample UI
     */
    case IDC_WIREFRAME:
      g_bWireframe = g_SampleUI.GetCheckBox(IDC_WIREFRAME)->GetChecked();
      break;
    case IDC_PAUSED:
      g_bPaused = g_SampleUI.GetCheckBox(IDC_PAUSED)->GetChecked();
      break;
    case IDC_DYNAMICMINMAX:
      g_pbDynamicMinMax->SetBool(
          g_SampleUI.GetCheckBox(IDC_DYNAMICMINMAX)->GetChecked());
      break;
    case IDC_WAVENORMALS:
      g_pbWaveNormals->SetBool(
          g_SampleUI.GetCheckBox(IDC_WAVENORMALS)->GetChecked());
      break;
    case IDC_NEWTERRAIN:
      g_TerrainUI.SetVisible(true);
      break;
    case IDC_TECHNIQUE: {
      char *tech =
          (char *)g_SampleUI.GetComboBox(IDC_TECHNIQUE)->GetSelectedData();
      g_pTechnique = g_pEffect10->GetTechniqueByName(tech);
      break;
    }
    case IDC_SHADOWMAPS_RESOLUTION: {
      int value =
          (1 << g_SampleUI.GetSlider(IDC_SHADOWMAPS_RESOLUTION)->GetValue());
      StringCchPrintf(sz, 100, L"SM Res.: %dx%d", value, value);
      g_SampleUI.GetStatic(IDC_SHADOWMAPS_RESOLUTION_S)->SetText(sz);
      g_pScene->SetShadowMapDimensions(value, value);
      break;
    }
    case IDC_SHADOWMAPS_PRECISION:
      g_pScene->SetShadowMapPrecision(
          g_SampleUI.GetCheckBox(IDC_SHADOWMAPS_PRECISION)->GetChecked());
      break;
    case IDC_SHADOWMAPS_ZEPSILON: {
      float value =
          g_SampleUI.GetSlider(IDC_SHADOWMAPS_ZEPSILON)->GetValue() / 10000.0f;
      StringCchPrintf(sz, 100, L"Z epsilon: %.4f", value);
      g_SampleUI.GetStatic(IDC_SHADOWMAPS_ZEPSILON_S)->SetText(sz);
      g_pfZEpsilon->SetFloat(value);
      break;
    }
    case IDC_SHADOWMAPS_PCF:
      g_pbPCF->SetBool(g_SampleUI.GetCheckBox(IDC_SHADOWMAPS_PCF)->GetChecked());
      break;
    case IDC_CAMERA_SPEED: {
      float value =
          g_SampleUI.GetSlider(IDC_CAMERA_SPEED)->GetValue() / 100.0f;
      StringCchPrintf(sz, 100, L"Camera speed: %.2f", value);
      g_SampleUI.GetStatic(IDC_CAMERA_SPEED_S)->SetText(sz);
      g_Camera.SetScalers(0.01f, value);
      break;
    }
    case IDC_FLY_MODE: {
      if (g_SampleUI.GetCheckBox(IDC_FLY_MODE)->GetChecked()) {
        g_pScene->SetMovement(SCENE_MOVEMENT_FLY);
      } else {
        g_pScene->SetMovement(SCENE_MOVEMENT_WALK);
      }
      break;
    }
    case IDC_SCREEN_ERROR: {
      float value =
          g_SampleUI.GetSlider(IDC_SCREEN_ERROR)->GetValue() / 10.0f;
      StringCchPrintf(sz, 100, L"Screen error: %.1f", value);
      g_SampleUI.GetStatic(IDC_SCREEN_ERROR_S)->SetText(sz);
      g_fScreenError = value;
      SAFE_DELETE(g_pLODSelector);
      g_pLODSelector = new DynamicLODSelector(g_fFOV, g_uiScreenHeight, g_fScreenError);
      if (g_pScene) g_pScene->SetLODSelector(g_pLODSelector);
      break;
    }
    case IDC_POINT_EMITTER:
      g_bPointEmitter = g_SampleUI.GetCheckBox(IDC_POINT_EMITTER)->GetChecked();
      break;
    case IDC_BOX_EMITTER:
      g_bBoxEmitter = g_SampleUI.GetCheckBox(IDC_BOX_EMITTER)->GetChecked();
      break;

    case IDC_HDR_ENABLED:
      g_bHDRenabled = g_SampleUI.GetCheckBox(IDC_HDR_ENABLED)->GetChecked();
      break;
    case IDC_DOF_ENABLED:
      g_bDOFenabled = g_SampleUI.GetCheckBox(IDC_DOF_ENABLED)->GetChecked();
      break;
    case IDC_MOTIONBLUR_ENABLED:
      g_bMotionBlurenabled = g_SampleUI.GetCheckBox(IDC_MOTIONBLUR_ENABLED)->GetChecked();
      break;  
  
    /**
     * Terrain UI
     */
    case IDC_NEWTERRAIN_SIZE: {
      int value =
          (1 << g_TerrainUI.GetSlider(IDC_NEWTERRAIN_SIZE)->GetValue()) + 1;
      StringCchPrintf(sz, 100, L"Size: %dx%d", value, value);
      g_TerrainUI.GetStatic(IDC_NEWTERRAIN_SIZE_S)->SetText(sz);
      break;
    }
    case IDC_NEWTERRAIN_ROUGHNESS: {
      float value =
          g_TerrainUI.GetSlider(IDC_NEWTERRAIN_ROUGHNESS)->GetValue() / 100.0f;
      StringCchPrintf(sz, 100, L"Roughness: %.2f", value);
      g_TerrainUI.GetStatic(IDC_NEWTERRAIN_ROUGHNESS_S)->SetText(sz);
      break;
    }
    case IDC_NEWTERRAIN_LOD: {
      int value = g_TerrainUI.GetSlider(IDC_NEWTERRAIN_LOD)->GetValue();
      StringCchPrintf(sz, 100, L"LOD Levels: %d", value);
      g_TerrainUI.GetStatic(IDC_NEWTERRAIN_LOD_S)->SetText(sz);
      break;
    }
    case IDC_NEWTERRAIN_SCALE: {
      float value =
          g_TerrainUI.GetSlider(IDC_NEWTERRAIN_SCALE)->GetValue() / 10.0f;
      StringCchPrintf(sz, 100, L"Scale: %.1f", value);
      g_TerrainUI.GetStatic(IDC_NEWTERRAIN_SCALE_S)->SetText(sz);
      break;
    }
    case IDC_NEWTERRAIN_OK: {
      g_TerrainUI.SetVisible(false);

      g_nTerrainN = g_TerrainUI.GetSlider(IDC_NEWTERRAIN_SIZE)->GetValue();
      g_fTerrainR =
          g_TerrainUI.GetSlider(IDC_NEWTERRAIN_ROUGHNESS)->GetValue()/100.0f;
      g_nTerrainLOD = g_TerrainUI.GetSlider(IDC_NEWTERRAIN_LOD)->GetValue();
      g_fTerrainScale =
          g_TerrainUI.GetSlider(IDC_NEWTERRAIN_SCALE)->GetValue() / 10.0f;

      g_pScene->CreateTerrain(g_nTerrainN, g_fTerrainR, g_nTerrainLOD, g_fTerrainScale);
      Terrain *terrain = g_pScene->GetTerrain();
      g_pfMinHeight->SetFloat(terrain->GetMinHeight());
      g_pfMaxHeight->SetFloat(terrain->GetMaxHeight());

      ResetVolcano();
      MakeItRain();

      break;
    }
  }
}



void DrawFullscreenQuad( ID3D10Device* pd3dDevice, ID3D10EffectTechnique* pTech, UINT Width, UINT Height )
{
    // Save the Old viewport
    D3D10_VIEWPORT vpOld[D3D10_VIEWPORT_AND_SCISSORRECT_MAX_INDEX];
    UINT nViewPorts = 1;
    pd3dDevice->RSGetViewports( &nViewPorts, vpOld );

    // Setup the viewport to match the backbuffer
    D3D10_VIEWPORT vp;
    vp.Width = Width;
    vp.Height = Height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    pd3dDevice->RSSetViewports( 1, &vp );


    UINT strides = sizeof( SCREEN_VERTEX );
    UINT offsets = 0;
    ID3D10Buffer* pBuffers[1] = { g_pScreenQuadVB };

    pd3dDevice->IASetInputLayout( g_pQuadLayout );
    pd3dDevice->IASetVertexBuffers( 0, 1, pBuffers, &strides, &offsets );
    pd3dDevice->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );

    D3D10_TECHNIQUE_DESC techDesc;
    pTech->GetDesc( &techDesc );

    for( UINT uiPass = 0; uiPass < techDesc.Passes; uiPass++ )
    {
        pTech->GetPassByIndex( uiPass )->Apply( 0 );

        pd3dDevice->Draw( 4, 0 );
    }

    // Restore the Old viewport
    pd3dDevice->RSSetViewports( nViewPorts, vpOld );
}
