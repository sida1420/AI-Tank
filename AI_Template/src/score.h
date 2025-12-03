#pragma once
#include <ai/AI.h>
#include <queue>


void resetScore(AI* pAI);

void line(AI* pAI, int sx, int sy, int ex, int ey, int change);

void cross(AI* pAI, int sx, int sy, bool ho, bool ve, int change);

void targetInline(AI* pAI);
void allySpot(AI* pAI);

void bulletInline(AI* pAI);

int move(AI* pAI, Tank* tank);