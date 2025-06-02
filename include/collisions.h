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

void Slide(Entity *entity, int speed){
    if (entity->spr.y != entity->box.y) {
        entity->spr.y += IsHigherOrLower(entity->box.y, entity->spr.y) * speed;
    } else if (entity->spr.x != entity->box.x) {
        entity->spr.x += IsHigherOrLower(entity->box.x, entity->spr.x) * speed;
    }
}

void Move(void **data, int *timer, int speed, int dataLength[]){
    Entity *players = (Entity *)data[PLAYER];
    Entity *crates = (Entity *)data[CRATE];

    if (*timer > 0){
        //Slide crate
        for (int i = 0; i < dataLength[CRATE]; i++){
            Slide(&crates[i], speed);
        }

        //Slide player
        Slide(&players[0], speed);
        *timer -= speed;
    } else {
        players[0].spr = players[0].box;

        for (int i = 0; i < dataLength[CRATE]; i++){
            if (!CheckCollisionPoints(crates[i].box, crates[i].spr)){
                crates[i].box = crates[i].spr;
            }
        }
        *timer = 0;
    }
}

void HandlePreCollisions(TileInfo *tile, CSV *map, void **data, UI_Element *keysUI, int *timer){
    // Collision testing map boundaries
    Entity *players = (Entity *)data[PLAYER];
    if (!IsInsideMap((Vector2){ tile->x, tile->y }, *map, false)) {
        players[0].box = players[0].spr;
        *timer = 0;
        return;
    }

    //
    strcpy(tile->code, map->array[tile->y][tile->x]);
    tile->index = map->dataIndex[tile->y][tile->x];

    if (isdigit(*tile->code)){
        switch (atoi(tile->code)){
            case WALL:
                players[0].box = players[0].spr;
                *timer = 0;
                break;

            case CRATE:
                Entity    *crates    =  (Entity *)data[CRATE];
                if (CheckCollisionPoints(crates[tile->index].box, players[0].box)) {
                    if (!CheckCollisionPoints(players[0].box, players[0].spr)) {
                        crates[tile->index].box.y += IsHigherOrLower(players[0].box.y, players[0].spr.y) * TILE_SIZE;
                        crates[tile->index].box.x += IsHigherOrLower(players[0].box.x, players[0].spr.x) * TILE_SIZE;
                    }

                    int crateY = crates[tile->index].box.y / TILE_SIZE;
                    int crateX = crates[tile->index].box.x / TILE_SIZE;
                    bool isInsideBounds = IsInsideMap((Vector2){ crateX, crateY }, *map, false);
                    int cratePosition = (isInsideBounds ? atoi(map->array[crateY][crateX]) : -1);

                    if (!isInsideBounds || map->array[crateY][crateX][0] == 'C' ||
                        (cratePosition != EMPTY && cratePosition != PORTAL)) {
                        players[0].box = players[0].spr;
                        crates[tile->index].box = crates[tile->index].spr;
                        *timer = 0;
                    } else {
                        if (map->array[crateY][crateX][0] != 'P') {
                            map->dataIndex[tile->y][tile->x] = map->dataIndex[crateY][crateX];
                            map->dataIndex[crateY][crateX] = tile->index;
                            strcpy(map->array[tile->y][tile->x], "00");
                            strcpy(map->array[crateY][crateX], "03");
                        }
                    }
                }
                break;
        }
    }

    tile->index = map->dataIndex[tile->y][tile->x];

    // Collision testing door
    Item *doors = (Item *)data[DOOR];
    if (atoi(tile->code) == DOOR && !doors[tile->index].active &&
        CheckCollisionPoints(players[0].box, doors[tile->index].position)) {
        if (keysUI->counter) {
            doors[tile->index].active = true;
            (keysUI->counter)--;
            snprintf(keysUI->display, sizeof(keysUI->display), "Keys: %d", keysUI->counter);
        } else {
            players[0].box = players[0].spr;
            *timer = 0;
        }
    }
}

void HandlePostCollisions(
    TileInfo *tile, CSV *map,
    void **data, int dataLength[], Direction *goToDir,
    UI_Element *keysUI, int *timer
){
    Entity *players = (Entity *)data[PLAYER];
    Portals *portals = (Portals *)data[PORTAL];
    Entity *crates = (Entity *)data[CRATE];

    // Collide with portals
    if (tile->code[0] == 'P') CheckCollisionPortals(&players[0], portals[tile->index], *goToDir, timer, false, map);
    for (int i = 0; i < dataLength[CRATE]; i++){
        if (map->array[(int)crates->box.y / TILE_SIZE][(int)crates->box.x / TILE_SIZE][0] == 'P'){
            int index = map->dataIndex[(int)crates->box.y / TILE_SIZE][(int)crates->box.x / TILE_SIZE];
            CheckCollisionPortals(&crates[i], portals[index], *goToDir, timer, true, map);
        }
    }

    //Collision testing conveyor belt
    if (tile->code[0] == 'C' && !*timer){
        *goToDir = (Direction)(tile->code[1]-'0');
        players[0].box.y += (DetectDestinationDir(*goToDir, false) * TILE_SIZE);
        players[0].box.x += (DetectDestinationDir(*goToDir, true) * TILE_SIZE);
        *timer = TILE_SIZE;
    }

    //Collect key
    Item *keys = (Item *)data[KEY];
    if (
        atoi(tile->code) == KEY &&
        !keys[tile->index].active &&
        CheckCollisionPoints(players[0].spr, keys[tile->index].position)
    ){
        keys[tile->index].active = true;
        keysUI->counter++;
        snprintf(keysUI->display, sizeof(keysUI->display), "Keys: %d", keysUI->counter);
    }
}

void MoveAndCollide(TileInfo *currentTile, CSV *map, void **data, int dataLength[], UI_Element *keysUI, Direction *goToDir, int speed, int *timer){
    HandlePreCollisions(currentTile, map, data, keysUI, timer);
    Move(data, timer, speed, dataLength);
    HandlePostCollisions(currentTile, map, data, dataLength, goToDir, keysUI, timer);
}