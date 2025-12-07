
#include "score.h"

int grid[22][22];

bool isWalkable(AI* pAI, float x, float y) {
    if (x <= 0 || x >= 21 || y <= 0 || y >= 21) return false;

    for (int i= floor(x+0.1); i <= ceil(y-0.1); i++) {
        for (int j = floor(x + 0.1); j <= ceil(y - 0.1); j++) {
            int b = pAI->GetBlock(i, j);
            if (b == BLOCK_HARD_OBSTACLE || b == BLOCK_SOFT_OBSTACLE || b == BLOCK_WATER)
                return false;

        }
    }
    return true;
}

bool isShootable(AI* pAI, float x, float y) {
    if (x <= 0 || x >= 21 || y <= 0 || y >= 21) return false;

    for (int i = floor(x + 0.1); i <= ceil(y - 0.1); i++) {
        for (int j = floor(x + 0.1); j <= ceil(y - 0.1); j++) {
            int b = pAI->GetBlock(i, j);
            if (b == BLOCK_HARD_OBSTACLE || b == BLOCK_SOFT_OBSTACLE)
                return false;
        }
    }
    return true;
}



void castRay(AI* pAI, int sx, int sy, int dx, int dy, int startVal, int decay) {
    int cx = sx;
    int cy = sy;
    int cur = startVal;

    while (true) {
        cx += dx;
        cy += dy;

        if (!isShootable(pAI, cx, cy)) break;

        grid[cx][cy] += cur;

        if (startVal < 0) cur += decay;
        else cur -= decay;

        if ((startVal < 0 && cur >= 0) || (startVal > 0 && cur <= 0)) break;
    }
}

void calculateMap(AI* pAI) {
    // A. Reset Grid
    for (int y = 0; y < 22; y++)
        for (int x = 0; x < 22; x++)
            grid[x][y] = 0;

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

        castRay(pAI, round(b->GetX()), round(b->GetY()), dx, dy, -10000, 500);
    }

    // C. Mark Enemies
    for (int i = 0; i < NUMBER_OF_TANK; i++) {
        Tank* t = pAI->GetEnemyTank(i);
        if (!t || t->GetHP() == 0) continue;

        int tx = round(t->GetX());
        int ty = round(t->GetY());

        castRay(pAI, tx, ty, 0, -1, 500, 0); // Up
        castRay(pAI, tx, ty, 0, 1, 500, 0); // Down
        castRay(pAI, tx, ty, -1, 0, 500, 0); // Left
        castRay(pAI, tx, ty, 1, 0, 500, 0); // Right
    }
}

struct Node { int x, y, firstDir; };

// =========================================================
// 4. GET SMART MOVE (Float-First BFS)
// =========================================================
int getSmartMove(AI* pAI, Tank* tank) {
    int myX = round(tank->GetX());
    int myY = round(tank->GetY());
    float fx = tank->GetX();
    float fy = tank->GetY();

    // --- STEP 1: Find Best Goal ---
    int bestScore = -999999;
    int tx = myX, ty = myY;

    for (int i = 1; i < 21; i++) {
        for (int j = 1; j < 21; j++) {
            if (!isWalkable(pAI, i, j)) {
                continue;
            }

            // Distance Cost: 10 points per step.
            // This ensures we pick the NEAREST tile on the "Shooting Line".
            int dist = abs(i - myX) + abs(j - myY);
            int score = grid[i][j] - (dist * 10);

            if (score > bestScore) {
                bestScore = score;
                tx = i; ty = j;
            }
        }
    }


    // If satisfied, stay.
    if (tx == myX && ty == myY) return 0;

    // --- STEP 2: Float-First Queue Init (The Anti-Snap Fix) ---
    std::queue<Node> q;
    bool visited[22][22] = { false };

    // Up, Right, Down, Left
    int dx[] = { 0, 0, 1, 0, -1 };
    int dy[] = { 0, -1, 0, 1, 0 };

    // Try all 4 physical directions first
    for (int dir = 1; dir <= 4; dir++) {
        // Look ahead 0.5 units
        float nextFX = fx + dx[dir];
        float nextFY = fy + dy[dir];

        // Does the tank FIT there?
        if (isWalkable(pAI, nextFX, nextFY)) {
            int nextIX = round(nextFX);
            int nextIY = round(nextFY);

            visited[nextIX][nextIY] = true;
            q.push({ nextIX, nextIY, dir });
        }
    }

    // --- STEP 3: Integer BFS ---
    while (!q.empty()) {
        Node n = q.front(); q.pop();

        if (n.x == tx && n.y == ty) return n.firstDir;

        for (int i = 1; i <= 4; i++) {
            int nx = n.x + dx[i];
            int ny = n.y + dy[i];

            if(isWalkable(pAI,nx,ny)&& !visited[nx][ny]){
                visited[nx][ny] = true;
                q.push({ nx, ny, n.firstDir });
            }
        }
    }

    return 0; // No path
}