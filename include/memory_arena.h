#pragma once

#include <types.h>
#include <map_data.h>
#include <stdlib.h>
#include <stdio.h>
#include <util.h>

// MemoryArena: Simple linear allocator for fast, temporary allocations.
typedef struct MemoryArena { void *buffer; size_t size, used; } MemoryArena;

// Initializes MemoryArena with the given size
static inline MemoryArena ArenaMalloc(size_t size){
    void *buffer = malloc(size);
    if (!buffer) Error("ArenaCreate: Failed to allocate %zu bytes", size);
    return (MemoryArena){ buffer, size, 0 };
}

// Frees the memory arena and resets its fields
static inline void ArenaFree(MemoryArena *arena){
    if (!arena) return;
    free(arena->buffer);
    *arena = (MemoryArena){ 0 };
}

// Reserves a block of memory from the arena
static inline void *ArenaReserve(MemoryArena *arena, size_t size){
    if (!arena || !arena->buffer || !size) return NULL;

    if (arena->used + size > arena->size){
        fprintf(stderr, "ArenaReserve: Out of memory (%zu/%zu)\n", arena->used, arena->size);
        ArenaFree(arena);
        exit(EXIT_FAILURE);
    }

    void *result = (char *)arena->buffer + arena->used;
    arena->used += size;
    return result;
}

// Reserves memory for the game's objects
static inline void ArenaReserveObjects(MemoryArena *arena, Map *map, const size_t structSize[]){
    void **obj = map->objects;
    unsigned int *length = map->length;
    const size_t *size = structSize;

    obj[TILE_PLAYER  ] = (Entity  *)ArenaReserve(arena, length[TILE_PLAYER  ] * size[TILE_PLAYER  ]);
    obj[TILE_WALL    ] = (Vector2 *)ArenaReserve(arena, length[TILE_WALL    ] * size[TILE_WALL    ]);
    obj[TILE_CRATE   ] = (Entity  *)ArenaReserve(arena, length[TILE_CRATE   ] * size[TILE_CRATE   ]);
    obj[TILE_DOOR    ] = (Item    *)ArenaReserve(arena, length[TILE_DOOR    ] * size[TILE_DOOR    ]);
    obj[TILE_KEY     ] = (Item    *)ArenaReserve(arena, length[TILE_KEY     ] * size[TILE_KEY     ]);
    obj[TILE_PUDDLE  ] = (Vector2 *)ArenaReserve(arena, length[TILE_PUDDLE  ] * size[TILE_PUDDLE  ]);
    obj[TILE_CONVEYOR] = (DirItem *)ArenaReserve(arena, length[TILE_CONVEYOR] * size[TILE_CONVEYOR]);

    // Portals are paired, so only allocate for pairs
    size_t portalPairs = length[TILE_PORTAL] / 2;
    obj[TILE_PORTAL]   = (Portals *)ArenaReserve(arena, portalPairs * size[TILE_PORTAL]);
}

// Function to fill a Map struct with the informed CSV file values
void GenerateArrayFromCSV(const char *fileName, Map *map, MemoryArena *arena){
    FILE *file = fopen(fileName, "r");
    if (!file) Error("Unable to open file: %s", fileName);

    // Allocate memory
    map->array     = (char ***)ArenaReserve(arena, map->rows * sizeof(char **));
    map->dataIndex = (int   **)ArenaReserve(arena, map->rows * sizeof(int   *));

    for (int i = 0; i < map->rows; i++){
        map->array[i]     = (char **)ArenaReserve(arena, map->cols * sizeof(char *));
        map->dataIndex[i] = (int   *)ArenaReserve(arena, map->cols * sizeof(int   ));

        for (int j = 0; j < map->cols; j++){
            map->array[i][j] = (char *)ArenaReserve(arena, (CELL_SIZE + 1) * sizeof(char));
            memset(map->array[i][j], 0, (CELL_SIZE + 1) * sizeof(char));
        }
    }

    // Fill array with cells
    char ch;
    int index = 0;
    map->cols = map->rows = 0;

    while ((ch = fgetc(file)) != EOF){
        if (ch == ','){
            map->cols++;
            index = 0;
            continue;
        } else if (ch == '\n'){
            map->rows++;
            map->cols = index = 0;
            continue;
        }

        map->array[map->rows][map->cols][index] = ch;
        if (index == CELL_SIZE - 1) map->array[map->rows][map->cols][index + 1] = '\0';
        index++;
    }
    map->rows++;
    map->cols++;

    fclose(file);
}