#include "stdafx.h"
#include "Tile.h"

int _tmain(int argc, _TCHAR* argv[])
{
  Tile t(8, 1.f);
  t.saveImage(L"Terrain.bmp");

  return 0;
}
