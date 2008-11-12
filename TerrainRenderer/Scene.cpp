#include "Scene.h"

void Scene::AddPointLight(D3DXVECTOR3 pos, D3DXVECTOR4 color){

  Pointlights[Pointlight::InstanceCount-1] = new Pointlight(pos, color);
}

void Scene::OnFrameMove(float fTime){
  
  for (int i = 0; i< Pointlight::InstanceCount; i++) Pointlights[i]->OnFrameMove(fTime);

}

void Scene::GetShaderHandles(ID3D10Effect* pFx){
  Pointlight::GetHandles(pFx);
}

Scene::Scene(void)
{
}

Scene::~Scene(void)
{
 
}
