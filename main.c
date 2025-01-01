#define TILE_SIZE 64

#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <mylib.h>

int main(){
    int speed = 2, timer = 0;
    enum Direction goToDir = NOWHERE;

    Entity player;
    
    EntityArr crates;
    crates.size = 0;
    
    Entity portals = {{.x = 0,.y = 0}, {.x = 640,.y = 640}};
    Key door = {{320, 384}, false};
    Key key = {{.x = 320,.y = 0}, false};

    char keysUI[30];
    int keys = 0;

    Camera2D camera = {
        .offset = (Vector2){ 800/2, 600/2},
        .zoom = 1
    };

    InitWindow(800, 600, "Hello World");
    SetTargetFPS(60);

    int rows = 0, cols = 0;
    Array2D *map;
    LoadMap("map.csv", &rows, &cols, &map);

    //Create objects

    for (int i = 0; i <= rows; i++){
        for (int j = 0; j < cols; j++){
            int currentValue = map->array[i][j] - '0';
            switch (currentValue){
                case PLAYER:
                    player.spr.y = player.box.y = i * TILE_SIZE;
                    player.spr.x = player.box.x = j * TILE_SIZE;
                    break;
                case CRATE:
                    Entity newElement = {{j * TILE_SIZE, i * TILE_SIZE}, {j * TILE_SIZE, i * TILE_SIZE}};
                    crates.instance = Append(crates.instance, crates.size, newElement);
                    crates.size++;
                    break;
            }
        }
    }

    Texture2D keyTexture = LoadTextureFromFile("images/Key.png");
    Texture2D wallTexture = LoadTextureFromFile("images/Wall.png");
    Texture2D crateTexture = LoadTextureFromFile("images/Crate.png");
    Texture2D doorOpTexture = LoadTextureFromFile("images/DoorOpened.png");
    Texture2D doorLoTexture = LoadTextureFromFile("images/DoorLocked.png");

    //Animation data
    Image playerSprites = LoadImage("images/PlayerSprites.png");
    ImageResizeNN(&playerSprites, TILE_SIZE*2, TILE_SIZE);
    Texture2D anim = LoadTextureFromImage(playerSprites);
    UnloadImage(playerSprites);
    Rectangle frameRec = { 0.0f, 0.0f, (float)anim.width/2, (float)anim.height };
    int currentFrame = 0, framesCounter = 0;

    while (!WindowShouldClose()){
        //Basic Animation
        int framesSpeed = (speed-1) * 2;
        framesCounter++;
        if (framesCounter >= (60/framesSpeed)){
            framesCounter = 0;
            currentFrame++;
            if (currentFrame > 1) currentFrame = 0;
            frameRec.x = (float)currentFrame*(float)anim.width/2;
        }

        BeginDrawing();
        ClearBackground(BLACK);

        camera.target = (Vector2){ player.spr.x + 32.0f, player.spr.y + 32.0f },
        BeginMode2D(camera);

        //Draw Walls
        for (int i = 0; i <= rows; i++){
            for (int j = 0; j < cols; j++){
                if (map->array[i][j] == '2'){
                    DrawTexture(wallTexture, j*TILE_SIZE, i*TILE_SIZE, GRAY);
                }
            }
        }

        //Draw other objects
        DrawEntity(portals, BLUE, GREEN);
        DrawEntity(player, GRAY, RED);

        if (!key.collected){
            DrawTexture(keyTexture, key.box.x, key.box.y, YELLOW);
        }

        if (!door.collected){
            DrawTexture(doorLoTexture, door.box.x, door.box.y, WHITE);
        } else {
            DrawTexture(doorOpTexture, door.box.x, door.box.y, WHITE);
        }

        DrawTextureRec(anim, frameRec, (Vector2){(float)player.spr.x, (float)player.spr.y}, BLUE);

        for (int i = 0; i < crates.size; i++){
            DrawEntity(crates.instance[i], WHITE, BROWN);
            DrawTexture(crateTexture, crates.instance[i].spr.x, crates.instance[i].spr.y, BROWN);
        }

        sprintf(keysUI, "Keys: %d", keys);
        DrawText(keysUI, player.spr.x+280, player.spr.y+280, 30, WHITE);

        speed = 2 * (IsKeyDown(KEY_LEFT_SHIFT) + 1);

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
            printf("P: [%d, %d] - %d\n", (player.box.y/TILE_SIZE), (player.box.x/TILE_SIZE), IsInsideMap((Block){player.box.x/TILE_SIZE, player.box.y/TILE_SIZE}, *map));
        }

        //Collision testing
        int gridX = player.box.x / TILE_SIZE;
        int gridY = player.box.y / TILE_SIZE;

        if (IsInsideMap((Block){gridX, gridY}, *map)){
            switch (map->array[gridY][gridX] - '0'){
                case WALL:
                    player.box.x = player.spr.x;
                    player.box.y = player.spr.y;
                    timer = 0;
                    break;

                case CRATE:
                    for (int i = 0; i < crates.size; i++){
                        if (
                            crates.instance[i].box.x == player.box.x &&
                            crates.instance[i].box.y == player.box.y
                        ){
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
                                IsInsideMap((Block){crateX, crateY}, *map) &&
                                (
                                    map->array[crateY][crateX]-'0' == WALL ||
                                    map->array[crateY][crateX]-'0' == CRATE ||
                                    map->array[crateY][crateX]-'0' == DOOR
                                )
                            ){
                                player.box.x = player.spr.x;
                                player.box.y = player.spr.y;
                                crates.instance[i].box.x = crates.instance[i].spr.x;
                                crates.instance[i].box.y = crates.instance[i].spr.y;
                                timer = 0;
                            } else {
                                map->array[gridY][gridX] = '1';
                                map->array[gridY+IsHigherOrLower(crateY, gridY)][gridX+IsHigherOrLower(crateX, gridX)] = '3';
                                printf("C: [%d, %d]\n", crateX, crateY);
                            }
                        }
                    }
                    break;
            }
        }

        //Collision testing door
        if (
            player.box.y == door.box.y &&
            player.box.x == door.box.x &&
            !door.collected
        ){
            if (keys){
                door.collected = true;
                keys--;
            } else {
                player.box.x = player.spr.x;
                player.box.y = player.spr.y;
                timer = 0;
            }
        }

        //Smooth movement animation
        if (player.spr.y != player.box.y || player.spr.x != player.box.x){
            if (timer > 0){
                //Animate crate
                for (int i = 0; i < crates.size; i++){
                    if (
                        crates.instance[i].box.x != crates.instance[i].spr.x ||
                        crates.instance[i].box.y != crates.instance[i].spr.y
                    ){
                        crates.instance[i].spr.y += IsHigherOrLower(crates.instance[i].box.y, crates.instance[i].spr.y) * speed;
                        crates.instance[i].spr.x += IsHigherOrLower(crates.instance[i].box.x, crates.instance[i].spr.x) * speed;
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

                for (int i = 0; i < crates.size; i++){
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
        }

        // Collide with portals
        if (
            player.box.y == player.spr.y &&
            player.box.x == player.spr.x &&
            (
                (player.box.y == portals.box.y && player.box.x == portals.box.x) ||
                (player.box.y == portals.spr.y && player.box.x == portals.spr.x)
            )
        ){
            int portalY = (player.box.y == portals.box.y) ? portals.spr.y : portals.box.y;
            int portalX = (player.box.x == portals.box.x) ? portals.spr.x : portals.box.x;

            player.spr.y = portalY;
            player.spr.x = portalX;
            player.box.y = portalY + (((goToDir == UP) * -1) + (goToDir == DOWN)) * TILE_SIZE;
            player.box.x = portalX + (((goToDir == LEFT) * -1) + (goToDir == RIGHT)) * TILE_SIZE;
            timer = TILE_SIZE;
        }

        //Collect key
        if (
            player.spr.y == key.box.y &&
            player.spr.x == key.box.x &&
            !key.collected
        ){
            key.collected = true;
            keys++;
        }

        EndMode2D();
        EndDrawing();
    }

    UnloadTexture(wallTexture);
    UnloadTexture(crateTexture);
    UnloadTexture(doorOpTexture);
    UnloadTexture(doorLoTexture);
    UnloadTexture(anim);
    Free2DArray(map);
    free(crates.instance);
    CloseWindow();
    return 0;
}