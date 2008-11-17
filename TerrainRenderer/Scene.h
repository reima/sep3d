#pragma once
#include <vector>
#include "DXUT.h"
#include "LightSource.h"

class Scene {
 public:
  /**
   * Konstruktor.
   * Erzeugt eine neue Szene mit den angegebenen Materialeigenschaften
   */
  Scene(float ambient, float diffuse, float specular, float exponent);
  /**
   * Destruktor.
   */
  ~Scene(void);

  /**
   * Erzeugt eine neue Punkt-Lichtquelle in der Szene.
   */
  void AddPointLight(const D3DXVECTOR3 &position, const D3DXVECTOR3 &color,
                     const D3DXVECTOR3 &rotation);
  /**
   * Erzeugt eine neue gerichtete Lichtquelle in der Szene.
   */
  void AddDirectionalLight(const D3DXVECTOR3 &direction,
                           const D3DXVECTOR3 &color,
                           const D3DXVECTOR3 &rotation);
  /**
   * Erzeugt eine neue Scheinwerfer-Lichtquelle in der Szene.
   */
  void AddSpotLight(const D3DXVECTOR3 &position, const D3DXVECTOR3 &direction,
                    const D3DXVECTOR3 &color, const D3DXVECTOR3 &rotation,
                    float cutoff_angle, float exponent);

  /**
   * Führt Per-Frame-Updates in der Szene aus
   */
  void OnFrameMove(float elapsed_time, const D3DXVECTOR3 &cam_pos);
  void GetShaderHandles(ID3D10Effect* effect);

 private:
   /**
   * Sämtliche Lichtquellen in der Szene
   */
  std::vector<LightSource *> light_sources_;
  /**
   * Materialeigenschaft: Ambienter Koeffizient
   */
  float ambient_;
  /**
   * Materialeigenschaft: Diffuser Koeffizient
   */
  float diffuse_;
  /**
   * Materialeigenschaft: Spekularer Koeffizient
   */
  float specular_;
  /**
   * Materialeigenschaft: Spekularer Exponent
   */
  float exponent_;
  /**
   * Kameraposition
   */
  D3DXVECTOR3 cam_pos_;

  ID3D10EffectVectorVariable *pMaterialParameters;
  ID3D10EffectVectorVariable *pCameraPosition;
};
