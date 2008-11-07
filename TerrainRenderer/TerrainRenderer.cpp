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
#include "SDKmesh.h"
#include "resource.h"

#include "Tile.h"
#include "LODSelector.h"
#include "FixedLODSelector.h"


#define TERRAIN_N         8
#define TERRAIN_ROUGHNESS 1.0f
#define TERRAIN_NUM_LOD   3

//#define DEBUG_VS   // Uncomment this line to debug D3D9 vertex shaders 
//#define DEBUG_PS   // Uncomment this line to debug D3D9 pixel shaders 


//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
CFirstPersonCamera          g_Camera;               // A first person camera
CDXUTDialogResourceManager  g_DialogResourceManager; // manager for shared resources of dialogs
CD3DSettingsDlg             g_SettingsDlg;          // Device settings dialog
CDXUTTextHelper*            g_pTxtHelper = NULL;
CDXUTDialog                 g_HUD;                  // dialog for standard controls
CDXUTDialog                 g_SampleUI;             // dialog for sample specific controls

// Direct3D 9 resources
extern ID3DXFont*           g_pFont9;
extern ID3DXSprite*         g_pSprite9;


// Direct3D 10 resources
ID3DX10Font*                g_pFont10 = NULL;
ID3DX10Sprite*              g_pSprite10 = NULL;
ID3D10Effect*               g_pEffect10 = NULL;
ID3D10EffectTechnique*      g_pTechnique = NULL;
ID3D10InputLayout*          g_pVertexLayout = NULL;
ID3D10EffectMatrixVariable* g_pmWorldViewProj = NULL;
ID3D10EffectMatrixVariable* g_pmWorld = NULL;
ID3D10EffectScalarVariable* g_pfTime = NULL;
ID3D10EffectVectorVariable* g_pvCamPos = NULL;
ID3D10EffectShaderResourceVariable* g_ptWaves = NULL;
ID3D10ShaderResourceView*   g_pWavesRV = NULL;

Tile*                       g_pTile = NULL;
LODSelector*                g_pLODSelector = NULL;

bool                        g_bWireframe = false;
ID3D10RasterizerState*      g_pRSWireframe = NULL;

//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
#define IDC_TOGGLEFULLSCREEN        1
#define IDC_TOGGLEREF               2
#define IDC_CHANGEDEVICE            3
#define IDC_NEWTERRAIN              4
#define IDC_LODSLIDER               5
#define IDC_LODSLIDER_S             6
#define IDC_WIREFRAME               7
#define IDC_NEWTERRAIN_LOD          8
#define IDC_NEWTERRAIN_SIZE         9
#define IDC_NEWTERRAIN_ROUGHNESS    10
#define IDC_NEWTERRAIN_LOD_S        11
#define IDC_NEWTERRAIN_SIZE_S       12
#define IDC_NEWTERRAIN_ROUGHNESS_S  13
#define IDC_NEWTERRAIN_OK           14
#define IDC_TECHNIQUE               15

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

ID3D10Effect* LoadEffect(ID3D10Device* pd3dDevice,
                         const std::wstring& filename,
                         const D3D10_SHADER_MACRO *Shader_Macros = NULL,
                         const bool bDebugCompile = false);


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
  DXUTCreateDevice(true, 640, 480);
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

  g_HUD.SetCallback(OnGUIEvent);
  int iY = 10;
  g_HUD.AddButton(IDC_TOGGLEFULLSCREEN, L"Toggle full screen", 35, iY, 125, 22);
  g_HUD.AddButton(IDC_TOGGLEREF, L"Toggle REF (F3)", 35, iY += 24, 125, 22,
                  VK_F3);
  g_HUD.AddButton(IDC_CHANGEDEVICE, L"Change device (F2)", 35, iY += 24, 125,
                  22, VK_F2);

  g_HUD.AddButton(IDC_NEWTERRAIN, L"New Terrain...", 35, iY += 24, 125,
                  22, VK_F2);
  g_HUD.AddCheckBox(IDC_WIREFRAME, L"Wireframe (F5)", 35, iY += 24, 125, 22,
                    g_bWireframe, VK_F5);
  g_HUD.AddStatic(0, L"Technique:", 35, iY += 24, 125, 22);
  g_HUD.AddComboBox(IDC_TECHNIQUE, 35, iY += 24, 125, 22);
  g_HUD.GetComboBox(IDC_TECHNIQUE)->AddItem(L"Vertex Coloring", "VertexShaderColoring");
  g_HUD.GetComboBox(IDC_TECHNIQUE)->AddItem(L"Vertex Col. + Phong", "VertexShaderColoringPhong");
  g_HUD.GetComboBox(IDC_TECHNIQUE)->AddItem(L"Pixel Coloring", "PixelShaderColoring");
  g_HUD.GetComboBox(IDC_TECHNIQUE)->AddItem(L"Normal Coloring", "NormalColoring");
  g_HUD.GetComboBox(IDC_TECHNIQUE)->AddItem(L"Phong", "Phong");

  WCHAR sz[100];
  StringCchPrintf(sz, 100, L"LOD (+/-): %d", 0);
  g_HUD.AddStatic(IDC_LODSLIDER_S, sz, 35, iY += 24, 125, 22);
  g_HUD.AddSlider(IDC_LODSLIDER, 35, iY += 24, 125, 22, 0, TERRAIN_NUM_LOD, 0);
  
  iY = 10;
  StringCchPrintf(sz, 100, L"Size: %dx%d", (1<<TERRAIN_N)+1, (1<<TERRAIN_N)+1);
  g_HUD.AddStatic(IDC_NEWTERRAIN_SIZE_S, sz, -150, iY, 125, 22);
  g_HUD.AddSlider(IDC_NEWTERRAIN_SIZE, -150, iY += 24, 125, 22, 1, 10,
                  TERRAIN_N);
  g_HUD.GetStatic(IDC_NEWTERRAIN_SIZE_S)->SetVisible(false);
  g_HUD.GetSlider(IDC_NEWTERRAIN_SIZE)->SetVisible(false);

  StringCchPrintf(sz, 100, L"Roughness: %.2f", TERRAIN_ROUGHNESS);
  g_HUD.AddStatic(IDC_NEWTERRAIN_ROUGHNESS_S, sz, -150, iY += 24, 125, 22);
  g_HUD.AddSlider(IDC_NEWTERRAIN_ROUGHNESS, -150, iY += 24, 125, 22, 0, 1000,
                  (int)(TERRAIN_ROUGHNESS*100));
  g_HUD.GetStatic(IDC_NEWTERRAIN_ROUGHNESS_S)->SetVisible(false);
  g_HUD.GetSlider(IDC_NEWTERRAIN_ROUGHNESS)->SetVisible(false);

  StringCchPrintf(sz, 100, L"LOD Levels: %d", TERRAIN_NUM_LOD);
  g_HUD.AddStatic(IDC_NEWTERRAIN_LOD_S, sz, -150, iY += 24, 125, 22);
  g_HUD.AddSlider(IDC_NEWTERRAIN_LOD, -150, iY += 24, 125, 22, 0, 5,
                  TERRAIN_NUM_LOD);
  g_HUD.GetStatic(IDC_NEWTERRAIN_LOD_S)->SetVisible(false);
  g_HUD.GetSlider(IDC_NEWTERRAIN_LOD)->SetVisible(false);

  g_HUD.AddButton(IDC_NEWTERRAIN_OK, L"Generate", -150, iY += 24, 125, 22);
  g_HUD.GetButton(IDC_NEWTERRAIN_OK)->SetVisible(false);

  g_SampleUI.SetCallback(OnGUIEvent);
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

/**
 * Erzeugt ein neues Terrain mit den übergebenen Parametern und bereitet es auf
 * das Rendering vor.
 */
HRESULT CreateTerrain(ID3D10Device *pd3dDevice, int terrain_n = TERRAIN_N,
                      float terrain_roughness = TERRAIN_ROUGHNESS,
                      int terrain_num_lod = TERRAIN_NUM_LOD) {
  HRESULT hr;
  SAFE_DELETE(g_pTile);
  // Tile erzeugen
  g_pTile = new Tile(terrain_n, terrain_roughness, terrain_num_lod);
  g_pTile->TriangulateZOrder();
  g_pTile->CalculateNormals();
  // Tile-Daten in D3D10-Buffer speichern
  V_RETURN(g_pTile->CreateBuffers(pd3dDevice));
  // Lokale Tile-Daten freigeben
  g_pTile->FreeMemory();
  return S_OK;
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
  DWORD dwShaderFlags = D3D10_SHADER_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
  // Set the D3D10_SHADER_DEBUG flag to embed debug information in the shaders.
  // Setting this flag improves the shader debugging experience, but still allows 
  // the shaders to be optimized and to run exactly the way they will run in 
  // the release configuration of this program.
  dwShaderFlags |= D3D10_SHADER_DEBUG;
#endif
/*  V_RETURN(D3DX10CreateEffectFromFile(L"TerrainRenderer.fx", NULL, NULL,
                                      "fx_4_0", dwShaderFlags, 0, pd3dDevice,
                                      NULL, NULL, &g_pEffect10, NULL, NULL));*/
  g_pEffect10 = LoadEffect(pd3dDevice, L"TerrainRenderer.fx");
  g_pTechnique = g_pEffect10->GetTechniqueByIndex(0);

  // Load wave normal map
  V_RETURN(D3DX10CreateShaderResourceViewFromFile(pd3dDevice, L"waves.dds", NULL, NULL, &g_pWavesRV, NULL));

  // Get effects variables
  g_pmWorldViewProj =
      g_pEffect10->GetVariableByName("g_mWorldViewProjection")->AsMatrix();
  g_pmWorld = g_pEffect10->GetVariableByName("g_mWorld")->AsMatrix();
  g_pfTime = g_pEffect10->GetVariableByName("g_fTime")->AsScalar();
  g_pvCamPos = g_pEffect10->GetVariableByName("g_vCamPos")->AsVector();
  g_ptWaves = g_pEffect10->GetVariableByName("g_tWaves")->AsShaderResource();
  V_RETURN(g_ptWaves->SetResource(g_pWavesRV));

  // Setup the camera's view parameters
  D3DXVECTOR3 vecEye(0.0f, 5.0f, -5.0f);
  D3DXVECTOR3 vecAt (0.0f, 0.0f, -0.0f);
  g_Camera.SetViewParams(&vecEye, &vecAt);

  // Vertex Layout festlegen
  D3D10_INPUT_ELEMENT_DESC layout[] = {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
      D3D10_INPUT_PER_VERTEX_DATA, 0 },
    { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0,
      D3D10_INPUT_PER_VERTEX_DATA, 0 },
  };
  UINT num_elements = sizeof(layout) / sizeof(layout[0]);
  D3D10_PASS_DESC pass_desc;
  g_pTechnique->GetPassByIndex(0)->GetDesc(&pass_desc);
  V_RETURN(pd3dDevice->CreateInputLayout(layout, num_elements,
                                         pass_desc.pIAInputSignature,
                                         pass_desc.IAInputSignatureSize,
                                         &g_pVertexLayout));

  // Terrain erzeugen
  V_RETURN(CreateTerrain(pd3dDevice));

  // FixedLODSelector mit LOD-Stufe 0
  g_pLODSelector = new FixedLODSelector(0);

  // RasterizerState für Wireframe erzeugen
  D3D10_RASTERIZER_DESC rast_desc = {
    D3D10_FILL_WIREFRAME, D3D10_CULL_BACK,
    FALSE,
    0, 0.0f, 0.0f,
    TRUE, FALSE, FALSE, FALSE
  };
  V_RETURN(pd3dDevice->CreateRasterizerState(&rast_desc, &g_pRSWireframe));

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
  g_Camera.SetProjParams(D3DX_PI / 4, fAspectRatio, 0.1f, 1000.0f);

  g_HUD.SetLocation(pBackBufferSurfaceDesc->Width - 170, 0);
  g_HUD.SetSize(170, 170);
  g_SampleUI.SetLocation(pBackBufferSurfaceDesc->Width - 170,
                         pBackBufferSurfaceDesc->Height - 300);
  g_SampleUI.SetSize(170, 300);

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

  float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
  ID3D10RenderTargetView* pRTV = DXUTGetD3D10RenderTargetView();
  pd3dDevice->ClearRenderTargetView(pRTV, ClearColor);

  // Clear the depth stencil
  ID3D10DepthStencilView* pDSV = DXUTGetD3D10DepthStencilView();
  pd3dDevice->ClearDepthStencilView(pDSV, D3D10_CLEAR_DEPTH, 1.0, 0);

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
  g_pmWorld->SetMatrix((float*)&mWorld);
  g_pfTime->SetFloat((float)fTime);
  D3DXVECTOR3 cam_pos(*g_Camera.GetEyePt());
  g_pvCamPos->SetFloatVector(cam_pos);

  // Set vertex Layout
  pd3dDevice->IASetInputLayout(g_pVertexLayout);

  if (g_bWireframe) pd3dDevice->RSSetState(g_pRSWireframe);

  D3D10_TECHNIQUE_DESC tech_desc;
  g_pTechnique->GetDesc(&tech_desc);
  for (UINT p = 0; p < tech_desc.Passes; ++p) {
    g_pTechnique->GetPassByIndex(p)->Apply(0);
    g_pTile->Draw(pd3dDevice, g_pLODSelector, g_Camera.GetEyePt());
  }

  if (g_bWireframe) pd3dDevice->RSSetState(NULL);

  DXUT_BeginPerfEvent(DXUT_PERFEVENTCOLOR, L"HUD / Stats");
  RenderText();
  g_HUD.OnRender(fElapsedTime);
  g_SampleUI.OnRender(fElapsedTime);
  DXUT_EndPerfEvent();
}


//--------------------------------------------------------------------------------------
// Release D3D10 resources created in OnD3D10ResizedSwapChain 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D10ReleasingSwapChain(void* pUserContext) {
  g_DialogResourceManager.OnD3D10ReleasingSwapChain();
}


//--------------------------------------------------------------------------------------
// Release D3D10 resources created in OnD3D10CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D10DestroyDevice(void* pUserContext) {
  g_DialogResourceManager.OnD3D10DestroyDevice();
  g_SettingsDlg.OnD3D10DestroyDevice();
  SAFE_RELEASE(g_pFont10);
  SAFE_RELEASE(g_pEffect10);
  SAFE_RELEASE(g_pVertexLayout);
  SAFE_RELEASE(g_pSprite10);
  SAFE_RELEASE(g_pRSWireframe);
  SAFE_RELEASE(g_pWavesRV);
  SAFE_DELETE(g_pTxtHelper);
  SAFE_DELETE(g_pTile);
  SAFE_DELETE(g_pLODSelector);
}


//--------------------------------------------------------------------------------------
// Called right before creating a D3D9 or D3D10 device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings(DXUTDeviceSettings* pDeviceSettings,
                                   void* pUserContext) {
  if(pDeviceSettings->ver == DXUT_D3D9_DEVICE) {
    IDirect3D9* pD3D = DXUTGetD3D9Object();
    D3DCAPS9 Caps;
    pD3D->GetDeviceCaps(pDeviceSettings->d3d9.AdapterOrdinal,
                        pDeviceSettings->d3d9.DeviceType, &Caps);

    // If device doesn't support HW T&L or doesn't support 1.1 vertex shaders in HW 
    // then switch to SWVP.
    if ((Caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) == 0 ||
        Caps.VertexShaderVersion < D3DVS_VERSION(1, 1)) {
      pDeviceSettings->d3d9.BehaviorFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
    }

    // Debugging vertex shaders requires either REF or software vertex processing 
    // and debugging pixel shaders requires REF.  
#ifdef DEBUG_VS
    if (pDeviceSettings->d3d9.DeviceType != D3DDEVTYPE_REF) {
      pDeviceSettings->d3d9.BehaviorFlags &=
          ~D3DCREATE_HARDWARE_VERTEXPROCESSING;
      pDeviceSettings->d3d9.BehaviorFlags &= ~D3DCREATE_PUREDEVICE;
      pDeviceSettings->d3d9.BehaviorFlags |=
          D3DCREATE_SOFTWARE_VERTEXPROCESSING;
    }
#endif
#ifdef DEBUG_PS
    pDeviceSettings->d3d9.DeviceType = D3DDEVTYPE_REF;
#endif
  }

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

  return true;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove(double fTime, float fElapsedTime,
                          void* pUserContext) {
  // Update the camera's position based on user input 
  g_Camera.FrameMove(fElapsedTime);
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

  // Pass all remaining windows messages to camera so it can respond to user input
  g_Camera.HandleMessages(hWnd, uMsg, wParam, lParam);

  return 0;
}

void SetLOD(int lod) {
  g_HUD.GetSlider(IDC_LODSLIDER)->SetValue(lod);
  // Wert neu auslesen, da er evtl. auf den gültigen Bereich geclampt wurde
  lod = g_HUD.GetSlider(IDC_LODSLIDER)->GetValue();
  SAFE_DELETE(g_pLODSelector);
  g_pLODSelector = new FixedLODSelector(lod);
  WCHAR sz[100];
  StringCchPrintf(sz, 100, L"LOD (+/-): %d", lod);
  g_HUD.GetStatic(IDC_LODSLIDER_S)->SetText(sz);  
}

//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard(UINT nChar, bool bKeyDown, bool bAltDown,
                         void* pUserContext) {
  if (!bKeyDown) return;
  switch (nChar) {
    case VK_ADD:
      SetLOD(g_HUD.GetSlider(IDC_LODSLIDER)->GetValue() + 1);
      break;
    case VK_SUBTRACT:
      SetLOD(g_HUD.GetSlider(IDC_LODSLIDER)->GetValue() - 1);
      break;
  }
}


//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent(UINT nEvent, int nControlID, CDXUTControl* pControl,
                         void* pUserContext) {
  switch (nControlID) {
    case IDC_TOGGLEFULLSCREEN:
      DXUTToggleFullScreen(); break;
    case IDC_TOGGLEREF:
      DXUTToggleREF(); break;
    case IDC_CHANGEDEVICE:
      g_SettingsDlg.SetActive(!g_SettingsDlg.IsActive()); break;
    case IDC_NEWTERRAIN:
      g_HUD.GetSlider(IDC_NEWTERRAIN_LOD)->SetVisible(true);
      g_HUD.GetSlider(IDC_NEWTERRAIN_SIZE)->SetVisible(true);
      g_HUD.GetSlider(IDC_NEWTERRAIN_ROUGHNESS)->SetVisible(true);
      g_HUD.GetStatic(IDC_NEWTERRAIN_LOD_S)->SetVisible(true);
      g_HUD.GetStatic(IDC_NEWTERRAIN_SIZE_S)->SetVisible(true);
      g_HUD.GetStatic(IDC_NEWTERRAIN_ROUGHNESS_S)->SetVisible(true);
      g_HUD.GetButton(IDC_NEWTERRAIN_OK)->SetVisible(true);
      break;
    case IDC_LODSLIDER:
      SetLOD(g_HUD.GetSlider(IDC_LODSLIDER)->GetValue());
      break;
    case IDC_WIREFRAME:
      g_bWireframe = g_HUD.GetCheckBox(IDC_WIREFRAME)->GetChecked();
      break;
    case IDC_NEWTERRAIN_SIZE: {
      int value = (1 << g_HUD.GetSlider(IDC_NEWTERRAIN_SIZE)->GetValue()) + 1;
      WCHAR sz[100];
      StringCchPrintf(sz, 100, L"Size: %dx%d", value, value);
      g_HUD.GetStatic(IDC_NEWTERRAIN_SIZE_S)->SetText(sz);
      break;
    }
    case IDC_NEWTERRAIN_ROUGHNESS: {
      float value =
          g_HUD.GetSlider(IDC_NEWTERRAIN_ROUGHNESS)->GetValue() / 100.0f;
      WCHAR sz[100];
      StringCchPrintf(sz, 100, L"Roughness: %.2f", value);
      g_HUD.GetStatic(IDC_NEWTERRAIN_ROUGHNESS_S)->SetText(sz);
      break;
    }
    case IDC_NEWTERRAIN_LOD: {
      int value = g_HUD.GetSlider(IDC_NEWTERRAIN_LOD)->GetValue();
      WCHAR sz[100];
      StringCchPrintf(sz, 100, L"LOD Levels: %d", value);
      g_HUD.GetStatic(IDC_NEWTERRAIN_LOD_S)->SetText(sz);
      break;
    }
    case IDC_NEWTERRAIN_OK: {
      g_HUD.GetSlider(IDC_NEWTERRAIN_LOD)->SetVisible(false);
      g_HUD.GetSlider(IDC_NEWTERRAIN_SIZE)->SetVisible(false);
      g_HUD.GetSlider(IDC_NEWTERRAIN_ROUGHNESS)->SetVisible(false);
      g_HUD.GetStatic(IDC_NEWTERRAIN_LOD_S)->SetVisible(false);
      g_HUD.GetStatic(IDC_NEWTERRAIN_SIZE_S)->SetVisible(false);
      g_HUD.GetStatic(IDC_NEWTERRAIN_ROUGHNESS_S)->SetVisible(false);
      g_HUD.GetButton(IDC_NEWTERRAIN_OK)->SetVisible(false);

      int n = g_HUD.GetSlider(IDC_NEWTERRAIN_SIZE)->GetValue();
      float roughness =
          g_HUD.GetSlider(IDC_NEWTERRAIN_ROUGHNESS)->GetValue()/100.0f;
      int num_lod = g_HUD.GetSlider(IDC_NEWTERRAIN_LOD)->GetValue();
    
      CreateTerrain(DXUTGetD3D10Device(), n, roughness, num_lod);

      g_HUD.GetSlider(IDC_LODSLIDER)->SetValue(0);
      g_HUD.GetSlider(IDC_LODSLIDER)->SetRange(0, num_lod);
      g_HUD.GetStatic(IDC_LODSLIDER_S)->SetText(L"LOD: 0");
      SAFE_DELETE(g_pLODSelector);
      g_pLODSelector = new FixedLODSelector(0);
      break;
    }
    case IDC_TECHNIQUE: {
      char *tech = (char *)g_HUD.GetComboBox(IDC_TECHNIQUE)->GetSelectedData();
      g_pTechnique = g_pEffect10->GetTechniqueByName(tech);
    }
  }
}
