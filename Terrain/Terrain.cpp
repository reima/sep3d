#include "stdafx.h"
#include "Tile.h"

int _tmain(int argc, _TCHAR* argv[])
{
  Tile t(2, 1.f);
  t.saveImage(L"Terrain-.bmp");
  t.triangulate_z();
  return 0;
}
