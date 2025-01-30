float deltaTime;

#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <mylib.h>

int main(void){
    int speed = 2, timer = 0;

    //Map object variables
    void *data[END];
    int structSize[END] = {NULL, sizeof(Entity), sizeof(Block), sizeof(Entity), sizeof(Portals), sizeof(Item), sizeof(Item)};
    int dataLength[END];
    memset(dataLength, 0, sizeof(dataLength));

    //UI variables
    char keysUI[] = "Keys: 0";
    int counterKeys = 0;

    Direction goToDir = NOWHERE;
    Camera2D camera = {.offset = (Vector2){ 800/2, 600/2}, .zoom = 1};

    int cellSize = 2;
    CSV map;
    calcSizesCSV("map.csv", &map, cellSize, dataLength);

    MemoryArena arena;
    size_t size = ArenaCalcMapMemorySize(map, cellSize, dataLength, structSize);

    ArenaMalloc(&arena, size);
    GenerateArrayFromCSV("map.csv", &map, 2, &arena);
    ArenaReserveMemoryObjects(&arena, data, dataLength, structSize);
    PopulateData(data, &map, dataLength);

    Entity *players = (Entity *)data[PLAYER];
    //Block *walls = (Block *)data[WALL];
    Entity *crates = (Entity *)data[CRATE];
    Item *doors = (Item *)data[DOOR];
    Item *keys = (Item *)data[KEY];
    Portals *portals = (Portals *)data[PORTAL];

    InitWindow(800, 600, "Project");
    SetTargetFPS(60);

    TextureData textures[END];
    LoadTexturesFromFolder("./images", textures);

    Animation animPlayer = InitAnimValues(&textures[PLAYER].texture, 0, 0.5f, 2);
    Animation animPortal = InitAnimValues(&textures[PORTAL].texture, 0, 5.0f, 1);

    while (!WindowShouldClose()){
        deltaTime = GetFrameTime();
        camera.target = (Vector2){Clamp(players[0].spr.x + 32.0f, 800/2, 64*map.cols-800/2), Clamp(players[0].spr.y + 32.0f, 600/2, 64*map.rows-600/2)};
        animPlayer.frameDuration = 0.5f/(speed == 2 ? 1 : 3);

        //Start rendering
        BeginDrawing();
        ClearBackground(BLACK);
        BeginMode2D(camera);

        DrawMap(data, dataLength, textures, &animPlayer, &animPortal);
        DrawText(keysUI, camera.target.x+270, camera.target.y+250, 30, WHITE);

        int moveKeys[] = {KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_W, KEY_S, KEY_A, KEY_D, -1};
        int keyPressed = IsAnyOfKeysPressed(moveKeys);

        if (keyPressed && !timer){
            switch (keyPressed){
                case KEY_UP:
                case KEY_W:
                    goToDir = UP;
                    break;
                case KEY_DOWN:
                case KEY_S:
                    goToDir = DOWN;
                    break;
                case KEY_LEFT:
                case KEY_A:
                    goToDir = LEFT;
                    break;
                case KEY_RIGHT:
                case KEY_D:
                    goToDir = RIGHT;
                    break;
                default:
                    goToDir = NOWHERE;
                    break;
            }

            players[0].box.y += (((goToDir == UP)*-1) + (goToDir == DOWN)) * TILE_SIZE;
            players[0].box.x += (((goToDir == LEFT)*-1) + (goToDir == RIGHT)) * TILE_SIZE;
            timer = TILE_SIZE;
        }

        //Collision testing
        int gridX = players[0].box.x / TILE_SIZE;
        int gridY = players[0].box.y / TILE_SIZE;

        if (IsInsideMap((Block){gridX, gridY}, map)){
            if (isdigit(*map.array[gridY][gridX])){
                switch (atoi(map.array[gridY][gridX])){
                    case WALL:
                        players[0].box.x = players[0].spr.x;
                        players[0].box.y = players[0].spr.y;
                        timer = 0;
                        break;

                    case CRATE:
                        for (int i = 0; i < dataLength[CRATE]; i++){
                            if (CheckCollisionBlocks(crates[i].box, players[0].box)){
                                if (
                                    players[0].box.y != players[0].spr.y ||
                                    players[0].box.x != players[0].spr.x
                                ){
                                    crates[i].box.y += IsHigherOrLower(players[0].box.y, players[0].spr.y) * TILE_SIZE;
                                    crates[i].box.x += IsHigherOrLower(players[0].box.x, players[0].spr.x) * TILE_SIZE;
                                }

                                int crateY = crates[i].box.y / TILE_SIZE;
                                int crateX = crates[i].box.x / TILE_SIZE;

                                if (
                                    !IsInsideMap((Block){crateX, crateY}, map) ||
                                    atoi(map.array[crateY][crateX]) == WALL ||
                                    atoi(map.array[crateY][crateX]) == CRATE ||
                                    atoi(map.array[crateY][crateX]) == DOOR
                                ){
                                    players[0].box.x = players[0].spr.x;
                                    players[0].box.y = players[0].spr.y;
                                    crates[i].box.x = crates[i].spr.x;
                                    crates[i].box.y = crates[i].spr.y;
                                    timer = 0;
                                } else {
                                    strcpy(map.array[gridY][gridX], "01");
                                    strcpy(map.array[gridY+IsHigherOrLower(crateY, gridY)][gridX+IsHigherOrLower(crateX, gridX)], "03");
                                }
                            }
                        }
                        break;
                }
            }
        } else {
            players[0].box.x = players[0].spr.x;
            players[0].box.y = players[0].spr.y;
            timer = 0;
        }

        //Collision testing door
        for (int i = 0; i < dataLength[DOOR]; i++){
            if (
                CheckCollisionBlocks(players[0].box, doors[i].position) &&
                !doors[i].active
            ){
                if (counterKeys){
                    doors[i].active = true;
                    counterKeys--;
                    sprintf(keysUI, "Keys: %d", counterKeys);
                    break;
                }

                players[0].box.x = players[0].spr.x;
                players[0].box.y = players[0].spr.y;
                timer = 0;
            }
        }

        //Smooth movement animation
        speed = 2 * (IsKeyDown(KEY_LEFT_SHIFT) + 1);
        if (timer > 0){
            //Animate crate
            for (int i = 0; i < dataLength[CRATE]; i++){
                if (crates[i].box.x != crates[i].spr.x){
                    crates[i].spr.x += IsHigherOrLower(crates[i].box.x, crates[i].spr.x) * speed;
                } else if (crates[i].box.y != crates[i].spr.y){
                    crates[i].spr.y += IsHigherOrLower(crates[i].box.y, crates[i].spr.y) * speed;
                }
            }

            //Animate player
            if (players[0].spr.y != players[0].box.y){
                players[0].spr.y += IsHigherOrLower(players[0].box.y, players[0].spr.y) * speed;
            } else if (players[0].spr.x != players[0].box.x){
                players[0].spr.x += IsHigherOrLower(players[0].box.x, players[0].spr.x) * speed;
            }
            timer -= speed;
        } else {
            players[0].spr.y = players[0].box.y;
            players[0].spr.x = players[0].box.x;

            for (int i = 0; i < dataLength[CRATE]; i++){
                if (
                    crates[i].box.x != crates[i].spr.x ||
                    crates[i].box.y != crates[i].spr.y
                ){
                    crates[i].box.x = crates[i].spr.x;
                    crates[i].box.y = crates[i].spr.y;
                }
            }
            timer = 0;
        }

        // Collide with portals
        CheckCollisionPortals(&players[0], portals, goToDir, ((dataLength[PORTAL]/2)-(dataLength[PORTAL] % 2)), &timer, false, &map);
        for (int i = 0; i < dataLength[CRATE]; i++){
            CheckCollisionPortals(&crates[i], portals, goToDir, ((dataLength[PORTAL]/2)-(dataLength[PORTAL] % 2)), &timer, true, &map);
        }

        //Collect key
        for (int i = 0; i < dataLength[KEY]; i++){
            if (
                CheckCollisionBlocks(players[0].spr, keys[i].position) &&
                !keys[i].active
            ){
                keys[i].active = true;
                counterKeys++;
                sprintf(keysUI, "Keys: %d", counterKeys);
            }
        }

        EndMode2D();
        EndDrawing();
    }

    //Frees the allocated memory and ends the program
    for (int i = 0; i < END; i++) {
        UnloadTexture(textures[i].texture);
    }
    ArenaFree(&arena);
    CloseWindow();
}