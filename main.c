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

    char keysUI[] = "Keys: 0";
    int counterKeys = 0;

    Direction goToDir = NOWHERE;

    CSV map = {NULL, 1, 1};
    LoadCSV("map.csv", &map, 2);

    Entity player;

    BlockArr walls = {NULL, 0};
    EntityArr crates = {NULL, 0};
    ItemArr doors = {NULL, 0};
    ItemArr keys = {NULL, 0};

    PortalsArr portals = {NULL, 0};

    void *data[END] = {
        [EMPTY] = NULL,
        [PLAYER] = &player,
        [WALL] = &walls,
        [CRATE] = &crates,
        [PORTAL] = &portals,
        [DOOR] = &doors,
        [KEY] = &keys
    };

    Camera2D camera = {
        .offset = (Vector2){ 800/2, 600/2},
        .zoom = 1
    };

    LoadData(data, &map); //Load objects from map into their specific structs

    InitWindow(800, 600, "Project");
    SetTargetFPS(60);

    TextureData textures[END];
    LoadTexturesFromFolder("./images", textures);

    Animation animPlayer = InitAnimValues(&textures[PLAYER].texture, 0, 0.5f, 2);
    Animation animPortal = InitAnimValues(&textures[PORTAL].texture, 0, 5.0f, 1);

    while (!WindowShouldClose()){
        deltaTime = GetFrameTime();
        camera.target = (Vector2){Clamp(player.spr.x + 32.0f, 800/2, 64*map.cols-800/2), Clamp(player.spr.y + 32.0f, 600/2, 64*map.rows-600/2)};
        animPlayer.frameDuration = 0.5f/(speed == 2 ? 1 : 3);

        //Start rendering
        BeginDrawing();
        ClearBackground(BLACK);
        BeginMode2D(camera);

        DrawMap(data, textures, &animPlayer, &animPortal);
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

            player.box.y += (((goToDir == UP)*-1) + (goToDir == DOWN)) * TILE_SIZE;
            player.box.x += (((goToDir == LEFT)*-1) + (goToDir == RIGHT)) * TILE_SIZE;
            timer = TILE_SIZE;
        }

        //Collision testing
        int gridX = player.box.x / TILE_SIZE;
        int gridY = player.box.y / TILE_SIZE;

        if (IsInsideMap((Block){gridX, gridY}, map)){
            if (isdigit(*map.array[gridY][gridX])){
                switch (atoi(map.array[gridY][gridX])){
                    case WALL:
                        player.box.x = player.spr.x;
                        player.box.y = player.spr.y;
                        timer = 0;
                        break;

                    case CRATE:
                        for (int i = 0; i < crates.length; i++){
                            if (CheckCollisionBlocks(crates.instance[i].box, player.box)){
                                if (
                                    player.box.y != player.spr.y ||
                                    player.box.x != player.spr.x
                                ){
                                    crates.instance[i].box.y += IsHigherOrLower(player.box.y, player.spr.y) * TILE_SIZE;
                                    crates.instance[i].box.x += IsHigherOrLower(player.box.x, player.spr.x) * TILE_SIZE;
                                }

                                int crateY = crates.instance[i].box.y / TILE_SIZE;
                                int crateX = crates.instance[i].box.x / TILE_SIZE;

                                if (
                                    !IsInsideMap((Block){crateX, crateY}, map) ||
                                    atoi(map.array[crateY][crateX]) == WALL ||
                                    atoi(map.array[crateY][crateX]) == CRATE ||
                                    atoi(map.array[crateY][crateX]) == DOOR
                                ){
                                    player.box.x = player.spr.x;
                                    player.box.y = player.spr.y;
                                    crates.instance[i].box.x = crates.instance[i].spr.x;
                                    crates.instance[i].box.y = crates.instance[i].spr.y;
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
            player.box.x = player.spr.x;
            player.box.y = player.spr.y;
            timer = 0;
        }

        //Collision testing door
        for (int i = 0; i < doors.length; i++){
            if (
                CheckCollisionBlocks(player.box, doors.instance[i].position) &&
                !doors.instance[i].active
            ){
                if (counterKeys){
                    doors.instance[i].active = true;
                    counterKeys--;
                    sprintf(keysUI, "Keys: %d", counterKeys);
                    break;
                }

                player.box.x = player.spr.x;
                player.box.y = player.spr.y;
                timer = 0;
            }
        }

        //Smooth movement animation
        speed = 2 * (IsKeyDown(KEY_LEFT_SHIFT) + 1);
        if (timer > 0){
            //Animate crate
            for (int i = 0; i < crates.length; i++){
                if (crates.instance[i].box.x != crates.instance[i].spr.x){
                    crates.instance[i].spr.x += IsHigherOrLower(crates.instance[i].box.x, crates.instance[i].spr.x) * speed;
                } else if (crates.instance[i].box.y != crates.instance[i].spr.y){
                    crates.instance[i].spr.y += IsHigherOrLower(crates.instance[i].box.y, crates.instance[i].spr.y) * speed;
                }
            }

            //Animate player
            if (player.spr.y != player.box.y){
                player.spr.y += IsHigherOrLower(player.box.y, player.spr.y) * speed;
            } else if (player.spr.x != player.box.x){
                player.spr.x += IsHigherOrLower(player.box.x, player.spr.x) * speed;
            }
            timer -= speed;
        } else {
            player.spr.y = player.box.y;
            player.spr.x = player.box.x;

            for (int i = 0; i < crates.length; i++){
                if (
                    crates.instance[i].box.x != crates.instance[i].spr.x ||
                    crates.instance[i].box.y != crates.instance[i].spr.y
                ){
                    crates.instance[i].box.x = crates.instance[i].spr.x;
                    crates.instance[i].box.y = crates.instance[i].spr.y;
                }
            }
            timer = 0;
        }

        // Collide with portals
        CheckCollisionPortals(&player, &portals, goToDir, &timer, false, &map);
        for (int i = 0; i < crates.length; i++){
            CheckCollisionPortals(&crates.instance[i], &portals, goToDir, &timer, true, &map);
        }

        //Collect key
        for (int i = 0; i < keys.length; i++){
            if (
                CheckCollisionBlocks(player.spr, keys.instance[i].position) &&
                !keys.instance[i].active
            ){
                keys.instance[i].active = true;
                counterKeys++;
                sprintf(keysUI, "Keys: %d", counterKeys);
            }
        }

        EndMode2D();
        EndDrawing();
    }

    for (int i = 0; i < END; i++) {
        UnloadTexture(textures[i].texture);
    }
    FreeCSV(&map);
    free(walls.instance);
    free(crates.instance);
    free(doors.instance);
    free(keys.instance);
    free(portals.instance);
    CloseWindow();
}