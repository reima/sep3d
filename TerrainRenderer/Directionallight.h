#pragma once
#include "LightSource.h"

class DirectionalLight : public LightSource {
 public:
  /**
   * Konstruktor.
   * Erzeugt eine neue gerichtet Lichtquelle mit bestimmter Startrichtung,
   * Farbe und Rotationsgeschwindigkeit.
   */
  DirectionalLight(const D3DXVECTOR3 &direction, const D3DXVECTOR3 &color,
                   const D3DXVECTOR3 &rotation);
  virtual ~DirectionalLight(void);
  static void GetHandles(ID3D10Effect *effect);
  virtual void OnFrameMove(float elapsed_time);
  virtual void OnDestroyDevice(void);

 protected:
  D3DXVECTOR3 direction_;

 private:
  static unsigned int instance_count;
  static ID3D10EffectVectorVariable *pDir;
  static ID3D10EffectVectorVariable *pColor;
  static ID3D10EffectScalarVariable *pNumDL;
};
