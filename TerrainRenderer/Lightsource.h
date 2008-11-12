#pragma once

#include "DXUT.h"



class Lightsource
{
public:
  D3DXVECTOR3 Color;
  D3DXVECTOR3 Rotation;
  unsigned int ID;

  virtual void OnFrameMove(float fElapsedTime)=0;
};
