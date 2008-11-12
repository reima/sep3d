#pragma once
#include "DXUT.h"
#include "Pointlight.h"

class Scene
{
public:
  void AddPointLight(D3DXVECTOR3 pos, D3DXVECTOR4 color);

  void OnFrameMove(float fTime);
  void GetShaderHandles(ID3D10Effect* pFx);



  Scene(void);
  ~Scene(void);

private:
 Pointlight *Pointlights[16];


};
