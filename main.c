float deltaTime;

#include <mainlibs.h>

int main(void){
    const char title[] = "Project";
    float resolutionW = 800, resolutionH = 600;
    int speed = 2, timer = 0;

    //Map object variables
    void *data[END];
    int structSize[END] = {NULL, sizeof(Entity), sizeof(Vector2), sizeof(Entity), sizeof(Portals), sizeof(Item), sizeof(Item), sizeof(Vector2)};
    int dataLength[END];
    memset(dataLength, 0, sizeof(dataLength));

    //UI variables
    char keysUI[] = "Keys: 0";
    int counterKeys = 0;

    Direction goToDir = NOWHERE;
    Camera2D camera = {.offset = (Vector2){ resolutionW/2, resolutionH/2}, .zoom = 1};

    int cellSize = 2;
    CSV map;
    calcLengthsCSV("map.csv", &map, cellSize, dataLength);

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
    //Block *puddles = (Block *)data[PUDDLE];
    Portals *portals = (Portals *)data[PORTAL];

    InitWindow(resolutionW, resolutionH, title);
    SetTargetFPS(60);

    TextureData textures[END];
    LoadTexturesFromFolder("./images", textures);

    Animation animPlayer = InitAnimValues(&textures[PLAYER].texture, 0, 0.5f, 2);
    Animation animPortal = InitAnimValues(&textures[PORTAL].texture, 0, 5.0f, 1);

    Item projectiles[3];
    for (int i = 0; i < 2; i++){
        projectiles[i].active = false;
    }

    while (!WindowShouldClose()){
        deltaTime = GetFrameTime();
        camera.target = (Vector2){Clamp(players[0].spr.x + HALF_SIZE, resolutionW/2, TILE_SIZE * map.cols - resolutionW/2), Clamp(players[0].spr.y + HALF_SIZE, resolutionH/2, TILE_SIZE * map.rows - resolutionH/2)};
        animPlayer.frameDuration = 0.5f/(speed == 2 ? 1 : 3);

        //Start rendering
        BeginDrawing();
        ClearBackground(BLACK);
        BeginMode2D(camera);

        DrawMap(&map, data, dataLength, textures, &animPlayer, &animPortal);

        Vector2 mouse = GetMousePosition();
        float angle = CalculateAngle((Vector2){(float)players[0].box.x+HALF_SIZE, (float)players[0].box.y+HALF_SIZE}, mouse);
        HideCursor();
        float rotation;
        if (angle >= 45 && angle < 135) rotation = 0;
        if (angle >= 135 && angle < 225) rotation = 90;
        if (angle >= 225 && angle < 315) rotation = 180;
        if (angle >= 315 || angle < 45) rotation = 270;
        Vector2 origin = { textures[0].texture.width / 2.0f, textures[0].texture.height / 2.0f };
        DrawTexturePro(textures[0].texture, (Rectangle){ 0, 0, (float)textures[0].texture.width, (float)textures[0].texture.height }, (Rectangle){mouse.x, mouse.y, (float)textures[0].texture.width, (float)textures[0].texture.height}, origin, rotation, WHITE);

        DrawText(keysUI, camera.target.x+270, camera.target.y+250, 30, WHITE);

        //Input
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){
            printf("%f\n", angle);
            for (int i = 0; i < 2; i++){
                if  (!projectiles[i].active){
                    projectiles[i].position = (Vector2){players[0].box.x + HALF_SIZE, players[0].box.y + HALF_SIZE};
                    projectiles[i].active = true;
                    break;
                }
            }
        }

        int PosX, PosY, remX, remY;
        for (int i = 0; i < 2; i++){
            if  (projectiles[i].active){
                remX = (int)(projectiles[i].position.x - HALF_SIZE) % TILE_SIZE;
                remY = (int)(projectiles[i].position.y - HALF_SIZE) % TILE_SIZE;
                PosX = (projectiles[i].position.x - HALF_SIZE + remX) / TILE_SIZE;
                PosY = (projectiles[i].position.y - HALF_SIZE + remY) / TILE_SIZE;

                if (PosX < 0 || PosX > map.cols-1 || PosY < 0 || PosY > map.rows-1){
                    projectiles[i].active = false;
                    PosX = PosY = 0;
                    continue;
                }

                if (atoi(map.array[PosY][PosX]) == PUDDLE){
                    projectiles[i].active = false;

                    zapWater(&map, PosX, PosY);
                    restoreWater(&map, PosX, PosY);
                    continue;
                }

                DrawCircleV((Vector2){(float)projectiles[i].position.x, (float)projectiles[i].position.y}, 15, YELLOW);
                projectiles[i].position.x += 30;
            }
        }

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
        char tileCode[cellSize+1];
        strcpy(tileCode, map.array[gridY][gridX]);
        int tileIndex = map.dataIndex[gridY][gridX];

        if (IsInsideMap((Vector2){(float)gridX, (float)gridY}, map)){
            if (isdigit(*tileCode)){
                switch (atoi(tileCode)){
                    case WALL:{
                        players[0].box.x = players[0].spr.x;
                        players[0].box.y = players[0].spr.y;
                        timer = 0;
                        break;
                    }
                    case CRATE:{
                        if (CheckCollisionPoints(crates[tileIndex].box, players[0].box)){
                            if (
                                players[0].box.y != players[0].spr.y ||
                                players[0].box.x != players[0].spr.x
                            ){
                                crates[tileIndex].box.y += IsHigherOrLower(players[0].box.y, players[0].spr.y) * TILE_SIZE;
                                crates[tileIndex].box.x += IsHigherOrLower(players[0].box.x, players[0].spr.x) * TILE_SIZE;
                            }

                            int crateY = crates[tileIndex].box.y / TILE_SIZE;
                            int crateX = crates[tileIndex].box.x / TILE_SIZE;
                            int cratePosition = atoi(map.array[crateY][crateX]);

                            if (
                                !IsInsideMap((Vector2){(float)crateX, (float)crateY}, map) ||
                                cratePosition == WALL ||
                                cratePosition == CRATE ||
                                cratePosition == DOOR
                            ){
                                players[0].box.x = players[0].spr.x;
                                players[0].box.y = players[0].spr.y;
                                crates[tileIndex].box.x = crates[tileIndex].spr.x;
                                crates[tileIndex].box.y = crates[tileIndex].spr.y;
                                timer = 0;
                            } else {
                                if (map.array[crateY][crateX][0] != 'P'){
                                    map.dataIndex[gridY][gridX] = map.dataIndex[crateY][crateX];
                                    map.dataIndex[crateY][crateX] = tileIndex;
                                    strcpy(map.array[gridY][gridX], "00");
                                    strcpy(map.array[crateY][crateX], "03");
                                }
                            }
                        }
                        break;
                    }
                }
            }
        } else {
            players[0].box.x = players[0].spr.x;
            players[0].box.y = players[0].spr.y;
            timer = 0;
        }

        tileIndex = map.dataIndex[(int)players[0].box.y / TILE_SIZE][(int)players[0].box.x / TILE_SIZE];

        //Collision testing door
        if (
            atoi(tileCode) == 4 &&
            CheckCollisionPoints(players[0].box, doors[tileIndex].position) &&
            !doors[tileIndex].active
        ){
            if (counterKeys){
                doors[tileIndex].active = true;
                counterKeys--;
                sprintf(keysUI, "Keys: %d", counterKeys);
            } else {
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
        if (tileCode[0] == 'P') CheckCollisionPortals(&players[0], portals[tileIndex], goToDir, &timer, false, &map);
        for (int i = 0; i < dataLength[CRATE]; i++){
            if (map.array[(int)crates->box.y/TILE_SIZE][(int)crates->box.x/TILE_SIZE][0] == 'P'){
                int index = map.dataIndex[(int)crates->box.y/TILE_SIZE][(int)crates->box.x/TILE_SIZE];
                CheckCollisionPortals(&crates[i], portals[index], goToDir, &timer, true, &map);
            }
        }

        //Collect key
        if (
            atoi(tileCode) == 5 &&
            CheckCollisionPoints(players[0].spr, keys[tileIndex].position) &&
            !keys[tileIndex].active
        ){
            keys[tileIndex].active = true;
            counterKeys++;
            sprintf(keysUI, "Keys: %d", counterKeys);
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