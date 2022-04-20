#include <cstring>           // memset
#include <cassert>           // assert
#include <cstdio>            // sprintf
#include <iostream>          // cout
#include "engine/math.hpp"   // math
#include "engine/mesh.hpp"   // mesh
#include "engine/opengl.hpp" // opengl, glfw
#include "engine/shader.hpp" // shader
#include "engine/window.hpp" // window
#include "engine/font.hpp"   // font
#include "game/game.hpp"     // game features

// ---------------------------------------------------------------------------
// Defines

#define GAME_OBJ_NUM_MAX 32
#define GAME_OBJ_INST_NUM_MAX 2048

#define AST_NUM_MIN 2         // minimum number of asteroid alive
#define AST_NUM_MAX 32        // maximum number of asteroid alive
#define AST_SIZE_MAX 200.0f   // maximum asterois size
#define AST_SIZE_MIN 50.0f    // minimum asterois size
#define AST_VEL_MAX 100.0f    // maximum asterois velocity
#define AST_VEL_MIN 50.0f     // minimum asterois velocity
#define AST_CREATE_DELAY 0.1f // delay of creation between one asteroid to the next
#define AST_SPECIAL_RATIO 4   // number of asteroid for each 'special'
#define AST_SHIP_RATIO 100    // number of asteroid for each ship
#define AST_LIFE_MAX 10       // the life of the biggest asteroid (smaller one is scaled accordingly)
#define AST_VEL_DAMP 1E-8f    // dampening to use if the asteroid velocity is above the maximum
#define AST_TO_SHIP_ACC 0.01f // how much acceleration to apply to steer the asteroid toward the ship

#define SHIP_INITIAL_NUM 3          // initial number of ship
#define SHIP_SPECIAL_NUM 20         //
#define SHIP_SIZE 30.0f             // ship size
#define SHIP_ACCEL_FORWARD 100.0f   // ship forward acceleration (in m/s^2)
#define SHIP_ACCEL_BACKWARD -100.0f // ship backward acceleration (in m/s^2)
#define SHIP_DAMP_FORWARD 0.55f     // ship forward dampening
#define SHIP_DAMP_BACKWARD 0.05f    // ship backward dampening
#define SHIP_ROT_SPEED (1.0f * PI)  // ship rotation speed (degree/second)

#define BULLET_SPEED 1000.0f // bullet speed (m/s)
#define BULLET_SIZE 10.0f

#define BOMB_COST 0        // cost to fire a bomb
#define BOMB_RADIUS 250.0f // bomb radius
#define BOMB_LIFE 5.0f     // how many seconds the bomb will be alive
#define BOMB_SIZE 1.0f

#define MISSILE_COST 0
#define MISSILE_ACCEL 1000.0f // missile acceleration
#define MISSILE_TURN_SPEED PI // missile turning speed (radian/sec)
#define MISSILE_DAMP 1.5E-6f  // missile dampening
#define MISSILE_LIFE 10.0f    // how many seconds the missile will be alive
#define MISSILES_SIZE 10.0f   // size of missile.

#define PTCL_SCALE_DAMP 0.05f // particle scale dampening
#define PTCL_VEL_DAMP 0.05f   // particle velocity dampening

#define COLL_COEF_OF_RESTITUTION 1.0f // collision coefficient of restitution
#define COLL_RESOLVE_SIMPLE 1

// ---------------------------------------------------------------------------
enum
{
    // list of game object types
    TYPE_SHIP = 0,
    TYPE_BULLET,
    TYPE_BOMB,
    TYPE_MISSILE,
    TYPE_ASTEROID,
    TYPE_STAR,

    TYPE_PTCL_WHITE,
    TYPE_PTCL_YELLOW,
    TYPE_PTCL_RED,

    TYPE_NUM,

    PTCL_EXHAUST,
    PTCL_EXPLOSION_S,
    PTCL_EXPLOSION_M,
    PTCL_EXPLOSION_L,
};

// ---------------------------------------------------------------------------
// object flag definition

#define FLAG_ACTIVE 0x00000001
#define FLAG_LIFE_CTR_S 8
#define FLAG_LIFE_CTR_M 0x000000FF


// ---------------------------------------------------------------------------
// Struct/Class definitions

struct GameObj
{
    uint32_t      type;  // object type
    engine::mesh* pMesh; // pbject
};

// ---------------------------------------------------------------------------

struct GameObjInst
{
    GameObj* pObject;   // pointer to the 'original'
    uint32_t flag;      // bit flag or-ed together
    float    life;      // object 'life'
    float    scale;     //
    vec2     posCurr;   // object current position
    vec2     velCurr;   // object current velocity
    float    dirCurr;   // object current direction
    mat4     transform; // object drawing matrix
    void* pUserData; // pointer to custom data specific for each object type
    vec4 modColor;
};

// ---------------------------------------------------------------------------
// Static variables

// list of original object
static GameObj  sGameObjList[GAME_OBJ_NUM_MAX];
static uint32_t sGameObjNum;

// list of object instances
static GameObjInst sGameObjInstList[GAME_OBJ_INST_NUM_MAX];
static uint32_t    sGameObjInstNum;

// pointer ot the ship object
static GameObjInst* spShip;

// keep track when the last asteroid was created
static float sAstCreationTime;

// keep track the total number of asteroid active and the maximum allowed
static uint32_t sAstCtr;
static uint32_t sAstNum;

// current ship rotation speed
static float sShipRotSpeed;

// number of ship available (lives, 0 = game over)
static long sShipCtr;

// number of special attack
static long sSpecialCtr;

// the score = number of asteroid destroyed
static uint32_t sScore;

static float sGameStateChangeCtr;

// Window size
static int gAEWinMinX = 0;
static int gAEWinMaxX = 640;
static int gAEWinMinY = 0;
static int gAEWinMaxY = 480;

// ---------------------------------------------------------------------------

// function to 'load' object data
static void loadGameObjList();

// function to create/destroy a game object object
static GameObjInst* gameObjInstCreate(uint32_t type, float scale, vec2* pPos, vec2* pVel, float dir, bool forceCreate);
static void         gameObjInstDestroy(GameObjInst* pInst);

// function to create asteroid
static GameObjInst* astCreate(GameObjInst* pSrc);

// function to calculate the object's velocity after collison
static void resolveCollision(GameObjInst* pSrc, GameObjInst* pDst, vec2* pNrm);

// function to create the particles
static void sparkCreate(uint32_t type, vec2* pPos, uint32_t count, float angleMin, float angleMax, float srcSize = 0.0f, float velScale = 1.0f, vec2* pVelInit = 0);

// function for the missile to find a new target
static GameObjInst* missileAcquireTarget(GameObjInst* pMissile);

// ---------------------------------------------------------------------------

void GameStatePlayLoadSolo(void)
{
    // zero the game object list
    std::memset(sGameObjList, 0, sizeof(GameObj) * GAME_OBJ_NUM_MAX);
    sGameObjNum = 0;

    // zero the game object instance list
    std::memset(sGameObjInstList, 0, sizeof(GameObjInst) * GAME_OBJ_INST_NUM_MAX);
    sGameObjInstNum = 0;

    spShip = 0;

    // load/create the mesh data
    loadGameObjList();

    // initialize the initial number of asteroid
    sAstCtr = 0;
}

// ---------------------------------------------------------------------------

void GameStatePlayInitSolo(bool serverIs, const std::string& address, uint16_t port, bool verbose)
{
    // reset the number of current asteroid and the total allowed
    sAstCtr = 0;
    sAstNum = AST_NUM_MIN;

    // create the main ship
    spShip = gameObjInstCreate(TYPE_SHIP, SHIP_SIZE, 0, 0, 0.0f, true);
    assert(spShip);

    // get the time the asteroid is created
    sAstCreationTime = game::instance().game_time();

    // generate the initial asteroid
    for (uint32_t i = 0; i < sAstNum; i++)
        astCreate(0);

    // reset the score and the number of ship
    sScore = 0;
    sShipCtr = SHIP_INITIAL_NUM;
    sSpecialCtr = SHIP_SPECIAL_NUM;

    // reset the delay to switch to the result state after game over
    sGameStateChangeCtr = 2.0f;
}

// ---------------------------------------------------------------------------

void GameStatePlayUpdateSolo(void)
{
    // =================
    // update the input
    // =================
    float const dt = game::instance().dt();
    if (spShip == 0) {
        sGameStateChangeCtr -= dt;

        if (sGameStateChangeCtr < 0.0) {
            // gAEGameStateNext = GS_RESULT;
            // TODO: Change to RESULT GAME STATE
        }
    }
    else {
        if (game::instance().input_key_pressed(GLFW_KEY_UP)) {
#if 0
            vec2 acc, u, dir;

            // calculate the current direction vector
            AEVector2Set(&dir, glm::cos(spShip->dirCurr), glm::sin(spShip->dirCurr));

            // calculate the dampening vector
            AEVec2Scale(&u, &spShip->velCurr, -AEVec2Length(&spShip->velCurr) * 0.01f);//pow(SHIP_DAMP_FORWARD, dt));

            // calculate the acceleration vector and add the dampening vector to it
            //AEVec2Scale	(&acc, &dir, 0.5f * SHIP_ACCEL_FORWARD * dt * dt);
            AEVec2Scale(&acc, &dir, SHIP_ACCEL_FORWARD);
            AEVec2Add(&acc, &acc, &u);

            // add the velocity to the position
            //AEVec2Scale	(&u,               &spShip->velCurr, dt);
            //AEVec2Add	(&spShip->posCurr, &spShip->posCurr, &u);
            // add the acceleration to the position
            AEVec2Scale(&u, &acc, 0.5f * dt * dt);
            AEVec2Add(&spShip->posCurr, &spShip->posCurr, &u);

            // add the acceleration to the velocity
            AEVec2Scale(&u, &acc, dt);
            AEVec2Add(&spShip->velCurr, &acc, &spShip->velCurr);

            AEVec2Scale(&u, &dir, -spShip->scale);
            AEVec2Add(&u, &u, &spShip->posCurr);

            sparkCreate(PTCL_EXHAUST, &u, 2, spShip->dirCurr + 0.8f * PI, spShip->dirCurr + 1.2f * PI);
#else
            vec2 pos, dir;
            dir = { glm::cos(spShip->dirCurr), glm::sin(spShip->dirCurr) };
            pos = dir;
            dir = dir * (SHIP_ACCEL_FORWARD * dt);
            spShip->velCurr = spShip->velCurr + dir;
            spShip->velCurr = spShip->velCurr * glm::pow(SHIP_DAMP_FORWARD, dt);

            pos = pos * -spShip->scale;
            pos = pos + spShip->posCurr;

            sparkCreate(PTCL_EXHAUST, &pos, 2, spShip->dirCurr + 0.8f * PI, spShip->dirCurr + 1.2f * PI);
#endif
        }
        if (game::instance().input_key_pressed(GLFW_KEY_DOWN)) {
            vec2 dir;

            dir = { glm::cos(spShip->dirCurr), glm::sin(spShip->dirCurr) };
            dir = dir * SHIP_ACCEL_BACKWARD * dt;
            spShip->velCurr = spShip->velCurr + dir;
            spShip->velCurr = spShip->velCurr * glm::pow(SHIP_DAMP_BACKWARD, dt);
        }
        if (game::instance().input_key_pressed(GLFW_KEY_LEFT)) {
            sShipRotSpeed += (SHIP_ROT_SPEED - sShipRotSpeed) * 0.1f;
            spShip->dirCurr += sShipRotSpeed * dt;
            spShip->dirCurr = wrap(spShip->dirCurr, -PI, PI);
        }
        else if (game::instance().input_key_pressed(GLFW_KEY_RIGHT)) {
            sShipRotSpeed += (SHIP_ROT_SPEED - sShipRotSpeed) * 0.1f;
            spShip->dirCurr -= sShipRotSpeed * dt;
            spShip->dirCurr = wrap(spShip->dirCurr, -PI, PI);
        }
        else {
            sShipRotSpeed = 0.0f;
        }
        if (game::instance().input_key_triggered(GLFW_KEY_SPACE)) {
            vec2 vel;

            vel = { glm::cos(spShip->dirCurr), glm::sin(spShip->dirCurr) };
            vel = vel * BULLET_SPEED;

            gameObjInstCreate(TYPE_BULLET, BULLET_SIZE, &spShip->posCurr, &vel, spShip->dirCurr, true);
        }
        // if 'z' pressed
        if (game::instance().input_key_triggered(GLFW_KEY_Z) && (sSpecialCtr >= BOMB_COST)) {
            uint32_t i;

            // make sure there is no bomb is active currently
            for (i = 0; i < GAME_OBJ_INST_NUM_MAX; i++)
                if ((sGameObjInstList[i].flag & FLAG_ACTIVE) &&
                    (sGameObjInstList[i].pObject->type == TYPE_BOMB))
                    break;

            // if no bomb is active currently, create one
            if (i == GAME_OBJ_INST_NUM_MAX) {
                sSpecialCtr -= BOMB_COST;
                gameObjInstCreate(TYPE_BOMB, BOMB_SIZE, &spShip->posCurr, 0, 0, true);
            }
        }
        // if 'x' pressed
        if (game::instance().input_key_pressed(GLFW_KEY_X) && (sSpecialCtr >= MISSILE_COST)) {
            sSpecialCtr -= MISSILE_COST;

            float dir = spShip->dirCurr;
            vec2  vel = spShip->velCurr;
            vec2  pos;

            pos = { glm::cos(spShip->dirCurr), glm::sin(spShip->dirCurr) };
            pos = pos * spShip->scale * 0.5f;
            pos = pos + spShip->posCurr;

            gameObjInstCreate(TYPE_MISSILE, 1.0f, &pos, &vel, dir, true);
        }
    }

    // ==================================
    // create new asteroids if necessary
    // ==================================

    if ((sAstCtr < sAstNum) &&
        ((game::instance().game_time() - sAstCreationTime) > AST_CREATE_DELAY)) {
        // keep track the last time an asteroid is created
        sAstCreationTime = game::instance().game_time();

        // create an asteroid
        astCreate(0);
    }

    // ===============
    // update physics
    // ===============

    for (uint32_t i = 0; i < GAME_OBJ_INST_NUM_MAX; i++) {
        GameObjInst* pInst = sGameObjInstList + i;

        // skip non-active object
        if ((pInst->flag & FLAG_ACTIVE) == 0)
            continue;

        // update the position
        pInst->posCurr += pInst->velCurr * dt;
    }

    // ===============
    // update objects
    // ===============

    for (uint32_t i = 0; i < GAME_OBJ_INST_NUM_MAX; i++) {
        GameObjInst* pInst = sGameObjInstList + i;

        // skip non-active object
        if ((pInst->flag & FLAG_ACTIVE) == 0)
            continue;

        // check if the object is a ship
        if (pInst->pObject->type == TYPE_SHIP) {
            // warp the ship from one end of the screen to the other
            pInst->posCurr.x = wrap(pInst->posCurr.x, gAEWinMinX - SHIP_SIZE, gAEWinMaxX + SHIP_SIZE);
            pInst->posCurr.y = wrap(pInst->posCurr.y, gAEWinMinY - SHIP_SIZE, gAEWinMaxY + SHIP_SIZE);
        }
        // check if the object is an asteroid
        else if (pInst->pObject->type == TYPE_ASTEROID) {
            vec2  u;
            float uLen;

            // warp the asteroid from one end of the screen to the other
            pInst->posCurr.x = wrap(pInst->posCurr.x, gAEWinMinX - AST_SIZE_MAX, gAEWinMaxX + AST_SIZE_MAX);
            pInst->posCurr.y = wrap(pInst->posCurr.y, gAEWinMinY - AST_SIZE_MAX, gAEWinMaxY + AST_SIZE_MAX);

            // pull the asteroid toward the ship a little bit
            if (spShip) {
                // apply acceleration propotional to the distance from the asteroid to
                // the ship
                u = spShip->posCurr - pInst->posCurr;
                u = u * AST_TO_SHIP_ACC * dt;
                pInst->velCurr = pInst->velCurr + u;
            }

            // if the asterid velocity is more than its maximum velocity, reduce its
            // speed
            if ((uLen = glm::length(pInst->velCurr)) > (AST_VEL_MAX * 2.0f)) {
                u = pInst->velCurr * (1.0f / uLen) * (AST_VEL_MAX * 2.0f - uLen) * glm::pow(AST_VEL_DAMP, dt);
                pInst->velCurr = pInst->velCurr + u;
            }
        }
        // check if the object is a bullet
        else if (pInst->pObject->type == TYPE_BULLET) {
            // kill the bullet if it gets out of the screen
            if (!in_range(pInst->posCurr.x, gAEWinMinX - AST_SIZE_MAX, gAEWinMaxX + AST_SIZE_MAX) ||
                !in_range(pInst->posCurr.y, gAEWinMinY - AST_SIZE_MAX, gAEWinMaxY + AST_SIZE_MAX))
                gameObjInstDestroy(pInst);
        }
        // check if the object is a bomb
        else if (pInst->pObject->type == TYPE_BOMB) {
            // adjust the life counter
            pInst->life -= dt / BOMB_LIFE;

            if (pInst->life < 0.0f) {
                gameObjInstDestroy(pInst);
            }
            else {
                float radius = 1.0f - pInst->life;
                vec2  u;

                pInst->dirCurr += 2.0f * PI * dt;

                radius = 1.0f - radius;
                radius *= radius;
                radius *= radius;
                radius *= radius;
                radius *= radius;
                radius = (1.0f - radius) * BOMB_RADIUS;

                // generate the particle ring
                for (uint32_t j = 0; j < 10; j++) {
                    // float dir = frand() * 2.0f * PI;
                    float dir = (j / 9.0f) * 2.0f * PI + pInst->life * 1.5f * 2.0f * PI;

                    u.x = glm::cos(dir) * radius + pInst->posCurr.x;
                    u.y = glm::sin(dir) * radius + pInst->posCurr.y;

                    // sparkCreate(PTCL_EXHAUST, &u, 1, dir + 0.8f * PI, dir + 0.9f * PI);
                    sparkCreate(PTCL_EXHAUST, &u, 1, dir + 0.40f * PI, dir + 0.60f * PI);
                }
            }
        }
        // check if the object is a missile
        else if (pInst->pObject->type == TYPE_MISSILE) {
            // adjust the life counter
            pInst->life -= dt / MISSILE_LIFE;

            if (pInst->life < 0.0f) {
                gameObjInstDestroy(pInst);
            }
            else {
                vec2 dir;

                if (pInst->pUserData == 0) {
                    pInst->pUserData = missileAcquireTarget(pInst);
                }
                else {
                    GameObjInst* pTarget = (GameObjInst*)(pInst->pUserData);

                    // if the target is no longer valid, reacquire
                    if (((pTarget->flag & FLAG_ACTIVE) == 0) ||
                        (pTarget->pObject->type != TYPE_ASTEROID))
                        pInst->pUserData = missileAcquireTarget(pInst);
                }

                if (pInst->pUserData) {
                    GameObjInst* pTarget = (GameObjInst*)(pInst->pUserData);
                    vec2         u;
                    float        uLen;

                    // get the vector from the missile to the target and its length
                    u = pTarget->posCurr - pInst->posCurr;
                    uLen = glm::length(u);

                    // if the missile is 'close' to target, do nothing
                    if (uLen > 0.1f) {
                        // normalize the vector from the missile to the target
                        u = u * 1.0f / uLen;

                        // calculate the missile direction vector
                        dir = { glm::cos(pInst->dirCurr), glm::sin(pInst->dirCurr) };

                        // calculate the cos and sin of the angle between the target
                        // vector and the missile direction vector
                        float cosAngle = glm::dot(dir, u),
                            sinAngle = cross_product_mag(dir, u), rotAngle;

                        // calculate how much to rotate the missile
                        if (cosAngle < glm::cos(MISSILE_TURN_SPEED * dt))
                            rotAngle = MISSILE_TURN_SPEED * dt;
                        else
                            rotAngle = glm::cos(glm::clamp(cosAngle, -1.0f, 1.0f));

                        // rotate to the left if sine of the angle is positive and vice
                        // versa
                        pInst->dirCurr += (sinAngle > 0.0f) ? rotAngle : -rotAngle;
                    }
                }

                // adjust the missile velocity
                dir = { glm::cos(pInst->dirCurr), glm::sin(pInst->dirCurr) };
                dir = dir * MISSILE_ACCEL * dt;
                pInst->velCurr = pInst->velCurr + dir;
                pInst->velCurr = pInst->velCurr * glm::pow(MISSILE_DAMP, dt);

                sparkCreate(PTCL_EXHAUST, &pInst->posCurr, 1, pInst->dirCurr + 0.8f * PI, pInst->dirCurr + 1.2f * PI);
            }
        }
        // check if the object is a particle
        else if ((TYPE_PTCL_WHITE <= pInst->pObject->type) &&
            (pInst->pObject->type <= TYPE_PTCL_RED)) {
            pInst->scale *= pow(PTCL_SCALE_DAMP, dt);
            pInst->dirCurr += 0.1f;
            pInst->velCurr = pInst->velCurr * glm::pow(PTCL_VEL_DAMP, dt);

            if (pInst->scale < PTCL_SCALE_DAMP)
                gameObjInstDestroy(pInst);
        }
    }

    // ====================
    // check for collision
    // ====================
#if 1
    for (uint32_t i = 0; i < GAME_OBJ_INST_NUM_MAX; i++) {
        GameObjInst* pSrc = sGameObjInstList + i;

        // skip non-active object
        if ((pSrc->flag & FLAG_ACTIVE) == 0)
            continue;

        if ((pSrc->pObject->type == TYPE_BULLET) ||
            (pSrc->pObject->type == TYPE_MISSILE)) {
            for (uint32_t j = 0; j < GAME_OBJ_INST_NUM_MAX; j++) {
                GameObjInst* pDst = sGameObjInstList + j;

                // skip no-active and non-asteroid object
                if (((pDst->flag & FLAG_ACTIVE) == 0) ||
                    (pDst->pObject->type != TYPE_ASTEROID))
                    continue;

                if (point_in_aabb(pSrc->posCurr, pDst->posCurr, pDst->scale, pDst->scale) == false)
                    continue;

                if (pDst->scale < AST_SIZE_MIN) {
                    sparkCreate(PTCL_EXPLOSION_M, &pDst->posCurr, (uint32_t)(pDst->scale * 10), pSrc->dirCurr - 0.05f * PI, pSrc->dirCurr + 0.05f * PI, pDst->scale);
                    sScore++;

                    if ((sScore % AST_SPECIAL_RATIO) == 0)
                        sSpecialCtr++;
                    if ((sScore % AST_SHIP_RATIO) == 0)
                        sShipCtr++;
                    if (sScore == sAstNum * 5)
                        sAstNum = (sAstNum < AST_NUM_MAX) ? (sAstNum * 2) : sAstNum;

                    // destroy the asteroid
                    gameObjInstDestroy(pDst);
                }
                else {
                    sparkCreate(PTCL_EXPLOSION_S, &pSrc->posCurr, 10, pSrc->dirCurr + 0.9f * PI, pSrc->dirCurr + 1.1f * PI);

                    // impart some of the bullet/missile velocity to the asteroid
                    pSrc->velCurr = pSrc->velCurr * 0.01f * (1.0f - pDst->scale / AST_SIZE_MAX);
                    pDst->velCurr = pDst->velCurr + pSrc->velCurr;

                    // split the asteroid to 4
                    if ((pSrc->pObject->type == TYPE_MISSILE) ||
                        ((pDst->life -= 1.0f) < 0.0f))
                        astCreate(pDst);
                }

                // destroy the bullet
                gameObjInstDestroy(pSrc);

                break;
            }
        }
        else if (TYPE_BOMB == pSrc->pObject->type) {
            float radius = 1.0f - pSrc->life;

            pSrc->dirCurr += 2.0f * PI * dt;

            radius = 1.0f - radius;
            radius *= radius;
            radius *= radius;
            radius *= radius;
            radius *= radius;
            radius *= radius;
            radius = (1.0f - radius) * BOMB_RADIUS;

            // check collision
            for (uint32_t j = 0; j < GAME_OBJ_INST_NUM_MAX; j++) {
                GameObjInst* pDst = sGameObjInstList + j;

                if (((pDst->flag & FLAG_ACTIVE) == 0) ||
                    (pDst->pObject->type != TYPE_ASTEROID))
                    continue;

                // if (AECalcDistPointToRect(&pSrc->posCurr, &pDst->posCurr,
                // pDst->scale, pDst->scale) > radius)
                if (point_in_sphere(pSrc->posCurr, pDst->posCurr, radius) == false)
                    continue;

                if (pDst->scale < AST_SIZE_MIN) {
                    float dir = atan2f(pDst->posCurr.y - pSrc->posCurr.y,
                        pDst->posCurr.x - pSrc->posCurr.x);

                    gameObjInstDestroy(pDst);
                    sparkCreate(PTCL_EXPLOSION_M, &pDst->posCurr, 20, dir + 0.4f * PI, dir + 0.45f * PI);
                    sScore++;

                    if ((sScore % AST_SPECIAL_RATIO) == 0)
                        sSpecialCtr++;
                    if ((sScore % AST_SHIP_RATIO) == 0)
                        sShipCtr++;
                    if (sScore == sAstNum * 5)
                        sAstNum = (sAstNum < AST_NUM_MAX) ? (sAstNum * 2) : sAstNum;
                }
                else {
                    // split the asteroid to 4
                    astCreate(pDst);
                }
            }
        }
        else if (pSrc->pObject->type == TYPE_ASTEROID) {
            for (uint32_t j = 0; j < GAME_OBJ_INST_NUM_MAX; j++) {
                GameObjInst* pDst = sGameObjInstList + j;
                float        d;
                vec2         nrm, u;

                // skip no-active and non-asteroid object
                if ((pSrc == pDst) || ((pDst->flag & FLAG_ACTIVE) == 0) ||
                    (pDst->pObject->type != TYPE_ASTEROID))
                    continue;

                // check if the object rectangle overlap
                d = aabb_vs_aabb(pSrc->posCurr, pSrc->scale, pSrc->scale, pDst->posCurr, pDst->scale, pDst->scale);
                //&nrm);

                if (d >= 0.0f)
                    continue;

                // adjust object position so that they do not overlap
                u = nrm * d * 0.25f;
                pSrc->posCurr = pSrc->posCurr - u;
                pDst->posCurr = pDst->posCurr + u;

                // calculate new object velocities
                resolveCollision(pSrc, pDst, &nrm);
            }
        }
        else if (pSrc->pObject->type == TYPE_SHIP) {
            for (uint32_t j = 0; j < GAME_OBJ_INST_NUM_MAX; j++) {
                GameObjInst* pDst = sGameObjInstList + j;

                // skip no-active and non-asteroid object
                if ((pSrc == pDst) || ((pDst->flag & FLAG_ACTIVE) == 0) ||
                    (pDst->pObject->type != TYPE_ASTEROID))
                    continue;

                // check if the object rectangle overlap
                if (aabb_vs_aabb(pSrc->posCurr, pSrc->scale, pSrc->scale, pDst->posCurr, pDst->scale, pDst->scale) == false)
                    continue;

                // create the big explosion
                sparkCreate(PTCL_EXPLOSION_L, &pSrc->posCurr, 100, 0.0f, 2.0f * PI);

                // reset the ship position and direction
                spShip->posCurr = {};
                spShip->velCurr = {};
                spShip->dirCurr = 0.0f;

                sSpecialCtr = SHIP_SPECIAL_NUM;

                // destroy all asteroid near the ship so that you do not die as soon as
                // the ship reappear
                for (uint32_t j = 0; j < GAME_OBJ_INST_NUM_MAX; j++) {
                    GameObjInst* pInst = sGameObjInstList + j;
                    vec2         u;

                    // skip no-active and non-asteroid object
                    if (((pInst->flag & FLAG_ACTIVE) == 0) ||
                        (pInst->pObject->type != TYPE_ASTEROID))
                        continue;

                    u = pInst->posCurr - spShip->posCurr;

                    if (glm::length(u) < (spShip->scale * 10.0f)) {
                        sparkCreate(PTCL_EXPLOSION_M, &pInst->posCurr, 10, -PI, PI);
                        gameObjInstDestroy(pInst);
                    }
                }

                // reduce the ship counter
                sShipCtr--;

                // if counter is less than 0, game over
                if (sShipCtr < 0) {
                    sGameStateChangeCtr = 2.0;
                    gameObjInstDestroy(spShip);
                    spShip = 0;
                }

                break;
            }
        }
    }
#endif
    // =====================================
    // calculate the matrix for all objects
    // =====================================

    for (uint32_t i = 0; i < GAME_OBJ_INST_NUM_MAX; i++) {
        GameObjInst* pInst = sGameObjInstList + i;
        glm::mat3    m;

        // skip non-active object
        if ((pInst->flag & FLAG_ACTIVE) == 0)
            continue;

        auto t = glm::translate(vec3(pInst->posCurr.x, pInst->posCurr.y, 0));
        auto r = glm::rotate(pInst->dirCurr, vec3(0, 0, 1));
        auto s = glm::scale(vec3(pInst->scale, pInst->scale, 1));
        pInst->transform = t * r * s;
    }
}

// ---------------------------------------------------------------------------

void GameStatePlayDrawSolo(void)
{
    auto window = game::instance().window();
    gAEWinMinX = -window->size().x * 0.5f;
    gAEWinMaxX = window->size().x * 0.5f;
    gAEWinMinY = -window->size().y * 0.5f;
    gAEWinMaxY = window->size().y * 0.5f;
    mat4 vp = glm::ortho(float(gAEWinMinX), float(gAEWinMaxX), float(gAEWinMinY), float(gAEWinMaxY), 0.01f, 100.0f) * glm::lookAt(vec3(0, 0, 10), vec3(0, 0, 0), vec3(0, 1, 0));

    glViewport(0, 0, window->size().x, window->size().y);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_CULL_FACE);

    char strBuffer[1024];
    mat4 tmp, tmpScale = glm::scale(glm::vec3{ 10, 10, 1 });
    vec4 col;

    // draw all object in the list
    for (uint32_t i = 0; i < GAME_OBJ_INST_NUM_MAX; i++) {
        GameObjInst* pInst = sGameObjInstList + i;
        // skip non-active object
        if ((pInst->flag & FLAG_ACTIVE) == 0)
            continue;

        // if (pInst->pObject->type != TYPE_SHIP) continue;
        tmp = /*tmpScale * */ vp * sGameObjInstList[i].transform;
        col = /*tmpScale * */ vp * sGameObjInstList[i].modColor;

        game::instance().shader_default()->use();
        game::instance().shader_default()->set_uniform(0, tmp);
        game::instance().shader_default()->set_uniform(1, 255.0f * col);
        sGameObjInstList[i].pObject->pMesh->draw();
    }

    // Render text
    auto w = gAEWinMaxX - gAEWinMinX;
    auto h = gAEWinMaxY - gAEWinMinY;
    vp = glm::ortho(float(0), float(gAEWinMaxX - gAEWinMinX), float(0), float(h), 0.01f, 100.0f) * glm::lookAt(vec3(0, 0, 10), vec3(0, 0, 0), vec3(0, 1, 0));

    sprintf(strBuffer, "Score: %d", sScore);
    game::instance().font_default()->render(strBuffer, 10, h - 10, 24, vp);

    sprintf(strBuffer, "Level: %d", glm::log2<uint32_t>(sAstNum));
    game::instance().font_default()->render(strBuffer, 10, h - 30, 24, vp);

    sprintf(strBuffer, "Ship Left: %d", sShipCtr >= 0 ? sShipCtr : 0);
    game::instance().font_default()->render(strBuffer, 600, h - 10, 24, vp);

    sprintf(strBuffer, "Special:   %d", sSpecialCtr);
    game::instance().font_default()->render(strBuffer, 600, h - 30, 24, vp);

    // display the game over message
    if (sShipCtr < 0)
        game::instance().font_default()->render("       GAME OVER       ", 280, 260, 24, vp);
}

// ---------------------------------------------------------------------------

void GameStatePlayFreeSolo(void)
{
    // kill all object in the list
    for (uint32_t i = 0; i < GAME_OBJ_INST_NUM_MAX; i++)
        gameObjInstDestroy(sGameObjInstList + i);

    // reset asteroid count
    sAstCtr = 0;
}

// ---------------------------------------------------------------------------

void GameStatePlayUnloadSolo(void)
{
    // free all mesh
    for (uint32_t i = 0; i < sGameObjNum; i++) {
        delete sGameObjList[i].pMesh;
        sGameObjList[i].pMesh = nullptr;
    }
}

// ---------------------------------------------------------------------------
// Static function implementations

static void loadGameObjList()
{
    GameObj* pObj;

    // ================
    // create the ship
    // ================

    pObj = sGameObjList + sGameObjNum++;
    pObj->type = TYPE_SHIP;

    {
        engine::mesh* mesh = new engine::mesh();
        mesh->add_triangle(engine::gfx_triangle(-0.5f, -0.5f, 0xFFFF0000, 0.0f, 0.0f, 0.5f, 0.0f, 0xFFFFFFFF, 0.0f, 0.0f, -0.5f, 0.5f, 0xFFFF0000, 0.0f, 0.0f));
        mesh->create();
        pObj->pMesh = mesh;
        AE_ASSERT_MESG(pObj->pMesh, "fail to create object!!");
    }

    // ==================
    // create the bullet
    // ==================

    pObj = sGameObjList + sGameObjNum++;
    pObj->type = TYPE_BULLET;

    {
        engine::mesh* mesh = new engine::mesh();
        mesh->add_triangle(engine::gfx_triangle(-1.0f, 0.2f, 0x00FFFF00, 0.0f, 0.0f, -1.0f, -0.2f, 0x00FFFF00, 0.0f, 0.0f, 1.0f, -0.2f, 0xFFFFFF00, 0.0f, 0.0f));
        mesh->add_triangle(engine::gfx_triangle(-1.0f, 0.2f, 0x00FFFF00, 0.0f, 0.0f, 1.0f, -0.2f, 0xFFFFFF00, 0.0f, 0.0f, 1.0f, 0.2f, 0xFFFFFF00, 0.0f, 0.0f));
        mesh->create();
        pObj->pMesh = mesh;
        AE_ASSERT_MESG(pObj->pMesh, "fail to create object!!");
    }

    // ==================
    // create the bomb
    // ==================

    pObj = sGameObjList + sGameObjNum++;
    pObj->type = TYPE_BOMB;

    {
        engine::mesh* mesh = new engine::mesh();
        mesh->add_triangle(engine::gfx_triangle(-1.0f, 1.0f, 0xFFFF8000, 0.0f, 0.0f, -1.0f, -1.0f, 0xFFFF8000, 0.0f, 0.0f, 1.0f, -1.0f, 0xFFFF8000, 0.0f, 0.0f));
        mesh->add_triangle(engine::gfx_triangle(-1.0f, 1.0f, 0xFFFF8000, 0.0f, 0.0f, 1.0f, -1.0f, 0xFFFF8000, 0.0f, 0.0f, 1.0f, 1.0f, 0xFFFF8000, 0.0f, 0.0f));
        mesh->create();
        pObj->pMesh = mesh;
        AE_ASSERT_MESG(pObj->pMesh, "fail to create object!!");
    }

    // ==================
    // create the missile
    // ==================

    pObj = sGameObjList + sGameObjNum++;
    pObj->type = TYPE_MISSILE;

    {
        engine::mesh* mesh = new engine::mesh();
        mesh->add_triangle(engine::gfx_triangle(-1.0f, -0.5f, 0xFFFF0000, 0.0f, 0.0f, 1.0f, 0.0f, 0xFFFFFF00, 0.0f, 0.0f, -1.0f, 0.5f, 0xFFFF0000, 0.0f, 0.0f));
        mesh->create();
        pObj->pMesh = mesh;
        AE_ASSERT_MESG(pObj->pMesh, "fail to create object!!");
    }

    // ====================
    // create the asteroid
    // ====================

    pObj = sGameObjList + sGameObjNum++;
    pObj->type = TYPE_ASTEROID;

    {
        engine::mesh* mesh = new engine::mesh();
        mesh->add_triangle(engine::gfx_triangle(-0.5f, -0.5f, 0xFF808080, 0.0f, 0.0f, 0.5f, 0.5f, 0xFF808080, 0.0f, 0.0f, -0.5f, 0.5f, 0xFF808080, 0.0f, 0.0f));
        mesh->add_triangle(engine::gfx_triangle(-0.5f, -0.5f, 0xFF808080, 0.0f, 0.0f, 0.5f, -0.5f, 0xFF808080, 0.0f, 0.0f, 0.5f, 0.5f, 0xFF808080, 0.0f, 0.0f));
        mesh->create();
        pObj->pMesh = mesh;
        AE_ASSERT_MESG(pObj->pMesh, "fail to create object!!");
    }

    // ====================
    // create the star
    // ====================

    pObj = sGameObjList + sGameObjNum++;
    pObj->type = TYPE_STAR;

    {
        engine::mesh* mesh = new engine::mesh();
        mesh->add_triangle(engine::gfx_triangle(-0.5f, -0.5f, 0xFF8080FF, 0.0f, 0.0f, 0.5f, 0.5f, 0xFF8080FF, 0.0f, 0.0f, -0.5f, 0.5f, 0xFF8080FF, 0.0f, 0.0f));
        mesh->add_triangle(engine::gfx_triangle(-0.5f, -0.5f, 0xFF8080FF, 0.0f, 0.0f, 0.5f, -0.5f, 0xFF8080FF, 0.0f, 0.0f, 0.5f, 0.5f, 0xFF8080FF, 0.0f, 0.0f));
        mesh->create();
        pObj->pMesh = mesh;
        assert(pObj->pMesh && "fail to create object!!");
    }

    // ================
    // create the ptcl
    // ================

    for (uint32_t i = 0; i < 3; i++) {
        uint32_t color =
            (i == 0) ? (0xFFFFFFFF) : ((i == 1) ? (0xFFFFFF00) : (0xFFFF0000));

        pObj = sGameObjList + sGameObjNum++;
        pObj->type = TYPE_PTCL_WHITE + i;

        {
            engine::mesh* mesh = new engine::mesh();
            mesh->add_triangle(engine::gfx_triangle(-1.0f * (3 - i), -0.5f * (3 - i), color, 0.0f, 0.0f, 1.0f * (3 - i), 0.5f * (3 - i), color, 0.0f, 0.0f, -1.0f * (3 - i), 0.5f * (3 - i), color, 0.0f, 0.0f));
            mesh->add_triangle(engine::gfx_triangle(-1.0f * (3 - i), -0.5f * (3 - i), color, 0.0f, 0.0f, 1.0f * (3 - i), -0.5f * (3 - i), color, 0.0f, 0.0f, 1.0f * (3 - i), 0.5f * (3 - i), color, 0.0f, 0.0f));
            mesh->create();
            pObj->pMesh = mesh;
            assert(pObj->pMesh && "fail to create object!!");
        }
    }
}

// ---------------------------------------------------------------------------

GameObjInst* gameObjInstCreate(uint32_t type, float scale, vec2* pPos, vec2* pVel, float dir, bool forceCreate)
{
    vec2 zero = { 0.0f, 0.0f };

    // AE_ASSERT(type < sGameObjNum);

    // loop through the object instance list to find a non-used object instance
    for (uint32_t i = 0; i < GAME_OBJ_INST_NUM_MAX; i++) {
        GameObjInst* pInst = sGameObjInstList + i;

        // check if current instance is not used
        if (pInst->flag == 0) {
            // it is not used => use it to create the new instance
            pInst->pObject = sGameObjList + type;
            pInst->flag = FLAG_ACTIVE;
            pInst->life = 1.0f;
            pInst->scale = scale;
            pInst->posCurr = pPos ? *pPos : zero;
            pInst->velCurr = pVel ? *pVel : zero;
            pInst->dirCurr = dir;
            pInst->pUserData = 0;
            pInst->modColor = glm::vec4(1.0f);

            // keep track the number of asteroid
            if (pInst->pObject->type == TYPE_ASTEROID)
                sAstCtr++;

            // return the newly created instance
            return pInst;
        }
    }

    if (forceCreate) {
        float        scaleMin = FLT_MAX;
        GameObjInst* pDst = 0;

        // loop through the object instance list to find the smallest particle
        for (uint32_t i = 0; i < GAME_OBJ_INST_NUM_MAX; i++) {
            GameObjInst* pInst = sGameObjInstList + i;

            // check if current instance is a red particle
            if ((TYPE_PTCL_RED <= pInst->pObject->type) &&
                (pInst->pObject->type <= TYPE_PTCL_WHITE) &&
                (pInst->scale < scaleMin)) {
                scaleMin = pInst->scale;
                pDst = pInst;
            }
        }

        if (pDst) {
            pDst->pObject = sGameObjList + type;
            pDst->flag = FLAG_ACTIVE;
            pDst->life = 1.0f;
            pDst->scale = scale;
            pDst->posCurr = pPos ? *pPos : zero;
            pDst->velCurr = pVel ? *pVel : zero;
            pDst->dirCurr = dir;
            pDst->pUserData = 0;

            // keep track the number of asteroid
            if (pDst->pObject->type == TYPE_ASTEROID)
                sAstCtr++;

            // return the newly created instance
            return pDst;
        }
    }

    // cannot find empty slot => return 0
    return 0;
}

// ---------------------------------------------------------------------------

void gameObjInstDestroy(GameObjInst* pInst)
{
    // if instance is destroyed before, just return
    if (pInst->flag == 0)
        return;

    // zero out the flag
    pInst->flag = 0;

    // keep track the number of asteroid
    if (pInst->pObject->type == TYPE_ASTEROID)
        sAstCtr--;
}

// ---------------------------------------------------------------------------

GameObjInst* astCreate(GameObjInst* pSrc)
{
    GameObjInst* pInst;
    vec2         pos, vel;
    float        t, angle, size;

    if (pSrc) {
        float posOffset = pSrc->scale * 0.25f;
        float velOffset = (AST_SIZE_MAX - pSrc->scale + 1.0f) * 0.25f;
        float scaleNew = pSrc->scale * 0.5f;

        sparkCreate(PTCL_EXPLOSION_L, &pSrc->posCurr, 5, 0.0f * PI - 0.01f * PI, 0.0f * PI + 0.01f * PI, 0.0f, pSrc->scale / AST_SIZE_MAX, &pSrc->velCurr);
        sparkCreate(PTCL_EXPLOSION_L, &pSrc->posCurr, 5, 0.5f * PI - 0.01f * PI, 0.5f * PI + 0.01f * PI, 0.0f, pSrc->scale / AST_SIZE_MAX, &pSrc->velCurr);
        sparkCreate(PTCL_EXPLOSION_L, &pSrc->posCurr, 5, 1.0f * PI - 0.01f * PI, 1.0f * PI + 0.01f * PI, 0.0f, pSrc->scale / AST_SIZE_MAX, &pSrc->velCurr);
        sparkCreate(PTCL_EXPLOSION_L, &pSrc->posCurr, 5, 1.5f * PI - 0.01f * PI, 1.5f * PI + 0.01f * PI, 0.0f, pSrc->scale / AST_SIZE_MAX, &pSrc->velCurr);

        pInst = astCreate(0);
        pInst->scale = scaleNew;
        pInst->posCurr = { pSrc->posCurr.x - posOffset, pSrc->posCurr.y - posOffset };
        pInst->velCurr = { pSrc->velCurr.x - velOffset, pSrc->velCurr.y - velOffset };

        pInst = astCreate(0);
        pInst->scale = scaleNew;
        pInst->posCurr = { pSrc->posCurr.x + posOffset, pSrc->posCurr.y - posOffset };
        pInst->velCurr = { pSrc->velCurr.x + velOffset, pSrc->velCurr.y - velOffset };

        pInst = astCreate(0);
        pInst->scale = scaleNew;
        pInst->posCurr = { pSrc->posCurr.x - posOffset, pSrc->posCurr.y + posOffset };
        pInst->velCurr = { pSrc->velCurr.x - velOffset, pSrc->velCurr.y + velOffset };

        pSrc->scale = scaleNew;
        pSrc->posCurr = { pSrc->posCurr.x + posOffset, pSrc->posCurr.y + posOffset };
        pSrc->velCurr = { pSrc->velCurr.x + velOffset, pSrc->velCurr.y + velOffset };

        return pSrc;
    }

    // pick a random angle and velocity magnitude
    angle = frand() * 2.0f * PI;
    size = frand() * (AST_SIZE_MAX - AST_SIZE_MIN) + AST_SIZE_MIN;

    // pick a random position along the top or left edge
    if ((t = frand()) < 0.5f)
        pos = { gAEWinMinX + (t * 2.0f) * (gAEWinMaxX - gAEWinMinX), gAEWinMinY - size * 0.5f };
    else
        pos = { gAEWinMinX - size * 0.5f, gAEWinMinY + ((t - 0.5f) * 2.0f) * (gAEWinMaxY - gAEWinMinY) };

    // calculate the velocity vector
    vel = { glm::cos(angle), glm::sin(angle) };
    vel = vel * frand() * (AST_VEL_MAX - AST_VEL_MIN) + AST_VEL_MIN;

    // create the object instance
    pInst = gameObjInstCreate(TYPE_ASTEROID, size, &pos, &vel, 0.0f, true);
    assert(pInst);

    // set the life based on the size
    pInst->life = size / AST_SIZE_MAX * AST_LIFE_MAX;

    return pInst;
}

// ---------------------------------------------------------------------------

void resolveCollision(GameObjInst* pSrc, GameObjInst* pDst, vec2* pNrm)
{
#if COLL_RESOLVE_SIMPLE

    float ma = pSrc->scale * pSrc->scale, mb = pDst->scale * pDst->scale,
        e = COLL_COEF_OF_RESTITUTION;

    if (pNrm->y == 0) // EPSILON)
    {
        // calculate the relative velocity of the 1st object againts the 2nd object
        // along the x-axis
        float velRel = pSrc->velCurr.x - pDst->velCurr.x;

        // if the object is separating, do nothing
        if ((velRel * pNrm->x) >= 0.0f)
            return;

        pSrc->velCurr.x =
            (ma * pSrc->velCurr.x + mb * (pDst->velCurr.x - e * velRel)) /
            (ma + mb);
        pDst->velCurr.x = pSrc->velCurr.x + e * velRel;
    }
    else {
        // calculate the relative velocity of the 1st object againts the 2nd object
        // along the y-axis
        float velRel = pSrc->velCurr.y - pDst->velCurr.y;

        // if the object is separating, do nothing
        if ((velRel * pNrm->y) >= 0.0f)
            return;

        pSrc->velCurr.y =
            (ma * pSrc->velCurr.y + mb * (pDst->velCurr.y - e * velRel)) /
            (ma + mb);
        pDst->velCurr.y = pSrc->velCurr.y + e * velRel;
    }

#else

    float ma = pSrc->scale * pSrc->scale, mb = pDst->scale * pDst->scale,
        e = COLL_COEF_OF_RESTITUTION;
    vec2 u;
    vec2 velSrc, velDst;
    float velRel;

    // calculate the relative velocity of the 1st object againts the 2nd object
    AEVec2Sub(&u, &pSrc->velCurr, &pDst->velCurr);

    // if the object is separating, do nothing
    if (AEVec2DotProduct(&u, pNrm) > 0.0f)
        return;

    // calculate the side vector (pNrm rotated by 90 degree)
    AEVector2Set(&u, -pNrm->y, pNrm->x);

    // tranform the object velocities to the plane space
    velSrc.x = AEVec2DotProduct(&pSrc->velCurr, &u);
    velSrc.y = AEVec2DotProduct(&pSrc->velCurr, pNrm);
    velDst.x = AEVec2DotProduct(&pDst->velCurr, &u);
    velDst.y = AEVec2DotProduct(&pDst->velCurr, pNrm);

    // calculate the relative velocity along the y axis
    velRel = velSrc.y - velDst.y;

    // resolve collision along the Y axis
    velSrc.y = (ma * velSrc.y + mb * (velDst.y - e * velRel)) / (ma + mb);
    velDst.y = velSrc.y + e * velRel;

    // tranform back the velocity from the normal space to the world space
    AEVec2Scale(&pSrc->velCurr, pNrm, velSrc.y);
    AEVec2ScaleAdd(&pSrc->velCurr, &u, &pSrc->velCurr, velSrc.x);
    AEVec2Scale(&pDst->velCurr, pNrm, velDst.y);
    AEVec2ScaleAdd(&pDst->velCurr, &u, &pDst->velCurr, velDst.x);

#endif // COLL_RESOLVE_SIMPLE
}

// ---------------------------------------------------------------------------

void sparkCreate(uint32_t type, vec2* pPos, uint32_t count, float angleMin, float angleMax, float srcSize, float velScale, vec2* pVelInit)
{
    float velRange, velMin, scaleRange, scaleMin;

    if (type == PTCL_EXHAUST) {
        velRange = velScale * 30.0f;
        velMin = velScale * 10.0f;
        scaleRange = 5.0f;
        scaleMin = 2.0f;

        for (uint32_t i = 0; i < count; i++) {
            float t = frand() * 2.0f - 1.0f;
            float dir = angleMin + frand() * (angleMax - angleMin);
            float velMag = velMin + fabs(t) * velRange;
            vec2  vel;

            vel = { glm::cos(dir), glm::sin(dir) };
            vel = vel * velMag;

            if (pVelInit)
                vel = vel + *pVelInit;

            gameObjInstCreate((fabs(t) < 0.2f) ? (TYPE_PTCL_YELLOW) : (TYPE_PTCL_RED),
                t * scaleRange + scaleMin,
                pPos,
                &vel,
                frand() * 2.0f * PI,
                false);
        }
    }
    else if ((PTCL_EXPLOSION_S <= type) && (type <= PTCL_EXPLOSION_L)) {
        if (type == PTCL_EXPLOSION_S) {
            velRange = 500.0f;
            velMin = 200.0f;
            scaleRange = 05.0f;
            scaleMin = 02.0f;
        }
        else if (type == PTCL_EXPLOSION_M) {
            velRange = 1000.0f;
            velMin = 500.0f;
            scaleRange = 05.0f;
            scaleMin = 05.0f;
        }
        else {
            velRange = 1500.0f;
            velMin = 200.0f;
            scaleRange = 10.0f;
            scaleMin = 05.0f;
        }

        velRange *= velScale;
        velMin *= velScale;

        for (uint32_t i = 0; i < count; i++) {
            float dir = angleMin + (angleMax - angleMin) * frand();
            float t = frand();
            float velMag = t * velRange + velMin;
            vec2  vel;
            vec2  pos;

            pos = { pPos->x + (frand() - 0.5f) * srcSize, pPos->y + (frand() - 0.5f) * srcSize };

            vel = { glm::cos(dir), glm::sin(dir) };
            vel = vel * velMag;

            if (pVelInit)
                vel = vel + *pVelInit;

            gameObjInstCreate(
                (t < 0.25f) ? (TYPE_PTCL_WHITE)
                : ((t < 0.50f) ? (TYPE_PTCL_YELLOW) : (TYPE_PTCL_RED)),
                t * scaleRange + scaleMin,
                &pos,
                &vel,
                frand() * 2.0f * PI,
                false);
        }
    }
}

// ---------------------------------------------------------------------------

GameObjInst* missileAcquireTarget(GameObjInst* pMissile)
{
    vec2         dir, u;
    float        uLen, angleMin = glm::cos(0.25f * PI), minDist = FLT_MAX;
    GameObjInst* pTarget = 0;

    dir = { glm::cos(pMissile->dirCurr), glm::sin(pMissile->dirCurr) };

    for (uint32_t i = 0; i < GAME_OBJ_INST_NUM_MAX; i++) {
        GameObjInst* pInst = sGameObjInstList + i;

        if (((pInst->flag & FLAG_ACTIVE) == 0) ||
            (pInst->pObject->type != TYPE_ASTEROID))
            continue;

        u = pInst->posCurr - pMissile->posCurr;
        uLen = glm::length(u);

        if (uLen < 1.0f)
            continue;

        u = u * 1.0f / uLen;

        if (glm::dot(dir, u) < angleMin)
            continue;

        if (uLen < minDist) {
            minDist = uLen;
            pTarget = pInst;
        }
    }

    return pTarget;
}

// ---------------------------------------------------------------------------
