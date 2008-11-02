#pragma once
#include "LODSelector.h"

/**
 * LOD-Auswahl-Strategie, die nur eine fixe LOD-Stufe akzeptiert.
 */
class FixedLODSelector : public LODSelector {
 public:
  /**
   * Konstruktor.
   * @param lod Die zu akzeptierende LOD-Stufe.
   */
  FixedLODSelector(int lod);  
  virtual bool IsLODSufficient(const Tile *tile,
                               const D3DXVECTOR3 *cam_pos) const;
 private:
  void operator=(const FixedLODSelector &);

  /**
   * LOD-Stufe, die diese Instanz akzeptiert.
   */
  const int lod_;
};
