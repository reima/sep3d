#pragma once
#include "lightsource.h"

class Pointlight :
  public Lightsource
{
public:
  static unsigned int InstanceCount;
  D3DXVECTOR3 Position;
  static ID3D10EffectVectorVariable* pPos;
  static ID3D10EffectVectorVariable* pColor;
  static ID3D10EffectScalarVariable* pNumPL;
  virtual void OnFrameMove(float fElapsedTime);

  static void GetHandles(ID3D10Effect *pFx);

  Pointlight(D3DXVECTOR3 pos, D3DXVECTOR4 color);
  ~Pointlight(void);
};
