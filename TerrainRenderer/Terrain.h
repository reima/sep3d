#pragma once
#include "DXUT.h"
#include "DXUTCamera.h"

class Tile;
class LODSelector;
class CDXUTSDKMesh;

class Terrain {
 friend class Tile;

 public:
  /**
   * Konstruktor.
   * @param n Detaillevel, legt die Gr��e eines Tiles (2^n - 1) fest
   * @param roughness Rauheits-Faktor (je h�her desto gr��er die
   *                  H�henunterschiede)
   * @param num_lod Anzahl LOD-Ebenen
   */
  Terrain(int n, float roughness, int num_lod, float scale, bool water);
  ~Terrain(void);

  /**
   * Trianguliert streifenweise.
   */
  void TriangulateLines(void);

  /**
   * Trianguliert mit Z-Order.
   */
  void TriangulateZOrder(void);

  /**
   * Erzeugt Vertex- und Index-Buffer und l�dt das Dreiecksgitter in diese hoch.
   * Au�erdem werden die H�henkarten der Tiles erzeugt.
   * @warning Das Terrain muss zuvor trianguliert worden sein (durch Aufruf von
   *          Terrain::TriangulateLines oder Terrain::TriangulateZOrder).
   * @see Terrain::ReleaseBuffers
   */
  HRESULT CreateBuffers(ID3D10Device *device);

  void GetShaderHandles(ID3D10Effect *effect);

  /**
   * Gibt die von CreateBuffers erzeugten Buffer wieder frei.
   */
  void ReleaseBuffers(void);

  void GetBoundingBox(D3DXVECTOR3 *out, D3DXVECTOR3 *mid) const;
  void Draw(ID3D10EffectTechnique *technique, LODSelector *lod_selector,
            const CBaseCamera *camera, bool shadow_pass=false);
  void DrawVegetation(bool shadow_pass=false);

  /**
   * Ermittelt die minimale H�he im Terrain und gibt sie zur�ck.
   */
  float GetMinHeight(void) const;

  /**
   * Ermittelt die maximale H�he im Terrain und gibt sie zur�ck.
   */
  float GetMaxHeight(void) const;

  D3DXVECTOR3 GetHighestPoint(void) const;
  float GetHeightAt(const D3DXVECTOR3 &pos) const;

  int GetNumTrees(void) const { return num_trees_[0] + num_trees_[1]; }

 private:
  // Kopierkonstruktor und Zuweisungsoperator verbieten.
  Terrain(const Terrain &t);
  void operator=(const Terrain &t);

  void DrawTile(float scale, D3DXVECTOR2 &translate, UINT lod,
                ID3D10ShaderResourceView *srv);
  void DrawMesh(int num=0, bool shadow_pass=false);

  /**
   * Reserviert Speicher f�r den Index Buffer.
   */
  void InitIndexBuffer(void);

  /**
   * Rekursive Implementierung der Z-Order-Triangulierung.
   */
  void TriangulateZOrder0(int x1, int y1, int x2, int y2, int &i);

  void InitMeshes(void);
  HRESULT InitTrees(void);
  void InitVegetation(void);

  /**
   * Zeiger auf das Wurzel-Tile
   */
  Tile *tile_;
  /**
   * Gr��e eines Tiles (Seitenl�nge)
   */
  const int size_;

  ID3D10Device *device_;
  ID3D10InputLayout* vertex_layout_;
  /**
   * Zeiger auf den D3D10-Vertex-Buffer.
   */
  ID3D10Buffer *vertex_buffer_;
  /**
   * Zeiger auf den D3D10-Index-Buffer.
   */
  ID3D10Buffer *index_buffer_;
  ID3D10Buffer *tree_buffer_; // B�ume Transformationsmatrizen
  UINT num_trees_[2];
  UINT tree_offset_[2];

  ID3D10EffectScalarVariable *tile_scale_ev_;
  ID3D10EffectVectorVariable *tile_translate_ev_;
  ID3D10EffectScalarVariable *tile_lod_ev_;
  ID3D10EffectScalarVariable *terrain_size_ev_;
  ID3D10EffectShaderResourceVariable *tile_heightmap_ev_;

  ID3D10EffectTechnique *technique_;

  /**
   * Indizes f�r die Triangulierung des Terrains
   * @see Terrain::TriangulateLines
   * @see Terrain::TriangulateZOrder
   */
  unsigned int *indices_;

  CDXUTSDKMesh *mesh_[2];
  ID3D10InputLayout *mesh_vertex_layout_;
  ID3D10EffectShaderResourceVariable *mesh_texture_ev_;
  ID3D10ShaderResourceView *mesh_texture_srv_[2];
  ID3D10EffectPass *mesh_pass_;
  ID3D10EffectPass *mesh_shadow_pass_;
};
