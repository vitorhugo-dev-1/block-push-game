#pragma once

#include <string.h>
#include <defndec.h>
#include <csv.h>

// Returns true if point1's coordinates are equal to point2's
bool CheckCollisionPoints(Vector2 point1, Vector2 point2){
    return (point1.x == point2.x && point1.y == point2.y);
}

// Returns true if block1's coordinates are equal to block2's
void CheckCollisionPortals(Entity *entity, Portals portal, Direction goToDir, int *timer, bool isObj, CSV *map){
    if (
        CheckCollisionPoints(entity->box, entity->spr) &&
        (
            CheckCollisionPoints(entity->box, portal.entrance) ||
            CheckCollisionPoints(entity->box, portal.exit)
        )
    ){
        int portalY = (entity->box.y == portal.entrance.y) ? portal.exit.y : portal.entrance.y;
        int portalX = (entity->box.x == portal.entrance.x) ? portal.exit.x : portal.entrance.x;

        int oldY = (entity->box.y/TILE_SIZE) - ((goToDir == DOWN) - (goToDir == UP));
        int oldX = (entity->box.x/TILE_SIZE) - ((goToDir == RIGHT) - (goToDir == LEFT));

        entity->spr.y = portalY;
        entity->spr.x = portalX;
        entity->box.y = portalY + ((goToDir == DOWN) - (goToDir == UP)) * TILE_SIZE;
        entity->box.x = portalX + ((goToDir == RIGHT) - (goToDir == LEFT)) * TILE_SIZE;
        *timer = TILE_SIZE;

        if (isObj){
            int objX = (int)entity->box.x/TILE_SIZE;
            int objY = (int)entity->box.y/TILE_SIZE;
            
            int newIndex = map->dataIndex[objY][objX];
            int oldIndex = map->dataIndex[oldY][oldX];
            map->dataIndex[oldY][oldX] = newIndex;
            map->dataIndex[objY][objX] = oldIndex;
            strcpy(map->array[oldY][oldX], "00");
            strcpy(map->array[objY][objX], "03");
        }
    }
}