#pragma once

#include <stdlib.h>
#include <defndec.h>
#include <util.h>

typedef struct MemoryArena {void* buffer; size_t size, used;} MemoryArena;

//Initializes MemoryArena struct and allocates the informed size
void ArenaMalloc(MemoryArena* arena, size_t size){
    arena->size = size;
    arena->used = 0;
    arena->buffer = malloc(size);
    if (arena->buffer == NULL) Error("Failed to allocate memory\n");
}

// Frees the memory allocated for the informed arena
void ArenaFree(MemoryArena* arena){
    free(arena->buffer);
    arena->buffer = NULL;
    arena->size = 0;
    arena->used = 0;
}

//Reserves memory from the arena
void* ArenaReserveMemory(MemoryArena* arena, size_t size){
    if (arena->used + size > arena->size){
        printf("%lld/%lld\n", arena->used, arena->size);
        ArenaFree(arena);
        Error("Out of memory\n");
    }

    void* result = (char*)arena->buffer + arena->used;
    arena->used += size;
    return result;
}

//Reserves memory for the game's objects
void ArenaReserveMemoryObjects(MemoryArena *arena, void *data[], int dataLength[], int structSize[]){
    data[PLAYER]   =  (Entity *)ArenaReserveMemory(arena, dataLength[PLAYER]   * structSize[PLAYER]);
    data[WALL]     = (Vector2 *)ArenaReserveMemory(arena, dataLength[WALL]     * structSize[WALL]);
    data[CRATE]    =  (Entity *)ArenaReserveMemory(arena, dataLength[CRATE]    * structSize[CRATE]);
    data[DOOR]     =    (Item *)ArenaReserveMemory(arena, dataLength[DOOR]     * structSize[DOOR]);
    data[KEY]      =    (Item *)ArenaReserveMemory(arena, dataLength[KEY]      * structSize[KEY]);
    data[PUDDLE]   = (Vector2 *)ArenaReserveMemory(arena, dataLength[PUDDLE]   * structSize[PUDDLE]);
    data[CONVEYOR] = (DirItem *)ArenaReserveMemory(arena, dataLength[CONVEYOR] * structSize[CONVEYOR]);
    data[PORTAL]   = (Portals *)ArenaReserveMemory(arena, ((dataLength[PORTAL]/2) - (dataLength[PORTAL] % 2)) * structSize[PORTAL]);
}

//Reset the memory arena
void ArenaReset(MemoryArena* arena){
    arena->used = 0;
}