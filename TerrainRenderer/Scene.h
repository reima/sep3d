#pragma once
#include <vector>
#include "DXUT.h"
#include "LightSource.h"

class CFirstPersonCamera;
class Tile;
class LODSelector;
class ShadowedDirectionalLight;
class ShadowedPointLight;

class Scene {
 public:
  /**
   * Konstruktor.
   * Erzeugt eine neue Szene mit den angegebenen Materialeigenschaften
   */
  Scene(void);
  /**
   * Destruktor.
   */
  ~Scene(void);

  void SetMaterial(float ambient, float diffuse, float specular,
                   float exponent);

  void SetCamera(CFirstPersonCamera *camera) { camera_ = camera; }
  CFirstPersonCamera *GetCamera(void) { return camera_; }

  void SetLODSelector(LODSelector *lod_selector) { lod_selector_ = lod_selector; }
  LODSelector *GetLODSelector(void) { return lod_selector_; }

  /**
   * Erzeugt eine neue Punkt-Lichtquelle in der Szene.
   */
  void AddPointLight(const D3DXVECTOR3 &position, const D3DXVECTOR3 &color,
                     const D3DXVECTOR3 &rotation, bool shadowed = false);
  /**
   * Erzeugt eine neue gerichtete Lichtquelle in der Szene.
   */
  void AddDirectionalLight(const D3DXVECTOR3 &direction,
                           const D3DXVECTOR3 &color,
                           const D3DXVECTOR3 &rotation,
                           bool shadowed = false);
  /**
   * Erzeugt eine neue Scheinwerfer-Lichtquelle in der Szene.
   */
  void AddSpotLight(const D3DXVECTOR3 &position, const D3DXVECTOR3 &direction,
                    const D3DXVECTOR3 &color, const D3DXVECTOR3 &rotation,
                    float cutoff_angle, float exponent);

  /**
   * Erzeugt ein neues Terrain mit den übergebenen Parametern und bereitet es auf
   * das Rendering vor.
   */
  void CreateTerrain(int n, float roughness, int num_lod);
  Tile *GetTerrain(void) { return tile_; }

  void GetBoundingBox(D3DXVECTOR3 *box, D3DXVECTOR3 *mid);

  /**
   * Führt Per-Frame-Updates in der Szene aus
   */
  void OnFrameMove(float elapsed_time, const D3DXVECTOR3 &cam_pos);

  HRESULT OnCreateDevice(ID3D10Device *device);
  void GetShaderHandles(ID3D10Effect* effect);
  void OnDestroyDevice(void);
  void SetShadowMapDimensions(UINT width, UINT height);
  void SetShadowMapPrecision(bool high_precision);

  void Draw(void);

 private:
   /**
   * Sämtliche Lichtquellen in der Szene
   */
  std::vector<LightSource *> light_sources_;
  ShadowedPointLight *shadowed_point_light_;
  ShadowedDirectionalLight *shadowed_directional_light_;
  UINT shadow_map_width_;
  UINT shadow_map_height_;
  bool shadow_map_high_precision_;
  /**
   * Kameraposition
   */
  D3DXVECTOR3 cam_pos_;

  CFirstPersonCamera *camera_;
  Tile *tile_;
  LODSelector *lod_selector_;

  ID3D10Device *device_;
  ID3D10Effect *effect_;

  ID3D10EffectVectorVariable *pMaterialParameters;
  ID3D10EffectVectorVariable *pCameraPosition;
  ID3D10EffectScalarVariable *pShadowedPointLight;
  ID3D10EffectScalarVariable *pShadowedDirectionalLight;
};
