#pragma once
#include "DXUT.h"

class LightSource {
 public:
  /**
   * Konstruktor.
   * Erzeugt eine neue Lichtquelle mit bestimmter Farbe und Rotations-
   * geschwindigkeit.
   */
  LightSource(const D3DXVECTOR3 &color, const D3DXVECTOR3 &rotation)
      : color_(color), rotation_(rotation) {};
  virtual ~LightSource(void) {};
  /**
   * Führt Per-Frame-Updates an der Lichtquelle aus (Rotation).
   */
  virtual void OnFrameMove(float elapsed_time) = 0;
  virtual HRESULT OnCreateDevice(ID3D10Device *device) { return S_OK; };
  virtual void OnDestroyDevice(void) {};

 protected:
  D3DXVECTOR3 color_;
  D3DXVECTOR3 rotation_;
  unsigned int instance_id_;
};
