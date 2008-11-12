#pragma once
#include "lightsource.h"

class Directionallight :
  public Lightsource
{
public:
  static unsigned int InstanceCount;
  D3DXVECTOR3 Direction;
  ID3D10EffectVectorVariable* pDir;
  static ID3D10EffectScalarVariable* pNumDL;

  virtual void OnFrameMove(float fElapsedTime)=0;
  static void GetHandles(ID3D10Effect *pFx);

  Directionallight(void);
  ~Directionallight(void);
};
