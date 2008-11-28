#include "Scene.h"
#include "PointLight.h"
#include "DirectionalLight.h"
#include "SpotLight.h"
#include "Tile.h"
#include "LODSelector.h"

extern LODSelector *g_pLODSelector;

Scene::Scene(void)
    : cam_pos_(D3DXVECTOR3(0, 0, 0)),
      tile_(NULL),
      lod_selector_(NULL),
      device_(NULL),
      pMaterialParameters(NULL),
      pCameraPosition(NULL) {
}

Scene::~Scene(void) {
  std::vector<LightSource *>::iterator it;
  for (it = light_sources_.begin(); it != light_sources_.end(); ++it) {
    delete (*it);
  }
  SAFE_DELETE(tile_);
}

void Scene::SetMaterial(float ambient, float diffuse, float specular,
                        float exponent) {
  assert(pMaterialParameters != NULL);
  float material_parameters[] = { ambient, diffuse, specular, exponent };
  pMaterialParameters->SetFloatVector(material_parameters);
}

void Scene::AddPointLight(const D3DXVECTOR3 &position,
                          const D3DXVECTOR3 &color,
                          const D3DXVECTOR3 &rotation) {
  light_sources_.push_back(new PointLight(position, color, rotation));
}

void Scene::AddDirectionalLight(const D3DXVECTOR3 &direction,
                                const D3DXVECTOR3 &color,
                                const D3DXVECTOR3 &rotation) {
  light_sources_.push_back(new DirectionalLight(direction, color, rotation));
}

void Scene::AddSpotLight(const D3DXVECTOR3 &position,
                         const D3DXVECTOR3 &direction,
                         const D3DXVECTOR3 &color,
                         const D3DXVECTOR3 &rotation,
                         float cutoff_angle, float exponent) {
  light_sources_.push_back(new SpotLight(position, direction, color, rotation,
                                         cutoff_angle, exponent));
}

void Scene::OnFrameMove(float elapsed_time, const D3DXVECTOR3 &cam_pos) {
  assert(pCameraPosition != NULL);
  cam_pos_ = cam_pos;
  pCameraPosition->SetFloatVector(cam_pos_);
  std::vector<LightSource *>::iterator it;
  for (it = light_sources_.begin(); it != light_sources_.end(); ++it) {
    (*it)->OnFrameMove(elapsed_time);
  }
}

void Scene::OnCreateDevice(ID3D10Device *device) {
  device_ = device;
}

void Scene::GetShaderHandles(ID3D10Effect* effect) {
  assert(effect != NULL);
  PointLight::GetHandles(effect);
  DirectionalLight::GetHandles(effect);
  SpotLight::GetHandles(effect);
  pMaterialParameters =
      effect->GetVariableByName("g_vMaterialParameters")->AsVector();  
  pCameraPosition =
      effect->GetVariableByName("g_vCamPos")->AsVector();
}

void Scene::CreateTerrain(int n, float roughness, int num_lod) {
  assert(device_ != NULL);
  HRESULT hr;
  SAFE_DELETE(tile_);
  tile_ = new Tile(n, roughness, num_lod);
  tile_->TriangulateZOrder();
  tile_->CalculateNormals();
  V(tile_->CreateBuffers(device_));
  tile_->FreeMemory();
}

void Scene::GetBoundingBox(D3DXVECTOR3 *box, D3DXVECTOR3 *mid) {
  if (tile_) {
    tile_->GetBoundingBox(box, mid);
  } else {
    for (int i = 0; i < 8; ++i) {
      box[i] = D3DXVECTOR3(0, 0, 0);      
    }
    *mid = D3DXVECTOR3(0, 0, 0);
  }
}

void Scene::Draw(void) {
  assert(device_ != NULL);
  assert(lod_selector_ != NULL);
  if (tile_) {
    tile_->Draw(device_, lod_selector_, &cam_pos_);
  }  
}