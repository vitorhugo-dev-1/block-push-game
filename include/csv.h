#pragma once

#include <string.h>
#include <ctype.h>
#include <defndec.h>
#include <util.h>
#include <marena.h>

typedef struct CSV {char ***array; int rows, cols; int ** dataIndex;} CSV;

//Calculates how many elements are present in a CSV file and
//how many of each object are present in that same CSV
void calcLengthsCSV(const char *fileName, CSV *csv, int cellSize, int dataLength[]){
    FILE *file = fopen(fileName, "r");
    if (file == NULL) Error("Unable to open file\n");

    char element[cellSize];
    int counter = 0;
    char ch;
    csv->cols = csv->rows = 1;
    while ((ch = fgetc(file)) != EOF){
        if (ch == ','){
            csv->cols++;
        } else if (ch == '\n'){
            csv->cols = 1;
            csv->rows++;
        } else {
            element[counter] = ch;
            counter++;
            if (counter != cellSize) continue;
            if (isdigit(*element)){
                dataLength[atoi(element)]++;
            } else {
                switch (element[0]){
                    case 'C':
                        dataLength[CONVEYOR]++;
                        break;
                    case 'P':
                        dataLength[PORTAL]++;
                        break;
                }
            }
            counter = 0;
        }
    }
    fclose(file);
}

//Calculates how many bytes of memory the map data will use
size_t ArenaCalcMapMemorySize(CSV map, const int cellSize, const int dataLength[], const int structSize[]){
    size_t size = (map.rows * sizeof(char **)) +
                  (map.rows * (map.cols * sizeof(char *))) +
                  (map.rows * map.cols * (cellSize + 1) * sizeof(char))
                  +
                  (map.rows * sizeof(int *)) +
                  (map.rows * (map.cols * sizeof(int)));

    for (int i = 1; i < END; i++){
        size += dataLength[i] * structSize[i];
    }
    return size;
}

//Function to generate a 2D array with csv cell values for the CSV struct
void GenerateArrayFromCSV(const char *fileName, CSV *csv, int cellSize, MemoryArena *arena){
    FILE *file = fopen(fileName, "r");
    if (file == NULL) Error("Unable to open file\n");

    //Allocate memory
    csv->array = (char ***)ArenaReserveMemory(arena, csv->rows * sizeof(char **));
    csv->dataIndex = (int **)ArenaReserveMemory(arena, csv->rows * sizeof(int *));
    for (int i = 0; i < csv->rows; i++){
        csv->array[i] = (char **)ArenaReserveMemory(arena, csv->cols * sizeof(char *));
        csv->dataIndex[i] = (int *)ArenaReserveMemory(arena, csv->cols * sizeof(int));
        for (int j = 0; j < csv->cols; j++){
            csv->array[i][j] = (char *)ArenaReserveMemory(arena, (cellSize + 1) * sizeof(char));
            memset(csv->array[i][j], 0, (cellSize + 1) * sizeof(char));
        }
    }

    //Fill array
    char ch;
    int digit = 0;
    csv->cols = csv->rows = 0;
    while ((ch = fgetc(file)) != EOF){
        if (ch == ','){
            csv->cols++;
            digit = 0;
        } else if (ch == '\n'){
            csv->cols = 0;
            csv->rows++;
            digit = 0;
        } else {
            csv->array[csv->rows][csv->cols][digit] = ch;
            if (digit == cellSize - 1){
                csv->array[csv->rows][csv->cols][digit + 1] = '\0';
            }
            digit++;
        }
    }
    csv->rows++;
    csv->cols++;

    fclose(file);
}

//Loads into an array a map object that's not directly linked to any other object in the same map
void LoadSingularObject(void *data[], const int objectType, const int iteration, Vector2 position){
    Entity *players = (Entity *)data[PLAYER];
    Vector2 *walls = (Vector2 *)data[WALL];
    Entity *crates = (Entity *)data[CRATE];
    Item *doors = (Item *)data[DOOR];
    Item *keys = (Item *)data[KEY];
    Vector2 *puddles = (Vector2 *)data[PUDDLE];

    switch (objectType){
        case PLAYER:{
            Entity newPlayer = {position, position};
            players[iteration] = newPlayer;
            break;
        }
        case WALL:{
            Vector2 newWall = position;
            walls[iteration] = newWall;
            break;
        }
        case CRATE:{
            Entity newCrate = {position, position};
            crates[iteration] = newCrate;
            break;
        }
        case DOOR:{
            Item newDoor = {position, false};
            doors[iteration] = newDoor;
            break;
        }
        case KEY:{
            Item newKey = {position, false};
            keys[iteration] = newKey;
            break;
        }
        case PUDDLE:{
            Vector2 newPuddle = position;
            puddles[iteration] = newPuddle;
            break;
        }
    }
}

//Loads into an array a map object that's not directly linked to any other object in the same map
void LoadGroupedObject(void *data[], const char objectType[3], const int iteration, Vector2 position){
    DirItem *conveyors = (DirItem *)data[CONVEYOR];

    switch (objectType[0]){
        case 'C':{
            DirItem newConveyor = {position, (Direction)(objectType[1] - '0')};
            conveyors[iteration] = newConveyor;
            break;
        }
    }
}

//Loads into an array a map object that's directly linked to another one in the same map
void LoadLinkedObject(void *data[], const char *objectType, int *dataIndex, int iterator[], const int dataLength[], Vector2 position){
    Portals *portals = (Portals *)data[PORTAL];

    int index = objectType[1] - '0';
    *dataIndex = index;
    switch (objectType[0]){
        case 'P':
            if (iterator[PORTAL] > dataLength[PORTAL]) Error("Array index out of bounds\n");
            Portals newPortal = {position, {-1, -1}};
            iterator[PORTAL]++;

            if (!iterator[PORTAL]){
                portals[iterator[PORTAL]] = newPortal;
                return;
            }

            if (portals[index].exit.x == -1 || portals[index].exit.y == -1){
                portals[index].exit.x = position.x;
                portals[index].exit.y = position.y;
            } else {
                portals[index] = newPortal;
            }
            break;
    }
}

//Loads all data inside a CSV into their specific arrays
void PopulateData(void *data[], const CSV *map, const int dataLength[]){
    int iterator[END];
    memset(iterator, 0, sizeof(iterator));

    for (int y = 0; y < map->rows; y++){
        for (int x = 0; x < map->cols; x++){
            Vector2 position = {x * (float)TILE_SIZE, y * (float)TILE_SIZE};
            if (isdigit(*map->array[y][x])){
                int objectType = atoi(map->array[y][x]);
                if (iterator[objectType] > dataLength[objectType]) Error("Array index out of bounds\n");
                LoadSingularObject(data, objectType, iterator[objectType], position);
                map->dataIndex[y][x] = iterator[objectType];
                iterator[objectType]++;
            } else if (map->array[y][x][0] == 'C'){
                if (iterator[CONVEYOR] > dataLength[CONVEYOR]) Error("Array index out of bounds\n");
                LoadGroupedObject(data, map->array[y][x], iterator[CONVEYOR], position);
                map->dataIndex[y][x] = iterator[CONVEYOR];
                iterator[CONVEYOR]++;
            } else {
                LoadLinkedObject(data, map->array[y][x], &map->dataIndex[y][x], iterator, dataLength, position);
            }
        }
    }
}

//Checks if the informed position is inside the informed map
int IsInsideMap(Vector2 block, CSV map){
    if (
        block.x < 0 ||
        block.y < 0 ||
        block.x >= map.cols ||
        block.y >= map.rows
    ){
        return false;
    }
    return true;
}

void zapWater(const CSV *map, int x, int y){
    // Check if the current cell is out of bounds or not equal to 6
    if (
        x < 0 ||
        y < 0 ||
        x >= map->cols-1 ||
        y >= map->rows-1 ||
        atoi(map->array[y][x]) != 6
    ) return;

    // Update the current cell to F
    map->array[y][x][1] = 'F';
    DrawRectangle(x*TILE_SIZE, y*TILE_SIZE, TILE_SIZE, TILE_SIZE, YELLOW);

    // Recursively update the adjacent cells
    if (atoi(map->array[y+1][x]) == 6) zapWater(map, x, y+1);
    if (atoi(map->array[y][x+1]) == 6) zapWater(map, x+1, y);
    if (atoi(map->array[y-1][x]) == 6) zapWater(map, x, y-1);
    if (atoi(map->array[y][x-1]) == 6) zapWater(map, x-1, y);
}

void restoreWater(const CSV *map, int x, int y){
    // Check if the current cell is out of bounds or not equal to 1
    if (
        x < 0 ||
        y < 0 ||
        x >= map->cols-1 ||
        y >= map->rows-1 ||
        map->array[y][x][1] != 'F'
    ) return;

    // Update the current cell to 6
    map->array[y][x][1] = '6';
    DrawRectangle(x*TILE_SIZE, y*TILE_SIZE, TILE_SIZE, TILE_SIZE, YELLOW);

    // Recursively update the adjacent cells
    if (map->array[y+1][x][1] == 'F') restoreWater(map, x, y+1);
    if (map->array[y][x+1][1] == 'F') restoreWater(map, x+1, y);
    if (map->array[y-1][x][1] == 'F') restoreWater(map, x, y-1);
    if (map->array[y][x-1][1] == 'F') restoreWater(map, x-1, y);
}