int tileSize = 64, speed = 2, timer = 0;

typedef struct Block {int x, y;} Block;
typedef struct Entity {Block box, spr;} Entity;
//typedef struct Portals {Block entry, exit;} Portals;

#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <mylib.h>

typedef struct Key {Block box; bool collected;} Key;

enum Direction {
    NOWHERE,
    UP,
    DOWN,
    RIGHT,
    LEFT,
} goToDir;

Entity player = {{.x = 320,.y = 256}, {.x = 320,.y = 256}};
Entity crate = {{.x = 192,.y = 448}, {.x = 192,.y = 448}};
Entity portals = {{.x = 0,.y = 0}, {.x = 640,.y = 640}};
Key door = {{320, 384}, false};
Key key = {{.x = 320,.y = 0}, false};
char keysUI[30];
int keys = 0;

Camera2D camera = {
    .offset = (Vector2){ 800/2, 600/2},
    .zoom = 1
};

int main(){
    InitWindow(800, 600, "Hello World");
    SetTargetFPS(60);

    int rows = 0, cols = 0;
    Array2D *map;
    LoadMap("map.txt", &rows, &cols, &map);

    Texture2D keyTexture = LoadTextureFromFile("images/Key.png");
    Texture2D wallTexture = LoadTextureFromFile("images/Wall.png");
    Texture2D crateTexture = LoadTextureFromFile("images/Crate.png");
    Texture2D doorOpTexture = LoadTextureFromFile("images/DoorOpened.png");
    Texture2D doorLoTexture = LoadTextureFromFile("images/DoorLocked.png");

    //Animation data
    Image playerSprites = LoadImage("images/PlayerSprites.png");
    ImageResizeNN(&playerSprites, tileSize*2, tileSize);
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
                if (map->array[i][j] == '1'){
                    DrawTexture(wallTexture, j*tileSize, i*tileSize, GRAY);
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
        DrawEntity(crate, WHITE, BROWN);
        DrawTexture(crateTexture, crate.spr.x, crate.spr.y, BROWN);

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

            player.box.y += (((goToDir == UP)*-1) + (goToDir == DOWN)) * tileSize;
            player.box.x += (((goToDir == LEFT)*-1) + (goToDir == RIGHT)) * tileSize;
            timer = tileSize;
        }

        //Move/Collide with crate
        if (player.box.y == crate.box.y && player.box.x == crate.box.x){
            if (player.box.y != player.spr.y){
                crate.box.y += IsHigherOrLower(player.box.y, player.spr.y) * tileSize;
            } else if (player.box.x != player.spr.x){
                crate.box.x += IsHigherOrLower(player.box.x, player.spr.x) * tileSize;
            }

            int gridY = crate.box.y / tileSize;
            int gridX = crate.box.x / tileSize;

            if (
                gridY > 0 && gridY <= rows &&
                gridX > 0 && gridX <= cols &&
                map->array[gridY][gridX] == '1'
            ){
                player.box.x = player.spr.x;
                player.box.y = player.spr.y;
                crate.box.x = crate.spr.x;
                crate.box.y = crate.spr.y;
                timer = 0;
            }
        }

        //Collision testing wall
        int gridY = player.box.y / tileSize;
        int gridX = player.box.x / tileSize;

        if (
            gridY > 0 && gridY <= rows &&
            gridX > 0 && gridX <= cols &&
            map->array[gridY][gridX] == '1'
        ){
            player.box.x = player.spr.x;
            player.box.y = player.spr.y;
            timer = 0;
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
                if (crate.spr.y != crate.box.y){
                    crate.spr.y += IsHigherOrLower(crate.box.y, crate.spr.y) * speed;
                } else if (crate.spr.x != crate.box.x){
                    crate.spr.x += IsHigherOrLower(crate.box.x, crate.spr.x) * speed;
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
                crate.box.x = crate.spr.x;
                crate.box.y = crate.spr.y;
                timer = 0;
            }
        }

        // Collide with portals
        if (
            player.box.y == player.spr.y &&
            player.box.x == player.spr.x &&
            ((player.box.y == portals.box.y && player.box.x == portals.box.x) ||
            (player.box.y == portals.spr.y && player.box.x == portals.spr.x))
        ){
            int portalY = (player.box.y == portals.box.y) ? portals.spr.y : portals.box.y;
            int portalX = (player.box.x == portals.box.x) ? portals.spr.x : portals.box.x;

            player.spr.y = portalY;
            player.spr.x = portalX;
            player.box.y = portalY + (((goToDir == UP) * -1) + (goToDir == DOWN)) * tileSize;
            player.box.x = portalX + (((goToDir == LEFT) * -1) + (goToDir == RIGHT)) * tileSize;
            timer = tileSize;
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
    free2DArray(map);
    CloseWindow();
    return 0;
}