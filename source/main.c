// Haunted House 3D - Nintendo DS (libnds)
// Collect 3 keys, open the exit door, escape the old ghost lady.
// Controls: D-Pad = move/turn, L/R = strafe, A = run, START = restart

#include <nds.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define MW 16
#define MH 12
#define H 2048  // 0.5 in v16 (12-bit fraction)

static const char *LAYOUT[MH] = {
    "################",
    "#P.....#.......#",
    "#......#...K...#",
    "#..##..#.......#",
    "#..##..###.#####",
    "#..............#",
    "####.#####.##..#",
    "#K...#.....#...#",
    "#....#..G..#...#",
    "#....#.....#.K.#",
    "#....#.....#...#",
    "##############E#",
};

static char map[MH][MW + 1];
static int dist[MH][MW];
static float px, pz, yaw, ex, ez, bob, tcount;
static int keys, state;  // state: 0 play, 1 caught, 2 escaped
static float flick = 1.0f;

static void reset(void) {
    for (int z = 0; z < MH; z++) {
        for (int x = 0; x < MW; x++) {
            char c = LAYOUT[z][x];
            if (c == 'P') { px = x + 0.5f; pz = z + 0.5f; c = '.'; }
            else if (c == 'G') { ex = x + 0.5f; ez = z + 0.5f; c = '.'; }
            map[z][x] = c;
        }
        map[z][MW] = 0;
    }
    yaw = 270.0f;
    keys = 0;
    state = 0;
    bob = 0;
}

static int solid(float x, float z) {
    int cx = (int)floorf(x), cz = (int)floorf(z);
    if (cx < 0 || cz < 0 || cx >= MW || cz >= MH) return 1;
    char c = map[cz][cx];
    if (c == '#') return 1;
    if (c == 'E') return keys < 3;
    return 0;
}

static int collide(float x, float z, float r) {
    return solid(x - r, z - r) || solid(x + r, z - r) ||
           solid(x - r, z + r) || solid(x + r, z + r);
}

static void tryMove(float *x, float *z, float dx, float dz, float r) {
    if (!collide(*x + dx, *z, r)) *x += dx;
    if (!collide(*x, *z + dz, r)) *z += dz;
}

// ---------- drawing ----------

static void rawCube(float x, float y, float z, float sx, float sy, float sz,
                    int r, int g, int b, float br) {
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(sx, sy, sz);
    glBegin(GL_QUADS);

#define C(shade) glColor3b((int)(r * (shade) * br / 100), \
                           (int)(g * (shade) * br / 100), \
                           (int)(b * (shade) * br / 100))
    C(80);  // front
    glVertex3v16(-H, -H, H); glVertex3v16(H, -H, H);
    glVertex3v16(H, H, H);   glVertex3v16(-H, H, H);
    C(60);  // back
    glVertex3v16(-H, -H, -H); glVertex3v16(-H, H, -H);
    glVertex3v16(H, H, -H);   glVertex3v16(H, -H, -H);
    C(70);  // left
    glVertex3v16(-H, -H, -H); glVertex3v16(-H, -H, H);
    glVertex3v16(-H, H, H);   glVertex3v16(-H, H, -H);
    C(90);  // right
    glVertex3v16(H, -H, -H); glVertex3v16(H, H, -H);
    glVertex3v16(H, H, H);   glVertex3v16(H, -H, H);
    C(100); // top
    glVertex3v16(-H, H, -H); glVertex3v16(-H, H, H);
    glVertex3v16(H, H, H);   glVertex3v16(H, H, -H);
    C(50);  // bottom
    glVertex3v16(-H, -H, -H); glVertex3v16(H, -H, -H);
    glVertex3v16(H, -H, H);   glVertex3v16(-H, -H, H);
#undef C
    glEnd();
    glPopMatrix(1);
}

// Cube that fades with distance from the player (flashlight / darkness feel)
static void cube(float x, float y, float z, float sx, float sy, float sz,
                 int r, int g, int b) {
    float dx = x - px, dz = z - pz;
    float d = sqrtf(dx * dx + dz * dz);
    float br = (1.0f - d / 9.0f) * flick;
    if (br <= 0.02f) return;
    rawCube(x, y, z, sx, sy, sz, r, g, b, br);
}

static void drawWorld(void) {
    // floor and ceiling
    rawCube(MW / 2.0f, -0.55f, MH / 2.0f, MW, 0.1f, MH, 60, 50, 45, 0.3f);
    rawCube(MW / 2.0f, 1.15f, MH / 2.0f, MW, 0.1f, MH, 40, 40, 45, 0.25f);

    for (int z = 0; z < MH; z++) {
        for (int x = 0; x < MW; x++) {
            char c = map[z][x];
            float cx = x + 0.5f, cz = z + 0.5f;
            if (c == '#') {
                if ((x + z) & 1) cube(cx, 0.3f, cz, 1, 1.6f, 1, 120, 95, 80);
                else             cube(cx, 0.3f, cz, 1, 1.6f, 1, 105, 80, 68);
            } else if (c == 'E') {
                if (keys < 3) cube(cx, 0.3f, cz, 1, 1.6f, 1, 150, 80, 30);
                else          cube(cx, 0.3f, cz, 1, 1.6f, 1, 40, 220, 70);
            } else if (c == 'K') {
                float yy = -0.15f + sinf(tcount * 3.0f) * 0.06f;
                cube(cx, yy, cz, 0.2f, 0.2f, 0.2f, 255, 220, 0);
            }
        }
    }

    // the ghost lady
    cube(ex, 0.0f, ez, 0.45f, 1.0f, 0.3f, 150, 70, 170);    // body
    cube(ex, 0.62f, ez, 0.3f, 0.3f, 0.3f, 230, 215, 200);   // head
    cube(ex, 0.75f, ez, 0.34f, 0.1f, 0.34f, 200, 200, 210); // gray hair
}

// ---------- enemy AI (BFS over grid) ----------

static void bfs(int sx, int sz) {
    static int qx[MW * MH], qz[MW * MH];
    for (int z = 0; z < MH; z++)
        for (int x = 0; x < MW; x++) dist[z][x] = 999;
    int head = 0, tail = 0;
    dist[sz][sx] = 0;
    qx[tail] = sx; qz[tail++] = sz;
    static const int DX[4] = {1, -1, 0, 0}, DZ[4] = {0, 0, 1, -1};
    while (head < tail) {
        int x = qx[head], z = qz[head++];
        for (int i = 0; i < 4; i++) {
            int nx = x + DX[i], nz = z + DZ[i];
            if (nx < 0 || nz < 0 || nx >= MW || nz >= MH) continue;
            char c = map[nz][nx];
            if (c == '#' || c == 'E') continue;
            if (dist[nz][nx] != 999) continue;
            dist[nz][nx] = dist[z][x] + 1;
            qx[tail] = nx; qz[tail++] = nz;
        }
    }
}

static void updateEnemy(void) {
    int sx = (int)px, sz = (int)pz;
    if (sx < 0) sx = 0;
    if (sz < 0) sz = 0;
    if (sx >= MW) sx = MW - 1;
    if (sz >= MH) sz = MH - 1;
    bfs(sx, sz);

    int tx = (int)ex, tz = (int)ez;
    float gx = px, gz = pz;
    if (tx != sx || tz != sz) {
        int best = dist[tz][tx];
        static const int DX[4] = {1, -1, 0, 0}, DZ[4] = {0, 0, 1, -1};
        for (int i = 0; i < 4; i++) {
            int nx = tx + DX[i], nz = tz + DZ[i];
            if (nx < 0 || nz < 0 || nx >= MW || nz >= MH) continue;
            if (dist[nz][nx] < best) {
                best = dist[nz][nx];
                gx = nx + 0.5f; gz = nz + 0.5f;
            }
        }
    }
    float dx = gx - ex, dz = gz - ez;
    float len = sqrtf(dx * dx + dz * dz);
    float speed = 0.035f + 0.004f * keys;
    if (len > 0.001f) {
        float s = len < speed ? len : speed;
        ex += dx / len * s;
        ez += dz / len * s;
    }
}

// ---------- main ----------

int main(void) {
    videoSetMode(MODE_0_3D);
    glInit();
    glEnable(GL_ANTIALIAS);
    glClearColor(0, 0, 0, 31);
    glClearPolyID(63);
    glClearDepth(0x7FFF);
    glViewport(0, 0, 255, 191);
    consoleDemoInit();  // text on the bottom screen

    reset();

    while (1) {
        scanKeys();
        int held = keysHeld();
        int down = keysDown();
        tcount += 0.016f;

        if (state == 0) {
            float a = yaw * 0.0174533f;
            float fx = -sinf(a), fz = -cosf(a);
            float rx = cosf(a), rz = -sinf(a);
            float spd = (held & KEY_A) ? 0.085f : 0.050f;
            float mx = 0, mz = 0;

            if (held & KEY_LEFT)  yaw += 3.0f;
            if (held & KEY_RIGHT) yaw -= 3.0f;
            if (held & KEY_UP)    { mx += fx * spd; mz += fz * spd; }
            if (held & KEY_DOWN)  { mx -= fx * spd; mz -= fz * spd; }
            if (held & KEY_R)     { mx += rx * spd; mz += rz * spd; }
            if (held & KEY_L)     { mx -= rx * spd; mz -= rz * spd; }
            if (yaw >= 360.0f) yaw -= 360.0f;
            if (yaw < 0.0f) yaw += 360.0f;

            if (mx != 0 || mz != 0) {
                tryMove(&px, &pz, mx, mz, 0.22f);
                bob += 0.25f;
            }

            // pick up keys
            int cx = (int)px, cz = (int)pz;
            if (cx >= 0 && cz >= 0 && cx < MW && cz < MH) {
                if (map[cz][cx] == 'K') { map[cz][cx] = '.'; keys++; }
            }
            // door
            if (keys >= 3) {
                float ddx = px - (14 + 0.5f), ddz = pz - (11 + 0.5f);
                if (ddx * ddx + ddz * ddz < 1.0f) state = 2;
            }

            updateEnemy();
            float ddx = px - ex, ddz = pz - ez;
            if (ddx * ddx + ddz * ddz < 0.30f) state = 1;

            flick = (rand() % 25 == 0) ? 0.55f : 1.0f;
        } else if (down & KEY_START) {
            reset();
        }

        // 3D render
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(70, 256.0f / 192.0f, 0.1f, 30.0f);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glPolyFmt(POLY_ALPHA(31) | POLY_CULL_NONE);
        glRotateY(-yaw);
        glTranslatef(-px, -(0.1f + sinf(bob) * 0.02f), -pz);
        drawWorld();
        glFlush(0);

        // HUD on bottom screen
        printf("\x1b[0;0H");
        printf("  HAUNTED HOUSE 3D\n\n");
        printf("  Keys: %d / 3        \n", keys);
        if (state == 0) {
            printf("  %s\n\n", keys >= 3 ? "Door is OPEN! Run!      "
                                         : "Find the keys...        ");
            printf("  Pad: move/turn  L/R: strafe\n");
            printf("  A: run\n");
        } else if (state == 1) {
            printf("  SHE CAUGHT YOU!          \n\n");
            printf("  Press START to retry     \n");
        } else {
            printf("  YOU ESCAPED!             \n\n");
            printf("  Press START to play again\n");
        }

        swiWaitForVBlank();
    }
    return 0;
}
