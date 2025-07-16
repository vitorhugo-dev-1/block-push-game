#pragma once

#include <core_defs.h>
#include <types.h>
#include <string.h>
#include <ctype.h>
#include <util.h>

// Arrays of objects per tile type with length
typedef struct {
    char ***array;
    int  **dataIndex;
    void *objects[TILE_END];
    int  rows, cols;
    unsigned int length[TILE_END];
} Map;

// Calculates how many of the game's objects are present in the informed .csv file
// and returns their length in the form of a Map type variable
Map CountMapObjectsInCSV(const char *fileName){
    FILE *file = fopen(fileName, "r");
    if (!file) Error("Unable to open file: %s", fileName);

    Map map = { 0 };
    map.rows = map.cols = 1;
    char ch, element[CELL_SIZE + 1] = { 0 };
    int charCount = 0;

    while ((ch = fgetc(file)) != EOF){
        if (ch == ','){
            map.cols++;
            continue;
        } else if (ch == '\n'){
            map.cols = 1;
            map.rows++;
            continue;
        }

        element[charCount] = ch;
        charCount++;

        if (charCount != CELL_SIZE) continue;

        int objectType = isdigit(*element) ? atoi(element) : GetTileType(element[0]);
        map.length[objectType]++;
        charCount = 0;
    }
    fclose(file);

    return map;
}

// Calculates how many bytes of memory the informed map will require
size_t CalcMapMemorySize(const Map map, const size_t structSize[]){
    size_t size = 0;

    // Calculate final memory size of CSV array
    size += map.rows * sizeof(char **);
    size += map.rows * map.cols * sizeof(char *);
    size += map.rows * map.cols * (CELL_SIZE + 1) * sizeof(char);

    // Calculate final memory size of CSV dataIndex
    size += map.rows * sizeof(int *); // 
    size += map.rows * map.cols * sizeof(int);

    // Account for individual game objects
    for (int i = 1; i < TILE_END; i++){
        size += map.length[i] * structSize[i];
    }
    return size;
}

// Loads all data inside a Map into their specific arrays
void FillMapObjectArrays(Map *map){
    int iterator[TILE_END] = { [0 ... TILE_END - 1] = 0 };
    char ***array = map->array;

    for (int y = 0; y < map->rows; y++){
        for (int x = 0; x < map->cols; x++){
            Vector2 position = { x * TILE_SIZE, y * TILE_SIZE };

            int objectType = isdigit(*array[y][x]) ? atoi(array[y][x]) : GetTileType(array[y][x][0]);
            int iteration = iterator[objectType];

            if (iteration >= map->length[objectType]){
                Error("Index for %d out of bounds: %d >= %d", objectType, iteration, map->length[objectType]);
            }

            switch (objectType){
                case TILE_PLAYER: case TILE_CRATE:{
                    Entity *entities = (Entity *)map->objects[objectType];
                    entities[iteration] = (Entity){ position, position };
                    break;
                }
                case TILE_WALL: case TILE_PUDDLE:{
                    Vector2 *vectors = (Vector2 *)map->objects[objectType];
                    vectors[iteration] = position;
                    break;
                }
                case TILE_DOOR: case TILE_KEY:{
                    Item *items = (Item *)map->objects[objectType];
                    items[iteration] = (Item){ position, false };
                    break;
                }
                case TILE_CONVEYOR:{
                    DirItem *conveyors = (DirItem *)map->objects[objectType];
                    conveyors[iteration] = (DirItem){ position, (Direction)(array[y][x][1] - '0') };
                    break;
                }
                case TILE_PORTAL:{
                    Portals *portals = (Portals *)map->objects[objectType];
                    int index = map->array[y][x][1] - '0';

                    if (portals[index].exit.x != -1 || portals[index].exit.y != -1){
                        portals[index] = (Portals){ position, {-1, -1} };
                    } else {
                        portals[index].exit = position;
                    }
                    map->dataIndex[y][x] = index;
                    iterator[objectType]++;

                    continue;
                    break;
                }
            }

            map->dataIndex[y][x] = iterator[objectType];
            iterator[objectType]++;
        }
    }
}

// Checks if the informed position is inside the informed map
bool IsWithinMapBounds(Vector2 position, Map map){
    return (
        position.x >= 0 &&
        position.y >= 0 &&
        position.x < map.cols &&
        position.y < map.rows
    );
}

typedef bool (*ConditionFunc)(Map *map, int x, int y);
typedef void (*ActionFunc)(Map *map, Vector2 pos);

static inline bool IsPuddle(Map *map, int x, int y){
    return (atoi(map->array[y][x]) == TILE_PUDDLE);
}

static inline bool IsZapped(Map *map, int x, int y){
    return (map->array[y][x][1] == 'F');
}

void ApplyToAdjacentCells(Map *map, int x, int y, ConditionFunc condition, ActionFunc action){
    static const int directions[4][2] = { {0, 1}, {1, 0}, {0, -1}, {-1, 0} };

    for (int i = 0; i < 4; i++){
        int newX = x + directions[i][0];
        int newY = y + directions[i][1];
        Vector2 position = { newX, newY };

        if (IsWithinMapBounds(position, *map) && condition(map, newX, newY)){ 
            action(map, position);
        }
    }
}

void ZapWater(Map *map, Vector2 pos){
    int x = pos.x, y = pos.y;
    if (!IsPuddle(map, x, y)) return;

    map->array[y][x][1] = 'F';
    DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, YELLOW);

    ApplyToAdjacentCells(map, x, y, IsPuddle, ZapWater);
}

void RestoreWater(Map *map, Vector2 pos){
    int x = pos.x, y = pos.y;
    if (!IsZapped(map, x, y)) return;

    map->array[y][x][1] = TILE_PUDDLE + '0';
    DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, YELLOW);

    ApplyToAdjacentCells(map, x, y, IsZapped, RestoreWater);
}