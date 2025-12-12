#pragma once
#include <ai/AI.h>
#include <queue>


void castRay(AI* pAI, int sx, int sy, int dx, int dy, int startVal, int decay);

void calculateMap(AI* pAI);
int getSmartMove(AI* pAI, int id);
