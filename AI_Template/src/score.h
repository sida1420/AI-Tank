#pragma once
#include <ai/AI.h>
#include <queue>


void castRay(AI* pAI, int sx, int sy, int dx, int dy, int startVal, int decay);
int trigger_attack2(int id);
void calculateMap(AI* pAI);
int getSmartMove(AI* pAI, int id);
