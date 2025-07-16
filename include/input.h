#pragma once

#include <core_defs.h>
#include <types.h>
#include <raylib.h>
#include <map_data.h>

// Returns the first pressed key from the list, or KEY_NULL if none are pressed
int IsAnyOfKeysPressed(const int *keys, size_t length){
    for (size_t i = 0; i < length; i++){
        if (IsKeyDown(keys[i])) return keys[i];
    }
    return KEY_NULL;
}

// Initiates movement for an entity based on the pressed key
void InitiateMovement(int keyPressed, MoveParams *move, Entity *entity){
    switch (keyPressed){
        case KEY_W: case KEY_UP:    move->dir = DIR_UP;    break;
        case KEY_S: case KEY_DOWN:  move->dir = DIR_DOWN;  break;
        case KEY_A: case KEY_LEFT:  move->dir = DIR_LEFT;  break;
        case KEY_D: case KEY_RIGHT: move->dir = DIR_RIGHT; break;
        default:                    move->dir = DIR_NONE;  return;
    }

    entity->box.y += GetDirectionDelta(move->dir, false) * TILE_SIZE;
    entity->box.x += GetDirectionDelta(move->dir, true ) * TILE_SIZE;
    move->timer = TILE_SIZE;
}

// Spawns a projectile in the first available slot
void ShootProjectile(DirItem *projectiles, Vector2 position, Direction direction){
    for (unsigned int i = 0; i < MAX_SHOTS; i++){
        if  (projectiles[i].dir) continue;
        projectiles[i].position = position;
        projectiles[i].dir      = direction;
        return;
    }
}

void HandleProjectiles(Map map, DirItem *projectiles){
    for (unsigned int i = 0; i < MAX_SHOTS; i++){
        DirItem *proj = &projectiles[i];
        if  (!proj->dir) continue;

        Vector2 shotTile = { SnapToTile(proj->position.x), SnapToTile(proj->position.y) };
        int PosX = shotTile.x, PosY = shotTile.y;

        if (!IsWithinMapBounds(shotTile, map)){
            proj->dir = DIR_NONE;
            continue;
        }

        int tileCode = atoi(map.array[PosY][PosX]);

        if (tileCode == TILE_PUDDLE){
            proj->dir = DIR_NONE;
            ZapWater(&map, shotTile);
            RestoreWater(&map, shotTile);
            continue;
        }

        if (
            tileCode != TILE_PUDDLE &&
            tileCode != TILE_EMPTY  &&
            tileCode != TILE_PLAYER
        ){
            proj->dir = DIR_NONE;
            continue;
        }

        DrawCircleV(proj->position, 15.0f, YELLOW);
        proj->position.x += GetDirectionDelta(proj->dir, true ) * 30.0f;
        proj->position.y += GetDirectionDelta(proj->dir, false) * 30.0f;
    }
}