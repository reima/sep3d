#include <vector>
#include "ShadowedDirectionalLight.h"
#include "LODSelector.h"
#include "Scene.h"
#include "DXUTCamera.h"
#include "Geom2D.h"

extern CFirstPersonCamera g_Camera;
extern bool g_bTSM;
extern float g_fTerrainScale;

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
  tts_ev_ =
      effect->GetVariableByName("g_mDirectionalTrapezoidToSquare")->AsMatrix();
  shadow_map_ev_ =
      effect->GetVariableByName("g_tDirectionalShadowMap")->AsShaderResource();
  technique_ = effect->GetTechniqueByName("DirectionalShadowMap");
}

void ShadowedDirectionalLight::SetShaderVariables(void) {
  assert(shadowed_idx_ev_ != NULL);
  assert(lst_ev_ != NULL);
  shadowed_idx_ev_->SetInt(instance_id_);
  lst_ev_->SetMatrix(light_space_transform_);
  tts_ev_->SetMatrix(trapezoid_to_square_);
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
  //
  // Calculate light space transform
  //
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

void ShadowedDirectionalLight::TSM_UpdateMatrices(void) {
  UpdateMatrices();

  //
  // Calculate view frustum in light space
  //

  // Frustum in NDC
  D3DXVECTOR3 frustum[] = {
    // Near plane
    D3DXVECTOR3(-1, -1, 0),
    D3DXVECTOR3(-1,  1, 0),
    D3DXVECTOR3( 1,  1, 0),
    D3DXVECTOR3( 1, -1, 0),
    // Far plane
    D3DXVECTOR3(-1, -1, 1),
    D3DXVECTOR3(-1,  1, 1),
    D3DXVECTOR3( 1,  1, 1),
    D3DXVECTOR3( 1, -1, 1),
  };

  D3DXMATRIX view_proj_inv;
  view_proj_inv = *g_Camera.GetViewMatrix();
  D3DXMatrixMultiply(&view_proj_inv, &view_proj_inv, g_Camera.GetProjMatrix());
  D3DXMatrixInverse(&view_proj_inv, NULL, &view_proj_inv);

  D3DXMATRIX frustum_transform;
  D3DXMatrixMultiply(&frustum_transform, &view_proj_inv, &light_space_transform_);

  // Transform NDC frustum to World Space -> Light Space
  D3DXVec3TransformCoordArray(frustum, sizeof(D3DXVECTOR3),
                              frustum, sizeof(D3DXVECTOR3),
                              &frustum_transform, 8);

  //
  // Calculate 2D convex hull of frustum in light space
  //
  ConvexPolygon2D poly;
  for (int i = 0; i < 8; ++i) {
    poly.AddPoint(frustum[i]);
  }
  poly.MakeConvexHull();

  // Clip frustum to light space
  //poly.ClipToRect(D3DXVECTOR2(-1, -1), D3DXVECTOR2(1, 1));

  if (poly.GetPointCount() == 0) {
    D3DXMatrixIdentity(&trapezoid_to_square_);
    return;
  }

  D3DXVECTOR2 near_center = (frustum[0] + frustum[2]) * 0.5f;
  D3DXVECTOR2 far_center =  (frustum[4] + frustum[6]) * 0.5f;
  Line2D eye_line(near_center, far_center);
  Line2D eye_line_ortho = eye_line.OrthogonalLine();

  // Determine base and top line
  float min_dist, max_dist;
  D3DXVECTOR2 min_dist_point, max_dist_point;
  poly.FindExtremePoints(LineDistanceMetric2D(&eye_line_ortho),
                         &max_dist, &max_dist_point,
                         &min_dist, &min_dist_point);
  // min_dist and max_dist are now (directed) distances from near_center
  Line2D top_line = eye_line_ortho.ParallelThrough(min_dist_point);
  Line2D bottom_line = eye_line_ortho.ParallelThrough(max_dist_point);
  // Make sure top_line has minimal distance from near_center
  if (fabs(min_dist) > fabs(max_dist)) {
    top_line = eye_line_ortho.ParallelThrough(max_dist_point);
    bottom_line = eye_line_ortho.ParallelThrough(min_dist_point);
  }

  // Calculation of q according to paper
  float lambda = max_dist + (-min_dist);
  D3DXVECTOR2 temp = near_center - far_center;
  float delta = 0.05f * D3DXVec2Length(&temp) * g_fTerrainScale / 100.0f;
  float xi = -0.6f;
  float eta = lambda*delta*(1+xi)/(lambda-2*delta-lambda*xi);
  D3DXVECTOR2 q = eye_line.PointAt(-(max_dist + eta));

  // Calculate side lines
  float left_angle, right_angle;
  D3DXVECTOR2 left_point, right_point;
  poly.FindExtremePoints(LineAngleMetric2D(q, far_center),
                         &left_angle, &left_point,
                         &right_angle, &right_point);
  Line2D left_line(q, left_point);
  Line2D right_line(q, right_point);

  // Calculate trapezoid
  D3DXVECTOR2 t0 = right_line.Intersection(bottom_line);
  D3DXVECTOR2 t1 = left_line.Intersection(bottom_line);
  D3DXVECTOR2 t2 = left_line.Intersection(top_line);
  D3DXVECTOR2 t3 = right_line.Intersection(top_line);

  trapezoid_to_square_ = TSM_TrapezoidToSquare(t0, t1, t2, t3);
}

D3DXMATRIX ShadowedDirectionalLight::TSM_TrapezoidToSquare(const D3DXVECTOR2 &t0,
                                                     const D3DXVECTOR2 &t1,
                                                     const D3DXVECTOR2 &t2,
                                                     const D3DXVECTOR2 &t3) {
  // http://www.comp.nus.edu.sg/~tants/tsm/TSM_recipe.html
  D3DXMATRIX trapezoid_to_square;
  D3DXVECTOR4 vec4;
  D3DXMATRIX temp;
  // T_1
  D3DXVECTOR2 u = (t2 + t3) * 0.5f;
  D3DXMatrixTranslation(&trapezoid_to_square, -u.x, -u.y, 0);

  // R
  u = t2 - t3;
  D3DXVec2Normalize(&u, &u);
  D3DXMatrixIdentity(&temp);
  temp._11 = u.x;
  temp._21 = u.y;
  temp._12 = u.y;
  temp._22 = -u.x;
  D3DXMatrixMultiply(&trapezoid_to_square, &trapezoid_to_square, &temp);

  // T_2
  Line2D t0t3(t0, t3);
  Line2D t1t2(t1, t2);
  u = t0t3.Intersection(t1t2);
  D3DXVECTOR4 u4;
  D3DXVec2Transform(&u4, &u, &trapezoid_to_square);
  D3DXMatrixMultiply(&trapezoid_to_square, &trapezoid_to_square, D3DXMatrixTranslation(&temp, -u4.x, -u4.y, 0));

  // H
  u = (t0 + t1) * 0.5f;
  D3DXVec2Transform(&u4, &u, &trapezoid_to_square);
  D3DXMatrixIdentity(&temp);
  temp._21 = -u4.x/u4.y;
  D3DXMatrixMultiply(&trapezoid_to_square, &trapezoid_to_square, &temp);

  // S_1
  D3DXVec2Transform(&u4, &t2, &trapezoid_to_square);
  D3DXMatrixMultiply(&trapezoid_to_square, &trapezoid_to_square, D3DXMatrixScaling(&temp, 1/u4.x, 1/u4.y, 1));

  // N
  D3DXMatrixIdentity(&temp);
  temp._42 = 1.0f;
  temp._24 = 1.0f;
  temp._44 = 0.0f;
  D3DXMatrixMultiply(&trapezoid_to_square, &trapezoid_to_square, &temp);

  // T_3
  D3DXVECTOR4 v4;
  D3DXVec2Transform(&u4, &t0, &trapezoid_to_square);
  D3DXVec2Transform(&v4, &t2, &trapezoid_to_square);
  D3DXMatrixMultiply(&trapezoid_to_square, &trapezoid_to_square, D3DXMatrixTranslation(&temp, 0, -(u4.y/u4.w+v4.y/v4.w)*0.5f, 0));

  // S_2
  D3DXVec2Transform(&u4, &t0, &trapezoid_to_square);
  D3DXMatrixMultiply(&trapezoid_to_square, &trapezoid_to_square, D3DXMatrixScaling(&temp, 1, -u4.w/u4.y, 1));

  return trapezoid_to_square;
}

void ShadowedDirectionalLight::OnFrameMove(float elapsed_time) {
  assert(technique_ != NULL);
  assert(depth_stencil_view_ != NULL);
  assert(device_ != NULL);
  assert(scene_ != NULL);
  assert(shadow_map_ev_ != NULL);

  DirectionalLight::OnFrameMove(elapsed_time);
  if (g_bTSM) {
    TSM_UpdateMatrices();
  } else {
    UpdateMatrices();
    D3DXMatrixIdentity(&trapezoid_to_square_);
  }

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

  // Szene rendern
  scene_->Draw(technique_, true);

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
