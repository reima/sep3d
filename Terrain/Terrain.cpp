#include "stdafx.h"
#include "Tile.h"

int _tmain(int argc, _TCHAR* argv[])
{
  Tile t(8, 0.5f);
  t.saveImage(L"Terrain.bmp");
  t.triangulateZOrder();
  t.saveObj(L"Terrain.obj");

  return 0;
}
