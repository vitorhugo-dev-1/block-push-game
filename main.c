#define TILE_SIZE 64

#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <mylib.h>

int main(){
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

    Camera2D camera = {
        .offset = (Vector2){ 800/2, 600/2},
        .zoom = 1
    };

    InitWindow(800, 600, "Project");
    SetTargetFPS(60);

    //Load objects from map
    for (int i = 0; i < map.rows; i++){
        for (int j = 0; j < map.cols; j++){
            if (isdigit(*map.array[i][j])){
                int currentValue = atoi(map.array[i][j]);
                switch (currentValue){
                    case PLAYER:{
                        player.spr.y = player.box.y = i * TILE_SIZE;
                        player.spr.x = player.box.x = j * TILE_SIZE;
                        break;
                    }
                    case WALL:{
                        Block new_wall = {j * TILE_SIZE, i * TILE_SIZE};
                        walls.instance = (Block*)AppendElement(walls.instance, sizeof(Block), walls.length, &new_wall);
                        walls.length++;
                        break;
                    }
                    case CRATE:{
                        Entity new_crate = {{j * TILE_SIZE, i * TILE_SIZE}, {j * TILE_SIZE, i * TILE_SIZE}};
                        crates.instance = (Entity*)AppendElement(crates.instance, sizeof(Entity), crates.length, &new_crate);
                        crates.length++;
                        break;
                    }
                    case DOOR:{
                        Item new_door = {{j * TILE_SIZE, i * TILE_SIZE}, 0};
                        doors.instance = (Item*)AppendElement(doors.instance, sizeof(Item), doors.length, &new_door);
                        doors.length++;
                        break;
                    }
                    case KEY:{
                        Item new_key = {{j * TILE_SIZE, i * TILE_SIZE}, 0};
                        keys.instance = (Item*)AppendElement(keys.instance, sizeof(Item), keys.length, &new_key);
                        keys.length++;
                        break;
                    }
                }
            } else {
                switch (map.array[i][j][0]){
                    case 'P':
                        int index = map.array[i][j][1] - '0';
                        Portals new_portal = {{j * TILE_SIZE, i * TILE_SIZE}, {-1, -1}};

                        if (!portals.length){
                            portals.instance = (Portals*)AppendElement(portals.instance, sizeof(Portals), portals.length, &new_portal);
                            portals.length++;
                            continue;
                        }
                        
                        if (
                            portals.instance[index].exit.x == -1 ||
                            portals.instance[index].exit.y == -1
                        ){
                            portals.instance[index].exit.x = j * TILE_SIZE;
                            portals.instance[index].exit.y = i * TILE_SIZE;
                        } else {
                            portals.instance = (Portals*)AppendElement(portals.instance, sizeof(Portals), portals.length, &new_portal);
                            portals.length++;
                        }
                        break;
                }
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
        camera.target = (Vector2){Clamp(player.spr.x + 32.0f, 800/2, 64*map.cols-800/2), Clamp(player.spr.y + 32.0f, 600/2, 64*map.rows-600/2)};

        //Start rendering
        BeginDrawing();
        ClearBackground(BLACK);
        BeginMode2D(camera);

        //Basic Animation
        framesCounter++;
        if (framesCounter >= (60/speed)){
            framesCounter = 0;
            currentFrame++;
            if (currentFrame > 1) currentFrame = 0;
            frameRec.x = (float)currentFrame*(float)anim.width/2;
        }

        //Draw Walls
        for (int i = 0; i < walls.length; i++){
            DrawTexture(wallTexture, walls.instance[i].x, walls.instance[i].y, GRAY);
        }
        
        //Draw Doors
        for (int i = 0; i < doors.length; i++){
            if (doors.instance[i].active){
                DrawTexture(doorOpTexture, doors.instance[i].position.x, doors.instance[i].position.y, WHITE);
                continue;
            }
            DrawTexture(doorLoTexture, doors.instance[i].position.x, doors.instance[i].position.y, WHITE);
        }

        //Draw Keys
        for (int i = 0; i < keys.length; i++){
            if (!keys.instance[i].active){
                DrawTexture(keyTexture, keys.instance[i].position.x, keys.instance[i].position.y, YELLOW);
            }
        }

        //Draw Portals
        for (int i = 0; i < portals.length; i++){
            DrawPortal(portals.instance[i], BLUE, GREEN);
        }

        //Draw Player
        DrawEntity(player, GRAY, RED);
        DrawTextureRec(anim, frameRec, (Vector2){(float)player.spr.x, (float)player.spr.y}, BLUE);

        //Draw crates
        for (int i = 0; i < crates.length; i++){
            DrawEntity(crates.instance[i], WHITE, BROWN);
            DrawTexture(crateTexture, crates.instance[i].spr.x, crates.instance[i].spr.y, BROWN);
        }

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
                player.box.y == doors.instance[i].position.y &&
                player.box.x == doors.instance[i].position.x &&
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
        if (player.spr.y != player.box.y || player.spr.x != player.box.x){
            if (timer > 0){
                //Animate crate
                for (int i = 0; i < crates.length; i++){
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
        }

        // Collide with portals
        for (int i = 0; i < portals.length; i++){
            Portals current = portals.instance[i];
            if (
                player.box.y == player.spr.y &&
                player.box.x == player.spr.x &&
                (
                    (player.box.y == current.entrance.y && player.box.x == current.entrance.x) ||
                    (player.box.y == current.exit.y && player.box.x == current.exit.x)
                )
            ){
                int portalY = (player.box.y == current.entrance.y) ? current.exit.y : current.entrance.y;
                int portalX = (player.box.x == current.entrance.x) ? current.exit.x : current.entrance.x;

                player.spr.y = portalY;
                player.spr.x = portalX;
                player.box.y = portalY + (((goToDir == UP) * -1) + (goToDir == DOWN)) * TILE_SIZE;
                player.box.x = portalX + (((goToDir == LEFT) * -1) + (goToDir == RIGHT)) * TILE_SIZE;
                timer = TILE_SIZE;
            }
        }

        //Collect key
        for (int i = 0; i < keys.length; i++){
            if (
                player.spr.y == keys.instance[i].position.y &&
                player.spr.x == keys.instance[i].position.x &&
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

    UnloadTexture(wallTexture);
    UnloadTexture(crateTexture);
    UnloadTexture(doorOpTexture);
    UnloadTexture(doorLoTexture);
    UnloadTexture(anim);
    FreeCSV(&map);
    free(walls.instance);
    free(crates.instance);
    free(doors.instance);
    free(keys.instance);
    free(portals.instance);
    CloseWindow();
    return 0;
}