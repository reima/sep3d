#include "stdafx.h"
#include "Tile.h"

int _tmain(int argc, _TCHAR* argv[])
{
  Tile t(8, 0.5f, 3);
  t.SaveImage(L"Terrain.png");

  return 0;
}
