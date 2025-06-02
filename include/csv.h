#pragma once

#include <string.h>
#include <ctype.h>
#include <defndec.h>
#include <util.h>
#include <marena.h>

typedef struct CSV { char ***array; int rows, cols; int ** dataIndex; } CSV;

//Calculates how many elements are present in a CSV file and
//how many of each object are present in that same CSV
CSV CalcLengthsCSV(const char *fileName, int dataLength[]){
    FILE *file = fopen(fileName, "r");
    if (!file) Error("Unable to open file\n");

    char ch, element[CELL_SIZE+1];
    int counter = 0;
    CSV csv = { NULL, 1, 1, NULL };

    while ((ch = fgetc(file)) != EOF){
        if (ch == ','){
            csv.cols++;
            continue;
        } else if (ch == '\n'){
            csv.cols = 1;
            csv.rows++;
            continue;
        }

        element[counter] = ch;
        counter++;

        if (counter != CELL_SIZE) continue;

        if (!isdigit(*element)){
            switch (element[0]){
                case 'C': dataLength[CONVEYOR]++; break;
                case 'P': dataLength[PORTAL]++;   break;
            }
        } else {
            dataLength[atoi(element)]++;
        }

        counter = 0;
    }
    fclose(file);

    return csv;
}

//Calculates how many bytes of memory the map data will use
size_t CalcMapMemorySize(const CSV map, const int dataLength[], const int structSize[]){
    size_t size = (map.rows * sizeof(char **)) +
                  (map.rows * (map.cols * sizeof(char *))) +
                  (map.rows * map.cols * (CELL_SIZE + 1) * sizeof(char))
                  +
                  (map.rows * sizeof(int *)) +
                  (map.rows * (map.cols * sizeof(int)));

    for (int i = 1; i < END; i++){
        size += dataLength[i] * structSize[i];
    }
    return size;
}

//Function to generate a 2D array with csv cell values for the CSV struct
void GenerateArrayFromCSV(const char *fileName, CSV *csv, MemoryArena *arena){
    FILE *file = fopen(fileName, "r");
    if (!file) Error("Unable to open file\n");

    //Allocate memory
    csv->array     = (char ***)ArenaReserveMemory(arena, csv->rows * sizeof(char **));
    csv->dataIndex =   (int **)ArenaReserveMemory(arena, csv->rows * sizeof(int *));

    for (int i = 0; i < csv->rows; i++){
        csv->array[i]     = (char **)ArenaReserveMemory(arena, csv->cols * sizeof(char *));
        csv->dataIndex[i] =   (int *)ArenaReserveMemory(arena, csv->cols * sizeof(int));
        
        for (int j = 0; j < csv->cols; j++){
            csv->array[i][j] = (char *)ArenaReserveMemory(arena, (CELL_SIZE + 1) * sizeof(char));
            memset(csv->array[i][j], 0, (CELL_SIZE + 1) * sizeof(char));
        }
    }

    //Fill array with cells
    char ch;
    int digit = 0;
    csv->cols = csv->rows = 0;

    while ((ch = fgetc(file)) != EOF){
        if (ch == ','){
            csv->cols++;
            digit = 0;
            continue;
        } else if (ch == '\n'){
            csv->rows++;
            csv->cols = digit = 0;
            continue;
        }

        csv->array[csv->rows][csv->cols][digit] = ch;
        if (digit == CELL_SIZE - 1) csv->array[csv->rows][csv->cols][digit + 1] = '\0';
        digit++;
    }
    csv->rows++;
    csv->cols++;

    fclose(file);
}

//Loads into an array a map object that's not directly linked to any other object in the same map
void LoadSingularObject(void *data[], const int objectType, const int iteration, const Vector2 position) {
    if (iteration < 0) Error("Iteration is not valid");  // Ensure iteration is valid

    switch (objectType){
        case PLAYER: case CRATE:{
            Entity *entities = (Entity *)data[objectType];
            entities[iteration] = (Entity){ position, position };
            break;
        }
        case WALL: case PUDDLE:{
            Vector2 *vectors = (Vector2 *)data[objectType];
            vectors[iteration] = position;
            break;
        }
        case DOOR: case KEY:{
            Item *items = (Item *)data[objectType];
            items[iteration] = (Item){ position, false };
            break;
        }
    }
}

//Loads into an array a map object that's not directly linked to any other object in the same map
void LoadGroupedObject(void *data[], const char objectType[3], const int iteration, const Vector2 position){
    switch (objectType[0]){
        case 'C':{
            DirItem *conveyors = (DirItem *)data[CONVEYOR];
            conveyors[iteration] = (DirItem){ position, (Direction)(objectType[1] - '0') };
            break;
        }
    }
}

//Loads into an array a map object that's directly linked to another one in the same map
void LoadLinkedObject(
    void *data[],
    const char *objectType,
    int *dataIndex,
    int iterator[],
    const int dataLength[],
    const Vector2 position
){
    if (!objectType || strlen(objectType) < 2 || !isdigit(objectType[1])) {
        Error("Invalid objectType format\n");
    }

    int index = objectType[1] - '0';
    *dataIndex = index;

    switch (objectType[0]){
        case 'P':
            if (iterator[PORTAL] >= dataLength[PORTAL]) Error("Array index out of bounds\n");

            Portals *portals = (Portals *)data[PORTAL];
            iterator[PORTAL]++;

            if (!iterator[PORTAL]){
                portals[iterator[PORTAL]] = (Portals){ position, {-1, -1} };
                return;
            }

            if (portals[index].exit.x == -1 || portals[index].exit.y == -1){
                portals[index].exit = position;
            } else {
                portals[index] = (Portals){ position, {-1, -1} };
            }
            break;
    }
}

//Loads all data inside a CSV into their specific arrays
void PopulateData(void *data[], CSV *map, const int dataLength[]){
    int iterator[END] = { [0 ... END-1] = 0 };

    for (int y = 0; y < map->rows; y++){
        for (int x = 0; x < map->cols; x++){
            Vector2 position = { x * TILE_SIZE, y * TILE_SIZE };
            int objectType = atoi(map->array[y][x]);

            if (isdigit(*map->array[y][x])){
                if (iterator[objectType] >= dataLength[objectType]) Error("Array index out of bounds\n");

                LoadSingularObject(
                    data,
                    objectType,
                    iterator[objectType],
                    position
                );

            } else if (map->array[y][x][0] == 'C'){
                if (iterator[CONVEYOR] >= dataLength[CONVEYOR]) Error("Array index out of bounds\n");

                LoadGroupedObject(
                    data,
                    map->array[y][x],
                    iterator[CONVEYOR],
                    position
                );

                objectType = CONVEYOR;
            } else {
                LoadLinkedObject(
                    data,
                    map->array[y][x],
                    &map->dataIndex[y][x],
                    iterator,
                    dataLength,
                    position
                );

                continue;
            }

            map->dataIndex[y][x] = iterator[objectType];
            iterator[objectType]++;
        }
    }
}

//Checks if the informed position is inside the informed map
int IsInsideMap(Vector2 block, CSV map, bool isProjectile){
    if (
        block.x < 0 ||
        block.y < 0 ||
        block.x >= map.cols - isProjectile ||
        block.y >= map.rows - isProjectile
    ){
        return false;
    }
    return true;
}

typedef bool (*ConditionFunc)(CSV *map, int x, int y);
typedef void (*ActionFunc)(CSV *map, Vector2 pos);

bool IsPuddle(CSV *map, int x, int y){
    return (atoi(map->array[y][x]) == PUDDLE);
}

bool IsZapped(CSV *map, int x, int y){
    return (map->array[y][x][1] == 'F');
}

void ProcessAdjacentCells(CSV *map, int x, int y, ConditionFunc condition, ActionFunc action){
    int directions[4][2] = { {0, 1}, {1, 0}, {0, -1}, {-1, 0} };

    for (int i = 0; i < 4; i++){
        int newX = x + directions[i][0];
        int newY = y + directions[i][1];

        if (condition(map, newX, newY)){ action(map, (Vector2){ newX, newY }); }
    }
}

void zapWater(CSV *map, Vector2 pos){
    int x = pos.x, y = pos.y;
    if (!IsPuddle(map, x, y)) return; // Check if the current cell is not puddle

    // Update the current cell to F
    map->array[y][x][1] = 'F';
    DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, YELLOW);

    // Recursively update the adjacent cells
    ProcessAdjacentCells(map, x, y, IsPuddle, zapWater);
}

void restoreWater(CSV *map, Vector2 pos){
    int x = pos.x, y = pos.y;
    if (!IsZapped(map, x, y)) return; // Check if the current cell is not equal to F

    // Update the current cell to puddle value
    map->array[y][x][1] = 6 + '0';
    DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, YELLOW);

    // Recursively update the adjacent cells
    ProcessAdjacentCells(map, x, y, IsZapped, restoreWater);
}