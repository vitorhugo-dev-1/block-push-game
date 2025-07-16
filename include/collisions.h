#pragma once

#include <core_defs.h>
#include <types.h>
#include <string.h>
#include <map_data.h>

// Returns true if a's coordinates are equal to b's
static inline bool CheckCollisionPoints(Vector2 a, Vector2 b){
    return a.x == b.x && a.y == b.y;
}

// Returns true if block1's coordinates are equal to block2's
void HandleCollisionPortals(Map *map, Entity *entity, const Portals portal, MoveParams *move, bool isObj){
    if (!CheckCollisionPoints(entity->box, entity->spr)) return;

    bool atEntrance = CheckCollisionPoints(entity->box, portal.entrance);
    bool atExit     = CheckCollisionPoints(entity->box, portal.exit    );
    if (!atEntrance && !atExit) return;

    Vector2 target = (atEntrance ? portal.exit : portal.entrance);

    int deltaY = GetDirectionDelta(move->dir, false);
    int deltaX = GetDirectionDelta(move->dir, true );
    int oldY = (entity->box.y / TILE_SIZE) - deltaY;
    int oldX = (entity->box.x / TILE_SIZE) - deltaX;

    entity->spr = entity->box = target;
    entity->box.y += deltaY * TILE_SIZE;
    entity->box.x += deltaX * TILE_SIZE;
    move->timer = TILE_SIZE;

    if (!isObj) return;

    int newX = entity->box.x / TILE_SIZE;
    int newY = entity->box.y / TILE_SIZE;

    int oldIndex = map->dataIndex[oldY][oldX];
    map->dataIndex[oldY][oldX] = map->dataIndex[newY][newX];
    map->dataIndex[newY][newX] = oldIndex;

    memcpy(map->array[oldY][oldX], "00", CELL_SIZE + 1);
    memcpy(map->array[newY][newX], "03", CELL_SIZE + 1);
}

// Moves an Entity across the level
void MoveEntity(Entity *entity, int speed){
    if (entity->spr.y != entity->box.y){
        entity->spr.y += CompareF(entity->box.y, entity->spr.y) * speed;
    } else if (entity->spr.x != entity->box.x){
        entity->spr.x += CompareF(entity->box.x, entity->spr.x) * speed;
    }
}

// Moves all entities across the level
void MoveAllEntities(Map *map, MoveParams *move){
    Entity *players = (Entity *)map->objects[TILE_PLAYER];
    Entity *crates  = (Entity *)map->objects[TILE_CRATE];

    if (move->timer > 0){
        for (unsigned int i = 0; i < map->length[TILE_CRATE]; i++){
            MoveEntity(&crates[i], move->speed);
        }

        MoveEntity(&players[0], move->speed);
        move->timer -= move->speed;
        return;
    }

    players[0].spr = players[0].box;

    for (unsigned int i = 0; i < map->length[TILE_CRATE]; i++){
        if (!CheckCollisionPoints(crates[i].box, crates[i].spr)){
            crates[i].box = crates[i].spr;
        }
    }
    move->timer = 0;
}

// Handles collisions logic that happens before the Entity sprites start moving
void HandlePreMoveCollisions(TileInfo *tile, Map *map, UIElement *keysUI, MoveParams *move){
    Entity *players = (Entity *)map->objects[TILE_PLAYER];
    Entity *crates  = (Entity *)map->objects[TILE_CRATE ];
    Item   *doors   = (Item   *)map->objects[TILE_DOOR  ];

    bool halt = false;
    int x = tile->x;
    int y = tile->y;

    // Testing map boundaries
    if (!IsWithinMapBounds((Vector2){ x, y }, *map)){
        players[0].box = players[0].spr;
        move->timer = 0;
        return;
    }

    memcpy(tile->code, map->array[y][x], CELL_SIZE + 1);
    tile->index = map->dataIndex[y][x];

    int tileID = isdigit(*tile->code) ? atoi(tile->code) : -1;
    int index = tile->index;

    switch (tileID){
        case TILE_WALL:{
            halt = true;
            break;
        }
        case TILE_CRATE:{
            if (!CheckCollisionPoints(crates[index].box, players[0].box)) break;

            if (!CheckCollisionPoints(players[0].box, players[0].spr)){
                float deltaX = GetDirectionDelta(move->dir, true ) * TILE_SIZE;
                float deltaY = GetDirectionDelta(move->dir, false) * TILE_SIZE;

                crates[index].box.x += deltaX;
                crates[index].box.y += deltaY;
            }

            int crateY = crates[index].box.y / TILE_SIZE;
            int crateX = crates[index].box.x / TILE_SIZE;
            
            if (!IsWithinMapBounds((Vector2){ crateX, crateY }, *map)){
                halt = true;
                crates[index].box = crates[index].spr;
                return;
            };

            int posID = atoi(map->array[crateY][crateX]);
            char posCode = map->array[crateY][crateX][0];

            if (posCode == 'C' || (posID != TILE_EMPTY && posID != TILE_PORTAL)){
                halt = true;
                crates[index].box = crates[index].spr;
                break;
            }

            if (posCode == 'P') return;

            map->dataIndex[y][x] = map->dataIndex[crateY][crateX];
            map->dataIndex[crateY][crateX] = index;
            memcpy(map->array[y][x], "00", CELL_SIZE + 1);
            memcpy(map->array[crateY][crateX], "03", CELL_SIZE + 1);
            break;
        }
        case TILE_DOOR:{
            if (doors[index].isActive) break;

            if (!keysUI->value){
                halt = true;
                break;
            }

            doors[index].isActive = true;
            (keysUI->value)--;
            snprintf(keysUI->text, sizeof(keysUI->text), "Keys: %d", keysUI->value);
            break;
        }
    }

    if (halt){
        players[0].box = players[0].spr;
        move->timer = 0;
    }

    tile->index = map->dataIndex[y][x];
}

void HandlePostMoveCollisions(TileInfo *tile, Map *map, UIElement *keysUI, MoveParams *move){
    Entity  *players = (Entity  *)map->objects[TILE_PLAYER];
    Portals *portals = (Portals *)map->objects[TILE_PORTAL];
    Entity  *crates  = (Entity  *)map->objects[TILE_CRATE ];

    // Collide with portals
    if (tile->code[0] == 'P'){
        HandleCollisionPortals(map, &players[0], portals[tile->index], move, false);
    }

    for (int i = 0; i < map->length[TILE_CRATE]; i++){
        int x = crates[i].box.x / TILE_SIZE;
        int y = crates[i].box.y / TILE_SIZE;

        if (map->array[y][x][0] == 'P'){
            int index = map->dataIndex[y][x];
            HandleCollisionPortals(map, &crates[i], portals[index], move, true);
        }
    }

    // Collision testing conveyor belt
    if (tile->code[0] == 'C' && !move->timer){
        move->dir = (Direction)(tile->code[1] - '0');
        players[0].box.y += GetDirectionDelta(move->dir, false) * TILE_SIZE;
        players[0].box.x += GetDirectionDelta(move->dir, true ) * TILE_SIZE;
        move->timer = TILE_SIZE;
    }

    // Collect key
    Item *keys = (Item *)map->objects[TILE_KEY];
    if (
        atoi(tile->code) == TILE_KEY &&
        !keys[tile->index].isActive &&
        CheckCollisionPoints(players[0].spr, keys[tile->index].position)
    ){
        keys[tile->index].isActive = true;
        keysUI->value++;
        snprintf(keysUI->text, sizeof(keysUI->text), "Keys: %d", keysUI->value);
    }
}

void MoveAndCollide(TileInfo *tile, Map *map, UIElement *keysUI, MoveParams *move){
    HandlePreMoveCollisions(tile, map, keysUI, move);
    MoveAllEntities(map, move);
    HandlePostMoveCollisions(tile, map, keysUI, move);
}