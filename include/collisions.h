#pragma once

#include <string.h>
#include <defndec.h>
#include <csv.h>

// Returns true if point1's coordinates are equal to point2's
bool CheckCollisionPoints(Vector2 point1, Vector2 point2){
    return (point1.x == point2.x && point1.y == point2.y);
}

// Returns true if block1's coordinates are equal to block2's
void CheckCollisionPortals(
    Entity *entity, const Portals portal, const Direction goToDir, int *timer, bool isObj, CSV *map
){
    bool isCollidingEntrance = CheckCollisionPoints(entity->box, portal.entrance);
    bool isCollidingExit = CheckCollisionPoints(entity->box, portal.exit);

    if (CheckCollisionPoints(entity->box, entity->spr) && (isCollidingEntrance || isCollidingExit)){
        Vector2 targetPosition = (isCollidingEntrance ? portal.exit : portal.entrance);

        int offsetY = DetectDestinationDir(goToDir, false);
        int offsetX = DetectDestinationDir(goToDir, true);
        int prevPosY = (entity->box.y / TILE_SIZE) - offsetY;
        int prevPosX = (entity->box.x / TILE_SIZE) - offsetX;

        entity->spr = entity->box = targetPosition;
        entity->box.y += offsetY * TILE_SIZE;
        entity->box.x += offsetX * TILE_SIZE;
        *timer = TILE_SIZE;

        if (isObj){
            int newPosX = (int)entity->box.x / TILE_SIZE;
            int newPosY = (int)entity->box.y / TILE_SIZE;

            int oldIndex = map->dataIndex[prevPosY][prevPosX];
            map->dataIndex[prevPosY][prevPosX] = map->dataIndex[newPosY][newPosX];
            map->dataIndex[newPosY][newPosX] = oldIndex;
            
            strcpy(map->array[prevPosY][prevPosX], "00");
            strcpy(map->array[newPosY][newPosX], "03");
        }
    }
}