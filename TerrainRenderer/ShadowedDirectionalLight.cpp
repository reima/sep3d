#include "ShadowedDirectionalLight.h"
#include "LODSelector.h"
#include "Scene.h"
#include "DXUTCamera.h"

ShadowedDirectionalLight::ShadowedDirectionalLight(
    const D3DXVECTOR3 &direction,
    const D3DXVECTOR3 &color,
    const D3DXVECTOR3 &rotation,
    Scene *scene,
    UINT map_width,
    UINT map_height,
    bool high_precision)
  : DirectionalLight(direction, color, rotation),
    map_width_(map_width),
    map_height_(map_height),
    high_precision_(high_precision),
    scene_(scene),
    device_(NULL),
    shadow_map_(NULL),
    depth_stencil_view_(NULL),
    shader_resource_view_(NULL),
    technique_(NULL),
    shadowed_idx_ev_(NULL),
    lst_ev_(NULL),
    shadow_map_ev_(NULL) {
}

ShadowedDirectionalLight::~ShadowedDirectionalLight(void) {
  OnDestroyDevice();
}

HRESULT ShadowedDirectionalLight::OnCreateDevice(ID3D10Device *device) {
  HRESULT hr;
  device_ = device;

  D3D10_TEXTURE2D_DESC tex2d_desc;
  tex2d_desc.Width = map_width_;
  tex2d_desc.Height = map_height_;
  tex2d_desc.MipLevels = 1;
  tex2d_desc.ArraySize = 1;
  tex2d_desc.Format = high_precision_ ? DXGI_FORMAT_R32_TYPELESS
                                      : DXGI_FORMAT_R16_TYPELESS;
  tex2d_desc.SampleDesc.Count = 1;
  tex2d_desc.SampleDesc.Quality = 0;
  tex2d_desc.Usage = D3D10_USAGE_DEFAULT;
  tex2d_desc.BindFlags = D3D10_BIND_DEPTH_STENCIL | D3D10_BIND_SHADER_RESOURCE;
  tex2d_desc.CPUAccessFlags = 0;
  tex2d_desc.MiscFlags = 0;
  V_RETURN(device->CreateTexture2D(&tex2d_desc, NULL, &shadow_map_));

  D3D10_DEPTH_STENCIL_VIEW_DESC dsv_desc;
  dsv_desc.Format = high_precision_ ? DXGI_FORMAT_D32_FLOAT
                                    : DXGI_FORMAT_D16_UNORM;
  dsv_desc.ViewDimension = D3D10_DSV_DIMENSION_TEXTURE2D;
  dsv_desc.Texture2D.MipSlice = 0;
  V_RETURN(device->CreateDepthStencilView(shadow_map_, &dsv_desc,
                                          &depth_stencil_view_));

  D3D10_SHADER_RESOURCE_VIEW_DESC srv_desc;
  srv_desc.Format = high_precision_ ? DXGI_FORMAT_R32_FLOAT
                                    : DXGI_FORMAT_R16_FLOAT;
  srv_desc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
  srv_desc.Texture2D.MipLevels = 1;
  srv_desc.Texture2D.MostDetailedMip = 0;
  V_RETURN(device->CreateShaderResourceView(shadow_map_, &srv_desc,
                                            &shader_resource_view_));
  return S_OK;
}

void ShadowedDirectionalLight::OnDestroyDevice(void) {
  SAFE_RELEASE(shader_resource_view_);
  SAFE_RELEASE(depth_stencil_view_);
  SAFE_RELEASE(shadow_map_);
}

void ShadowedDirectionalLight::GetShaderHandles(ID3D10Effect *effect) {
  DirectionalLight::GetHandles(effect);
  shadowed_idx_ev_ =
      effect->GetVariableByName("g_iShadowedDirectionalLight")->AsScalar();
  lst_ev_ =
      effect->GetVariableByName("g_mDirectionalLightSpaceTransform")->AsMatrix();
  shadow_map_ev_ =
      effect->GetVariableByName("g_tDirectionalShadowMap")->AsShaderResource();
  technique_ = effect->GetTechniqueByName("DirectionalShadowMap");
}

void ShadowedDirectionalLight::SetShaderVariables(void) {
  assert(shadowed_idx_ev_ != NULL);
  assert(lst_ev_ != NULL);
  shadowed_idx_ev_->SetInt(instance_id_);
  lst_ev_->SetMatrix(light_space_transform_);
}

void ShadowedDirectionalLight::SetShadowMapDimensions(UINT width, UINT height) {
  map_width_ = width;
  map_height_ = height;
  OnDestroyDevice();
  OnCreateDevice(device_);
}

void ShadowedDirectionalLight::SetShadowMapPrecision(bool high_precision) {
  high_precision_ = high_precision;
  OnDestroyDevice();
  OnCreateDevice(device_);
}

void ShadowedDirectionalLight::UpdateMatrices(void) {
  D3DXVECTOR3 box[8];
  D3DXVECTOR3 mid;

  scene_->GetBoundingBox(box, &mid);
  D3DXMATRIX view;

  D3DXVECTOR3 lookat = mid - direction_;
  D3DXVECTOR3 upvec = D3DXVECTOR3(0, 1, 0);

  D3DXMatrixLookAtLH(&view, &mid, &lookat, &upvec);

  D3DXVec3TransformCoordArray(box, sizeof(D3DXVECTOR3),
                              box, sizeof(D3DXVECTOR3),
                              &view, 8);

  D3DXVECTOR3 min_coords = box[0];
  D3DXVECTOR3 max_coords = box[0];
  for (int i = 1; i < 8; ++i) {
    D3DXVec3Minimize(&min_coords, &min_coords, &box[i]);
    D3DXVec3Maximize(&max_coords, &max_coords, &box[i]);
  }
  

  D3DXMATRIX proj;
  D3DXMatrixOrthoOffCenterLH(&proj,
                             min_coords.x,
                             max_coords.x,
                             min_coords.y,
                             max_coords.y,
                             min_coords.z,
                             max_coords.z);

  light_space_transform_ = view * proj;
}

void ShadowedDirectionalLight::OnFrameMove(float elapsed_time) {
  assert(technique_ != NULL);
  assert(depth_stencil_view_ != NULL);
  assert(device_ != NULL);
  assert(scene_ != NULL);
  assert(shadow_map_ev_ != NULL);

  DirectionalLight::OnFrameMove(elapsed_time);
  UpdateMatrices();
  SetShaderVariables();

  // Render Targets sichern
  ID3D10RenderTargetView *rtv_old[D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT];
  ID3D10DepthStencilView *dsv_old;
  device_->OMGetRenderTargets(D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT,
                              rtv_old, &dsv_old);

  // Viewports sichern
  D3D10_VIEWPORT viewports_old[D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT];
  UINT num_viewports = D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT;
  device_->RSGetViewports(&num_viewports, viewports_old);

  // Shader Resource ausbinden
  // DEVICE_OMSETRENDERTARGETS_HAZARD tritt aber trotzdem auf :(
  shadow_map_ev_->SetResource(NULL);

  // Unsere Textur als Depth-Stencil-Target setzen
  device_->OMSetRenderTargets(0, NULL, depth_stencil_view_);
  // Inhalt zurücksetzen
  device_->ClearDepthStencilView(depth_stencil_view_, D3D10_CLEAR_DEPTH, 1.0f, 0);

  // Viewport setzen
  D3D10_VIEWPORT viewport;
  viewport.TopLeftX = 0;
  viewport.TopLeftY = 0;
  viewport.Width = map_width_;
  viewport.Height = map_height_;
  viewport.MaxDepth = 1.0f;
  viewport.MinDepth = 0.0f;
  device_->RSSetViewports(1, &viewport);

  scene_->Draw(technique_);

  // Alte Render Targets wieder setzen
  device_->OMSetRenderTargets(D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT,
                              rtv_old, dsv_old);

  // Alte Viewports wieder setzen
  device_->RSSetViewports(num_viewports, viewports_old);

  // Referenzen auf alte Render Targets freigeben
  for (UINT i = 0; i < D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
    SAFE_RELEASE(rtv_old[i]);

  SAFE_RELEASE(dsv_old);

  shadow_map_ev_->SetResource(shader_resource_view_);
}
