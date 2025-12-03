
#include "score.h"

int map[22][22];


void resetScore(AI* pAI) {
	
	for (int y = 0; y < 22; y++) {
		for (int x = 0; x < 22; x++) {
			int t = 0;
			switch (pAI->GetBlock(x,y))
			{
			case BLOCK_HARD_OBSTACLE:
				t = INT_MIN;
				break;
			case BLOCK_WATER:
				t = INT_MIN;
				break;
			case BLOCK_SOFT_OBSTACLE:
				t = INT_MIN;
				break;
			default:
				t = 0;
				break;
			}
			map[x][y] = t;
		}
	}
}

bool passable(AI* pAI, float x, float y) {
	for (int i = floor(x); i <= ceil(x); i++) {
		for (int j = floor(y); j <= ceil(y); j++) {
			switch (pAI->GetBlock(i, j))
			{
			case BLOCK_HARD_OBSTACLE:
				return false;
			case BLOCK_WATER:
				return false;
			case BLOCK_SOFT_OBSTACLE:
				return false;
			case BLOCK_OUT_OF_BOARD:
				return false;
			default:
				break;
			}

		}
	}
	return true;
}

bool shootable(AI* pAI, float x, float y) {
	for (int i = floor(x); i <= ceil(x); i++) {
		for (int j = floor(y); j <= ceil(y); j++) {
			switch (pAI->GetBlock(i, j))
			{
			case BLOCK_HARD_OBSTACLE:
				return false;
			case BLOCK_SOFT_OBSTACLE:
				return false;
			case BLOCK_OUT_OF_BOARD:
				return false;
			default:
				break;
			}

		}
	}
	return true;
}

void line(AI* pAI, int sx, int sy, int ex, int ey, int change)
{
	if (sx == ex) {
		int dir = sy < ey ? 1 : -1;

		for (int y = sy; y <= ey; y += dir) {
			if (!shootable(pAI,sx,y)) {
				break;
			}
			map[sx][y] += change;
		}
	}
	else {
		int dir = sx < ex ? 1 : -1;

		for (int x = sx; x <= ex; x += dir) {
			if (!shootable(pAI, x, sy)) break;
			map[x][sy] += change;
		}
	}
}



void cross(AI* pAI, int sx, int sy, bool ho, bool ve, int change) {
	//improve
	if (ho) {
		line(pAI, sx+1, sy, 21, sy, change);
		line(pAI, sx, sy, 0, sy, change);
	}
	if (ve) {
		line(pAI, sx, sy+1, sx, 21, change);
		line(pAI, sx, sy, sx, 0, change);
	}

}
std::pair<int, int> nextFrame(int x, int y, float velocity, int dir) {
	switch (dir)
	{
	case 1:
		return { x,y - std::ceil(velocity) };
	case 2:
		return { x + std::ceil(velocity)  ,y };
	case 3:
		return { x,y + std::ceil(velocity) };

	default:
		return { x - std::ceil(velocity ) ,y };
	}
}

void targetInline(AI* pAI)
{
	for (int i = 0; i < NUMBER_OF_TANK; i++) {
		Tank* tempTank = pAI->GetEnemyTank(i);
		//don't waste effort if tank's death
		if ((tempTank == NULL) || (tempTank->GetHP() == 0))
			continue;

		std::pair<int, int> t = nextFrame(tempTank->GetX(), tempTank->GetY(), tempTank->GetSpeed(), tempTank->GetDirection());
		cross(pAI, t.first, t.second,true,true, 100);
		//more base on speed

	}
	std::vector<Base*> bases = pAI->GetEnemyBases();

	//for (int i = 0; i < bases.size(); i++) {
	//
		//cross(pAI, bases[i]->GetX(), bases[i]->GetY(), true,true,20);

	//}
}

void allySpot(AI* pAI)
{
	for (int i = 0; i < NUMBER_OF_TANK; i++) {
		Tank* tempTank = pAI->GetMyTank(i);
		//don't waste effort if tank's death
		if ((tempTank == NULL) || (tempTank->GetHP() == 0))
			continue;

		map[(int)tempTank->GetX()][(int)tempTank->GetY()] = -50;

	}	
}


void bulletInline(AI* pAI)
{
	std::vector<Bullet*> bullets = pAI->GetEnemyBullets();

	for (int i = 0; i < bullets.size(); i++) {
		line(pAI, bullets[i]->GetX(), bullets[i]->GetY(),
			nextFrame(bullets[i]->GetX(), bullets[i]->GetY(), bullets[i]->GetSpeed()+1, bullets[i]->GetDirection()).first,
			nextFrame(bullets[i]->GetX(), bullets[i]->GetY(), bullets[i]->GetSpeed()+1, bullets[i]->GetDirection()).second,
			-10 * bullets[i]->GetDamage());
	}
}


int cost(float sx, float sy, float ex, float ey) {
	return 5*(std::abs(sx - ex) + std::abs(sy- ey));
}

int move(AI* pAI, Tank* tank)
{
	int searchLimit = 100;
	std::priority_queue<std::vector<int>> queue;
	float x = tank->GetX();
	float y = tank->GetY();
	std::vector<std::vector<int>> checked(22,std::vector<int>(22,false));


	std::vector<std::pair<int, int>> dir = {
		{0,0},  // dummy for index 0
		{0,-1}, // 1 = UP
		{1,0},  // 2 = RIGHT
		{0,1},  // 3 = DOWN
		{-1,0}  // 4 = LEFT
	};

	std::vector<int> best = { map[int(round(x))][int(round(y))],0 };
	for (int i = 1; i < dir.size();i++) {
		float nx = x + dir[i].first;
		float ny = y + dir[i].second;
		if (!passable(pAI, nx, ny)) continue;
		int nv = map[int(round(x))][int(round(y))] - cost(x, nx, y, ny);
		if (best[0] < nv) {
			best = { nv ,i};
		}
		if (checked[int(round(nx))][int(round(ny))]) continue;
		checked[int(round(nx))][int(round(ny))] = true;

		queue.push({ map[int(round(nx))][int(round(ny))],int(round(nx)),int(round(ny)),i});
	}
	while (queue.size() > 0 && searchLimit) {
		searchLimit--;
		int v=queue.top()[0];
		int tx = queue.top()[1];
		int ty = queue.top()[2];
		int od = queue.top()[3];
		queue.pop();

		for (auto d : dir) {
			int nx = tx + d.first;
			int ny = ty + d.second;
			if (!passable(pAI,nx, ny)) continue;
			int nv = map[nx][ny] - cost(x, nx, y, ny);
			if (best[0] < nv) {
				best = { nv ,od };
			}
			if(checked[nx][ny]) continue;
			checked[nx][ny] = true;

			queue.push({ nv ,nx,ny,od });
		}
	}
	return best[1];
}






