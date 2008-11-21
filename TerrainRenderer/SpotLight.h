#pragma once
#include "LightSource.h"

class SpotLight : public LightSource {
 public:
  /**
   * Konstruktor.
   * Erzeugt eine neue Scheinwerfer-Lichtquelle an einer bestimmten Start-
   * position, mit bestimmter Richtung, Farbe und Rotationsgeschwindigkeit,
   * Öffnungswinkel und Abfall-Exponenten.
   */
  SpotLight(const D3DXVECTOR3 &position, const D3DXVECTOR3 &direction,
            const D3DXVECTOR3 &color, const D3DXVECTOR3 &rotation,
            float cutoff_angle, float exponent);
  static void GetHandles(ID3D10Effect *effect);
  virtual void OnFrameMove(float elapsed_time);

 private:
  D3DXVECTOR3 position_;
  D3DXVECTOR3 direction_;
  float cutoff_angle_;
  float exponent_;
  static unsigned int instance_count;
  static ID3D10EffectVectorVariable *pPos;
  static ID3D10EffectVectorVariable *pDir;
  static ID3D10EffectVectorVariable *pAngleExp;
  static ID3D10EffectVectorVariable *pColor;
  static ID3D10EffectScalarVariable *pNumSL;
};
