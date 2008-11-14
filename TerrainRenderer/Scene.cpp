#include "Scene.h"
#include "PointLight.h"
#include "DirectionalLight.h"
#include "SpotLight.h"

void Scene::AddPointLight(D3DXVECTOR3 &position, D3DXVECTOR3 &color) {
  light_sources_.push_back(new PointLight(position, color));
}

void Scene::AddDirectionalLight(D3DXVECTOR3 &direction, D3DXVECTOR3 &color) {
  light_sources_.push_back(new DirectionalLight(direction, color));
}

void Scene::AddSpotLight(D3DXVECTOR3 &position, D3DXVECTOR3 &direction,
                         D3DXVECTOR3 &color, float cutoff_angle, float exponent) {
  light_sources_.push_back(new SpotLight(position, direction, color, cutoff_angle, exponent));
}

void Scene::OnFrameMove(float fTime) {
  std::vector<LightSource *>::iterator it;
  for (it = light_sources_.begin(); it != light_sources_.end(); ++it) {
    (*it)->OnFrameMove(fTime);
  }
}

void Scene::GetShaderHandles(ID3D10Effect* pFx) {
  PointLight::GetHandles(pFx);
  DirectionalLight::GetHandles(pFx);
  SpotLight::GetHandles(pFx);
}

Scene::Scene(void) {
}

Scene::~Scene(void) {
  std::vector<LightSource *>::iterator it;
  for (it = light_sources_.begin(); it != light_sources_.end(); ++it) {
    delete (*it);
  }
}
