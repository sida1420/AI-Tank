
#include "score.h"

int grid[22][22];
int cost[22][22];

bool isWalkable(AI* pAI, float x, float y) {
    if (x <= 0 || x >= 21 || y <= 0 || y >= 21) return false;

    for (int i= floor(x+0.01); i <= ceil(x-0.01); i++) {
        for (int j = floor(y + 0.01); j <= ceil(y - 0.01); j++) {
            int b = pAI->GetBlock(i, j);
            if (b == BLOCK_HARD_OBSTACLE || b == BLOCK_SOFT_OBSTACLE || b == BLOCK_WATER)
                return false;
        }
    }
    

    return true;
}

bool collision(AI* pAI, float x, float y, int id) {

    for (int i = 0; i < NUMBER_OF_TANK; i++) {
        if (id == i) continue;
        Tank* t = pAI->GetMyTank(i);
        if (abs(t->GetX() - x) < 1 && abs(t->GetY() - y) < 1)
            return true;
        t = pAI->GetEnemyTank(i);
        if (abs(t->GetX() - x) < 1 && abs(t->GetY() - y) < 1)
            return true;
    }
    return false;
}

bool isShootable(AI* pAI, float x, float y) {
    if (x <= 0 || x >= 21 || y <= 0 || y >= 21) return false;

    for (int i = floor(x + 0.01); i <= ceil(x - 0.01); i++) {
        for (int j = floor(y + 0.01); j <= ceil(y - 0.01); j++) {
            int b = pAI->GetBlock(i, j);
            if (b == BLOCK_HARD_OBSTACLE || b == BLOCK_SOFT_OBSTACLE)
                return false;
        }
    }
    return true;
}



void castRay(AI* pAI, int sx, int sy, int dx, int dy, int startVal, int decay, bool costAffect=false) {
    int cx = sx;
    int cy = sy;
    int cur = startVal;

    while (true) {
        cx += dx;
        cy += dy;

        if (!isShootable(pAI, cx, cy)) break;

        grid[cx][cy] += cur;
        if (costAffect) cost[cx][cy] += cur;

        if (startVal < 0) cur += decay;
        else cur -= decay;

        if ((startVal < 0 && cur >= 0) || (startVal > 0 && cur <= 0)) break;
    }
}

void calculateMap(AI* pAI) {
    // A. Reset Grid
    for (int y = 0; y < 22; y++)
        for (int x = 0; x < 22; x++)
        {
            if (isWalkable(pAI, x, y)) {
                grid[x][y] = 0;
                cost[x][y] = 0;

            }
            else {
                grid[x][y] = -10000;
                cost[x][y] = -10000;
            }
        }

    // B. Mark Bullets
    
    auto bullets = pAI->GetEnemyBullets();
    for (auto b : bullets) {
        // Map direction 1..4 to dx/dy
        int dx = 0, dy = 0;
        int d = b->GetDirection();
        if (d == 1) dy = -1;
        if (d == 2) dx = 1;
        if (d == 3) dy = 1;
        if (d == 4) dx = -1;

        castRay(pAI, round(b->GetX()), round(b->GetY()), dx, dy, -500*b->GetDamage(), 500, true);
    }

    // C. Mark Enemies
    for (int i = 0; i < NUMBER_OF_TANK; i++) {
        Tank* t = pAI->GetEnemyTank(i);
        int tx = round(t->GetX());
        int ty = round(t->GetY());
        grid[tx][ty] -= 10000;
        if (!t || t->GetHP() == 0) {
            continue;
        }


        castRay(pAI, tx, ty, 0, -1, 500, 0, false); // Up
        castRay(pAI, tx, ty, 0, 1, 500, 0, false); // Down
        castRay(pAI, tx, ty, -1, 0, 500, 0, false); // Left
        castRay(pAI, tx, ty, 1, 0, 500, 0, false); // Right
    }
}

struct Node { int x, y, g, h, firstDir;
    int f() const 
    { return -g - h; }
    bool operator<(const Node& other) const 
    { return f() < other.f(); }
};

// =========================================================
// 4. GET SMART MOVE (Float-First BFS)
// =========================================================
int getSmartMove(AI* pAI, int id) {
    auto tank = pAI->GetMyTank(id);
    int myX = round(tank->GetX());
    int myY = round(tank->GetY());
    float fx = tank->GetX();
    float fy = tank->GetY();

    // --- STEP 1: Find Best Goal ---
    int bestScore = -999999;
    int tx = myX, ty = myY;

    //std::cout << "MAP\n";
    for (int i = 1; i < 21; i++) {
        for (int j = 1; j < 21; j++) {
           // std::cout << grid[j][i] << "\t";
            if (!isWalkable(pAI, i, j)) {
                continue;
            }
            

            // Distance Cost: 10 points per step.
            // This ensures we pick the NEAREST tile on the "Shooting Line".
            int dist = abs(i - myX) + abs(j - myY);
            int score = grid[i][j] - (dist * 100)-(collision(pAI,i,j,id)?5000:0);

            if (score > bestScore) {
                bestScore = score;
                tx = i; ty = j;
            }
        }
        //std::cout << "\n";
    }


    // If satisfied, stay.
    if (abs(tx -fx)<0.01 && abs(ty-fy) <0.01) return 0;

    // --- STEP 2: Float-First Queue Init (The Anti-Snap Fix) ---
    std::priority_queue<Node> pq; 
    int dist[22][22];
    for (int i = 0; i < 22; i++) for (int j = 0; j < 22; j++) dist[i][j] = 999999;

    // Up, Right, Down, Left
    int dx[] = { 0, 0, 1, 0, -1 };
    int dy[] = { 0, -1, 0, 1, 0 };

    // Try all 4 physical directions first
    for (int dir = 1; dir <= 4; dir++) {
        float nextFX = fx + 0.5*dx[dir];
        float nextFY = fy + 0.5*dy[dir];

        // Does the tank FIT there?
        if (isWalkable(pAI, nextFX, nextFY)) {
            int nextIX = round(fx + dx[dir]);
            int nextIY = round(fy + dy[dir]);

            dist[nextIX][nextIY] = 0;
            pq.push({ nextIX, nextIY,abs(cost[nextIX][nextIY]) +(collision(pAI,nextIX,nextIY,id) ? 1000 : 0), int(abs(tx - nextIX) + abs(ty - nextIY)) , dir });
        }
    }

    // --- STEP 3: Integer A* ---
    int limit = 400 ;
    while (!pq.empty()&&limit--) {
        Node n = pq.top(); pq.pop();

        if (n.x == tx && n.y == ty) return n.firstDir;



        for (int i = 1; i <= 4; i++) {
            int nx = n.x + dx[i];
            int ny = n.y + dy[i];

            int h = int(abs(tx - nx) + abs(ty - ny));
            int g = n.g + abs(cost[nx][ny]) + 100 -  (collision(pAI,nx,ny,id)?1000:0);
            if (isWalkable(pAI, nx, ny) && g < dist[nx][ny])
            {
                dist[nx][ny] = g;
                pq.push({ nx, ny,g,h, n.firstDir });
            }
        }
    }

    return 0; // No path
}