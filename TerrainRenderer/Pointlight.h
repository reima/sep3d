#pragma once
#include "LightSource.h"

class PointLight : public LightSource {
 public:
  /**
   * Konstruktor.
   * Erzeugt eine neue Punkt-Lichtquelle an einer bestimmten Startposition,
   * mit bestimmter Farbe und Rotationsgeschwindigkeit.
   */
  PointLight(const D3DXVECTOR3 &position, const D3DXVECTOR3 &color,
             const D3DXVECTOR3 &rotation);
  virtual ~PointLight(void);
  static void GetHandles(ID3D10Effect *effect);
  virtual void OnFrameMove(float elapsed_time);

 protected:
  D3DXVECTOR3 position_;

 private:
  static unsigned int instance_count;
  static ID3D10EffectVectorVariable *pPos;
  static ID3D10EffectVectorVariable *pColor;
  static ID3D10EffectScalarVariable *pNumPL;
};
