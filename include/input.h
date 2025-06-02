#pragma once

#include <raylib.h>
#include <defndec.h>
#include <csv.h>

//Checks if any of the passed keys are pressed
int IsAnyOfKeysPressed(int *keys){
    for (int i = 0; keys[i] != -1; i++){
        if (IsKeyDown(keys[i])) return keys[i];
    }
    return 0;
}

void InitiateMove(int keyPressed, Direction *goToDir, int *timer, Entity *entity){
    switch (keyPressed){
        case KEY_W: case KEY_UP:    { *goToDir = UP;    break; }
        case KEY_S: case KEY_DOWN:  { *goToDir = DOWN;  break; }
        case KEY_A: case KEY_LEFT:  { *goToDir = LEFT;  break; }
        case KEY_D: case KEY_RIGHT: { *goToDir = RIGHT; break; }
        default: *goToDir = NOWHERE; break;
    }

    entity->box.y += (DetectDestinationDir(*goToDir, false) * TILE_SIZE);
    entity->box.x += (DetectDestinationDir(*goToDir, true)  * TILE_SIZE);
    *timer = TILE_SIZE;
}

void ShootProjectile(DirItem *projectiles, Vector2 position, Direction direction){
    for (int i = 0; i < MAX_SHOTS; i++){
        if  (projectiles[i].dir) continue;

        projectiles[i].position = position;
        projectiles[i].dir      = direction;

        break;
    }
}

void HandleProjectiles(CSV map, DirItem *projectiles){
    for (int i = 0; i < MAX_SHOTS; i++){
        if  (!projectiles[i].dir) continue;

        Vector2 shotTile = {
            SnapToTile(projectiles[i].position.x),
            SnapToTile(projectiles[i].position.y)
        };
        int PosX = shotTile.x, PosY = shotTile.y;

        if (!IsInsideMap(shotTile, map, true)){
            projectiles[i].dir = NOWHERE;
            PosX = PosY = 0;
            continue;
        }

        int currentCode = atoi(map.array[PosY][PosX]);

        if (currentCode == PUDDLE){
            projectiles[i].dir = NOWHERE;
            zapWater(&map, shotTile);
            restoreWater(&map, shotTile);
            continue;
        }

        if (currentCode != PUDDLE && currentCode != EMPTY && currentCode != PLAYER){
            projectiles[i].dir = NOWHERE;
            continue;
        }

        DrawCircleV(projectiles[i].position, 15, YELLOW);
        projectiles[i].position.x += (DetectDestinationDir(projectiles[i].dir, true)  * 30);
        projectiles[i].position.y += (DetectDestinationDir(projectiles[i].dir, false) * 30);
    }
}