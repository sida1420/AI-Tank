#include "score.h"

#include <iostream>
#include <ai/AI.h>
#include <queue>

using namespace std;

int dx[] = { 0,0,1,0,-1 };
int dy[] = { 0,-1,0,1,0 };

const int TIME=50;
const int WINDOW = 201;
float goal[22][22];
bool used[22][22];
bool visited[TIME][WINDOW][WINDOW];


AI* ai;

bool Movable(float x, float y) {
	for (int i = floor(x + 0.01); i <= ceil(x - 0.01); i++) {
		for (int j = floor(y + 0.01); j <= ceil(y - 0.01); j++) {
			if (ai->GetBlock(i,j) != BLOCK_GROUND) return false;
		}
	}
	return true;
}

void Change(float x, float y, float x2, float y2, float change) {
	for (int i = floor(x + 0.01); i <= ceil(x2 - 0.01); i++) {
		for (int j = floor(y + 0.01); j <= ceil(y2 - 0.01); j++) {
			if (i < 0 || j < 0 || i>21 || j>21) continue;
			if (!Movable(i, j)) continue;
			goal[i][j] += change;
		}
	}
}


bool Shootable(float x, float y) {
	for (int i = floor(x + 0.01); i <= ceil(x - 0.01); i++) {
		for (int j = floor(y + 0.01); j <= ceil(y - 0.01); j++) {
			if (i < 0 || j < 0 || i>21 || j>21) return false;
			int b = ai->GetBlock(i, j);
			if (b != BLOCK_GROUND&&b!=BLOCK_WATER) return false;
		}
	}
	return true;
}

bool collision(float x, float y, int id) {
	for (int i = 0; i < NUMBER_OF_TANK; i++) {
		auto t = ai->GetEnemyTank(i);
		if (abs(t->GetX() - x) < 1 && abs(t->GetY() - y) < 1) return true;
		if (i == id) continue;
		t = ai->GetMyTank(i);
		if (abs(t->GetX() - x) < 1 && abs(t->GetY() - y) < 1) return true;

	}
	return false;
}

struct hitbox {
	float x1, x2, y1, y2;
	hitbox(float x1, float y1, float x2, float y2) {
		this->x1 = min(x1, x2);
		this->x2 = max(x1, x2);
		this->y1 = min(y1, y2);
		this->y2 = max(y1, y2);
	}
};

hitbox toHitbox(float x, float y, float w, float h, int d, float v) {
	float x1 = min(x + v * dx[d], x);
	float y1 = min(y + v * dy[d], y);
	float x2 = max(x + w, x + w + v * dx[d]);
	float y2 = max(y + h, y + h + v * dy[d]);

	return hitbox(x1, y1, x2, y2);
}

int bulletCol(float x, float y, int t) {
	int pen = 0;

	hitbox thb(x - 0.55, y - 0.55, x + 0.55, y + 0.55);
	for (auto b : ai->GetEnemyBullets()) {
		int d = b->GetDirection();
		float s = b->GetSpeed();

		float bx = b->GetX() + t * dx[d] * s;
		float by = b->GetY() + t * dy[d] * s;

		hitbox bhb(bx, by, bx - dx[d] * s *0.6, by - dy[d] * s *0.6);

		if (thb.x1 <= bhb.x2
			&& thb.x2 >= bhb.x1
			&& thb.y1 <= bhb.y2
			&& thb.y2 >= bhb.y1) {
			pen += 100 + b->GetDamage() * 10;
		}

	}
	return pen;
}


int castRay(float x, float y,int dir, int offset, int start, int decay, bool tank = false) {
	int dist = 0;
	int cur = start;

	int cx = round(x);
	int cy = round(y);


	while (true) {
		if (cx < 0 || cy < 0 || cx>21 || cy>21) break;
		if (!Shootable(cx, cy)) break;

		if (offset>=0) {
			cur = start - offset * decay;
			offset--;
		}


		if (tank) {
			if(used[cx][cy])
				goal[cx][cy] -= 3*cur;
			else {
				used[cx][cy] = true;
			}
		}
		goal[cx][cy] += cur;
		cur -= decay;

		dist++;
		if ((start > 0 && cur < 0) || (start < 0 && cur>0)) break;


		cx += dx[dir];
		cy += dy[dir];
	}
	return dist;

}


bool danger(int id) {
	auto t = ai->GetMyTank(id);
	hitbox thb(t->GetX() - 0.55, t->GetY() - 0.55, t->GetX() + 0.55, t->GetY() + 0.55);

	for (auto b : ai->GetEnemyBullets()) {
		//difference in each axies
		float xx = t->GetX() - b->GetX();
		float yy = t->GetY() - b->GetY();
		float wx = abs(abs(xx) * dy[b->GetDirection()]);
		float wy = abs(abs(yy) * dx[b->GetDirection()]);
		int d = b->GetDirection();

		//Check if bullet is head toward tank
		if (xx>0 && (d != 2 || wy > 0.6)
			|| xx<0 && (d != 4 || wy > 0.6)
			|| yy>0 && (d != 3 || wx > 0.6)
			|| yy<0 < b->GetY() && (d != 1 || wx > 0.6))
			continue;

		//Travel distance from bullet to tank
		float db = abs((xx) * dx[d] + (yy) * dy[d]);
		float tb = (db - 0.5) / b->GetSpeed();

		//How far it take for tank to strafe sideway
		float dd = abs(min(abs(b->GetX() - thb.x1), abs(b->GetX() - thb.x2)) * dy[d] +
					min(abs(b->GetY() - thb.y1), abs(b->GetY() - thb.y2)) * dx[d]);
		float tt = (dd) / t->GetSpeed();
		//Check if there will be collision. If bullet is to close, tank the hit
		if (tb >= tt-1 && tb <= tt + 2) return true;
		
	}

	return false;
}


int trigger(int id) {
	vector<pair<float, int>> prot;

	auto tank = ai->GetMyTank(id);

	float speed = 1;
	if (tank->GetType() == TANK_LIGHT)
		speed = 1.2;
	else if (tank->GetType() == TANK_HEAVY)
		speed = 0.8;


	for (int i = 1; i < 5; i++) {
		int dist = castRay(tank->GetX()+dx[i], tank->GetY()+dy[i], i, 0, 0, 0);
		hitbox line(tank->GetX(),tank->GetY(),tank->GetX()+dx[i]*dist,tank->GetY()+dy[i]*dist);

		for (int j = 0; j < 4;j++) {
			auto e = ai->GetEnemyTank(j);
			if (e->GetHP() <= 0) continue;

			float d = abs(tank->GetX() - e->GetX()) * dx[i] + abs(tank->GetY() - e->GetY()) * dy[i] - 1.f;

			float t = d / speed;
			
			hitbox temp = toHitbox(e->GetX()-0.5, e->GetY()-0.5, 1, 1, e->GetDirection(), min(2,max(0,t*e->GetSpeed()-0.9f)));

			if (temp.x1 <= line.x2
				&& temp.x2 >= line.x1
				&& temp.y1 <= line.y2
				&& temp.y2 >= line.y1) {
				prot.push_back({ d, i });
			}
		}

	}

	sort(prot.begin(), prot.end());
	if (prot.empty()) return 0;

	return prot[0].second;
}


void calculate(int id) {
	for (int i = 0; i < 22; i++) {
		for (int j = 0; j < 22; j++) {
			used[i][j] = false;
			if (Movable( i, j)) {
				goal[i][j] = 5*(max(abs(i-10.5),abs(j-10.5))-min(abs(i-10.5),abs(j-10.5)));
				

			}
			else {
				goal[i][j] = -10000;
			}
		}
	}
	auto tank = ai->GetMyTank(id);

	int offset = ceil(1.2f / tank->GetSpeed()+ tank->GetSpeed());

	for (int i = 0; i < NUMBER_OF_TANK;i++) {
		auto t = ai->GetEnemyTank(i);
		int x = round(t->GetX());
		int y = round(t->GetY());
		Change(t->GetX(), t->GetY(), t->GetX(), t->GetY(), -5000);

		if (t->GetHP() <= 0) continue;

		int start = 600+ (tank->GetHP()>t->GetHP())*5;

		castRay(x, y-1, 1, offset, start, 50, true);
		castRay(x+1, y, 2, offset, start, 50, true);
		castRay(x, y+1, 3, offset, start, 50, true);
		castRay(x-1, y, 4, offset, start, 50, true);
	}

	for (int i = 0; i < NUMBER_OF_TANK; i++) {
		if (i == id) continue;
		auto t = ai->GetMyTank(i);
		int x = round(t->GetX());
		int y = round(t->GetY());
		Change(t->GetX(), t->GetY(), t->GetX(), t->GetY(), -5000);

		if (t->GetHP() <= 0) continue;

		int start = 10;

		castRay(x, y-1, 1, 0, start, 1, true);
		castRay(x+1, y, 2, 0, start, 1, true);
		castRay(x, y+1, 3, 0, start, 1, true);
		castRay(x-1, y, 4, 0, start, 1, true);
	}

	for (auto b : ai->GetEnemyBullets()) {
		castRay(b->GetX(), b->GetY(), b->GetDirection(), 0, -b->GetDamage() * 50, -100);
	}

	for (auto p : ai->GetPowerUpList()) {
		Change(p->GetX(), p->GetY(), p->GetX(), p->GetY(), 5000);
	}

	for (auto s : ai->GetIncomingStrike()) {
		Change(s->GetX()-3, s->GetY()-3, s->GetX()+3, s->GetY()+3, -10000);
	}


}

struct node {
	float x, y, g, h;
	int t, dir;

	bool operator<(const node& other) const {
		return g + h > other.g + other.h;
	}
	bool operator>(const node& other) const {
		return g + h < other.g + other.h;
	}
};

int move(int id) {

	ai = AI::GetInstance();
	calculate(id);

	auto tank = ai->GetMyTank(id);

	float fx = tank->GetX();
	float fy = tank->GetY();
	float speed = tank->GetSpeed();

	float tx = round(fx);
	float ty = round(fy);
	float best = -1e9;
	//best
	for (int i = 0; i < 22; i++) {
		for (int j = 0; j < 22; j++) {
			if (!Movable(i, j)) continue;

			float score= goal[i][j]- 10*(abs(i - fx) + abs(j - fy));

			if (best < score) {
				best = score;
				tx = i;
				ty = j;
			}
		}
	}

	if (abs(tx - fx) < 0.1 &&abs( ty - fy) < 0.1) return 0;

	node bNode = { 0,0,1e9,1e9,0,0 };

	int lxo = round(fx * 20 )- 100;
	int lyo = round(fy * 20 )- 100;

	priority_queue<node> pq;
	memset(visited, false, sizeof(visited));


	for (int i = 0; i < 5; i++) {
		float gx = fx + speed * dx[i];
		float gy = fy + speed * dy[i];

		if (!Movable(gx, gy)) continue;

		int lx = round(gx * 20) - lxo;
		int ly = round(gy * 20) - lyo;

		if (visited[1][lx][ly]) continue;

		float g = speed+speed*( (collision(gx, gy, id) ? 5000 : 0) + bulletCol(gx, gy, 1));
		float h = 1.5 * (abs(gx - tx) + abs(gy - ty));

		node temp = { gx,gy,g,h,1,i };
		pq.push(temp);
	}

	int limit = 8000;

	while (!pq.empty()) {
		node n = pq.top(); pq.pop();
		int lx = round(n.x * 20) - lxo;
		int ly = round(n.y * 20) - lyo;
		if (abs(tx - n.x) < 0.1 && abs(ty - n.y )< 0.1) return n.dir;
		if (bNode.h + bNode.g - bNode.t * speed > n.h + n.g - n.t * speed) {
			bNode = n;
		}

		if (visited[n.t][lx][ly]) continue;
		visited[n.t][lx][ly] = true;



		int t = n.t + 1;
		if (t >= TIME) continue;

		for (int i = 0; i < 5; i++) {
			float gx = n.x + speed * dx[i];
			float gy = n.y + speed * dy[i];

			if (!Movable(gx, gy)) continue;

			lx = round(gx * 20) - lxo;
			ly = round(gy * 20) - lyo;

			if (lx < 0 || ly < 0 || lx >= WINDOW || ly >= WINDOW) continue;

			if (visited[t][lx][ly]) continue;

			float g = n.g + speed + speed*( (collision(gx, gy, id) ? 2000 : 0) + bulletCol(gx, gy, t));
			float h = 1.5 * (abs(gx - tx) + abs(gy - ty));

			node temp = { gx,gy,g,h,t,n.dir };
			pq.push(temp);

		}

		limit--;
		if (!limit) {
			cout << "Limit\n";
			break;
		}


		
	}
	cout << "Ran out\n";

	return bNode.dir;
}