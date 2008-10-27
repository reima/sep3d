#include "stdafx.h"
#include "Tile.h"

int _tmain(int argc, _TCHAR* argv[])
{
  Tile t(8, 1.5f, 3);
  t.SaveImages(L"Terrain.png");
  t.TriangulateZOrder();
  t.SaveObj(L"Terrain.obj");

  return 0;
}
