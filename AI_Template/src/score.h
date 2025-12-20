#pragma once
#include <ai/AI.h>
#include <queue>

int trigger(AI* pAI, int id);
int castRay(AI* pAI, int sx, int sy, int dx, int dy, int startVal, int decay, bool tank=false);
int castRayCost(AI* pAI, int sx, int sy, int dx, int dy, int startVal, int decay);
void calculateMap(AI* pAI,int id);
int getSmartMove(AI* pAI, int id);
