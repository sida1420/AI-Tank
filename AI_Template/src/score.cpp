
#include "score.h"
using namespace std;
int grid[22][22];
int cost[22][22];
bool used[22][22];

int dx[] = { 0, 0, 1, 0, -1 };
int dy[] = { 0, -1, 0, 1, 0 };

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

float hit_box(int typetank) {
    return 0.5;
}

int trigger_attack2(int id) {



    AI* p_AI = AI::GetInstance();

    //kiem tra xem xe tang nao co hit box trong tam ban theo 4 huong, moi lan lap la kiem tra 1 huong
    for (int i = 1; i < 5; i++) {
        //toa do xe tang minh
        float tx = p_AI->GetMyTank(id)->GetX();
        float ty = p_AI->GetMyTank(id)->GetY();
        // Neu gap tuong thi dung lai
        do {
            tx += dx[i];
            ty += dy[i];

            for (int j = 0; j < 4; j++) {
                Tank* tempTank = p_AI->GetEnemyTank(j);
                //xe dich chet roi thi ko ban
                if ((tempTank == NULL) || (tempTank->GetHP() == 0))
                    continue;
                float ex = tempTank->GetX();
                float ey = tempTank->GetY();
                float ex1 = ex + dx[tempTank->GetDirection()] * hit_box(tempTank->GetType());
                float ey1 = ey + dy[tempTank->GetDirection()] * hit_box(tempTank->GetType());

                if (tx >= min(ex, ex1) && tx <= max(ex, ex1) && abs(ty - ey) < 0.5) {
                    //cout << "Command from tx " << ex << " " << ex1 << " " << j << endl;
                    return i;
                    //Game::CommandTank(id, i + 1, false, true);
                    return true;
                }
                if (ty >= min(ey, ey1) && ty <= max(ey, ey1) && abs(tx - ex) < 0.5) {
                    //cout << "Command from ty " << ey << " " << ey1 << " " << j << endl;
                    return i;
                   // Game::CommandTank(id, i + 1, false, true);
                    return true;
                }
            }
            
        } while(isShootable(p_AI,tx,ty));
    }
    return 0;
}

void castRay(AI* pAI, int sx, int sy, int dx, int dy, int startVal, int decay, bool costAffect=false) {
    int cx = sx;
    int cy = sy;
    int cur = startVal;

    while (true) {

        if (!isShootable(pAI, cx, cy)) break;
        if (!used[cx][cy]) {
            used[cx][cy] = true;
            grid[cx][cy] += cur;
        }
        if (costAffect) cost[cx][cy] += cur;

        if (startVal < 0) cur += decay;
        else cur -= decay;

        if ((startVal < 0 && cur >= 0) || (startVal > 0 && cur <= 0)) break;
        cx += dx;
        cy += dy;
    }
}

void calculateMap(AI* pAI) {
    // A. Reset Grid
    for (int y = 0; y < 22; y++)
        for (int x = 0; x < 22; x++)
        {
            used[x][y] = false;
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
            int score = grid[i][j] - (dist * 10)-(collision(pAI,i,j,id)?5000:0);

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
    bool check[22][22] = { false };

    // Up, Right, Down, Left
    int dx[] = { 0, 0, 1, 0, -1 };
    int dy[] = { 0, -1, 0, 1, 0 };

    // Try all 4 physical directions first
    for (int dir = 1; dir <= 4; dir++) {
        float nextFX;
        float nextFY;

        float eps = 0.1f;

        if (dx[dir] == 1)       nextFX = floor(fx + eps) + 1; // Right: 6.75 -> 7; 7.0 -> 8
        else if (dx[dir] == -1) nextFX = ceil(fx - eps) - 1;  // Left:  6.75 -> 6; 7.0 -> 6
        else                    nextFX = fx + 0.5 * dx[dir];           // Vertical: Keep current X Round

        // 2. Calculate Next Y Integer
        if (dy[dir] == 1)       nextFY = floor(fy + eps) + 1; // Down
        else if (dy[dir] == -1) nextFY = ceil(fy - eps) - 1;  // Up
        else                    nextFY = fy + 0.5 * dy[dir];

        // Does the tank FIT there?
        if (isWalkable(pAI, nextFX, nextFY)) {
            int nextIX = round(nextFX);
            int nextIY = round(nextFY);

            check[nextIX][nextIY] = true;
            pq.push({ nextIX, nextIY,abs(cost[nextIX][nextIY]) +(collision(pAI,nextIX,nextIY,id) ? 1000 : 0), int(abs(tx - nextIX) + abs(ty - nextIY)) , dir });
        }
    }

    // --- STEP 3: Integer A* ---
    int limit = 1000 ;
    while (!pq.empty()) {
        limit--;
        if (limit == 0) {
            return pq.top().firstDir;
        }
        Node n = pq.top(); pq.pop();

        if (n.x == tx && n.y == ty) return n.firstDir;



        for (int i = 1; i <= 4; i++) {
            int nx = n.x + dx[i];
            int ny = n.y + dy[i];

            int h = int(abs(tx - nx) + abs(ty - ny));
            int g = n.g + abs(cost[nx][ny]) + 100 +  (collision(pAI,nx,ny,id)?1000:0);
            if (isWalkable(pAI, nx, ny) && !check[nx][ny])
            {
                check[nx][ny] = true;
                pq.push({ nx, ny,g,h, n.firstDir });
            }
        }
    }

    return 0; // No path
}