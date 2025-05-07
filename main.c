float deltaTime;

#include <mainlibs.h>

int main(void){
    const char title[] = "Project";
    float resolutionW = 800, resolutionH = 600;
    int speed = 2, timer = 0;

    //Map object variables
    void *data[END];
    int structSize[END] = {
        NULL,            //Empty space
        sizeof(Entity),  //Player character
        sizeof(Vector2), //Wall
        sizeof(Entity),  //Crate
        sizeof(Item),    //Door
        sizeof(Item),    //Key
        sizeof(Vector2), //Puddle
        sizeof(DirItem), //Conveyor
        sizeof(Portals)  //Portal
    };
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
    //Vector2 *walls = (Block *)data[WALL];
    Entity *crates = (Entity *)data[CRATE];
    Item *doors = (Item *)data[DOOR];
    Item *keys = (Item *)data[KEY];
    //Vector2 *puddles = (Block *)data[PUDDLE];
    //DirItem *conveyors = (DirItem *)data[CONVEYOR];
    Portals *portals = (Portals *)data[PORTAL];

    InitWindow(resolutionW, resolutionH, title);
    SetTargetFPS(60);

    Texture2D textures[END] = { NULL };
    LoadTexturesFromFolder("./images", textures);

    Animation animPlayer = InitAnimValues(&textures[PLAYER], 0, 0.5f, 2);
    Animation animPortal = InitAnimValues(&textures[PORTAL], 0, 5.0f, 1);

    DirItem projectiles[3];
    for (int i = 0; i < 2; i++){
        projectiles[i].dir = NOWHERE;
    }

    while (!WindowShouldClose()){
        deltaTime = GetFrameTime();
        camera.target = (Vector2){Clamp(players[0].spr.x + HALF_SIZE, resolutionW/2, TILE_SIZE * map.cols - resolutionW/2), Clamp(players[0].spr.y + HALF_SIZE, resolutionH/2, TILE_SIZE * map.rows - resolutionH/2)};
        animPlayer.frameDuration = 0.5f / (speed == 2 ? 1 : 3);

        //Start rendering
        HideCursor();
        BeginDrawing();
        ClearBackground(BLACK);
        BeginMode2D(camera);

        //Cursor values
        Vector2 origin = { textures[0].width / 2.0f, textures[0].height / 2.0f };
        Vector2 cursor = GetMousePosition();
        cursor = GetScreenToWorld2D(cursor, camera);
        float angle = CalculateAngle((Vector2){(float)players[0].box.x + HALF_SIZE, (float)players[0].box.y + HALF_SIZE}, cursor);
        float rotation;

        if (angle >= 45 && angle < 135) rotation = 0;
        else if (angle >= 135 && angle < 225) rotation = 90;
        else if (angle >= 225 && angle < 315) rotation = 180;
        else rotation = 270;

        //Rendering
        DrawMap(&map, data, dataLength, textures, &animPlayer, &animPortal);
        DrawTexturePro(textures[0], (Rectangle){ 0, 0, (float)textures[0].width, (float)textures[0].height }, (Rectangle){cursor.x, cursor.y, (float)textures[0].width, (float)textures[0].height}, origin, rotation, WHITE);
        DrawText(keysUI, camera.target.x + 270, camera.target.y + 250, 30, WHITE);

        //Controls
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){
            for (int i = 0; i < 2; i++){
                if  (projectiles[i].dir) continue;

                projectiles[i].position = (Vector2){players[0].box.x + HALF_SIZE, players[0].box.y + HALF_SIZE};

                if (rotation == 0) projectiles[i].dir = UP;
                else if (rotation == 90) projectiles[i].dir = RIGHT;
                else if (rotation == 180) projectiles[i].dir = DOWN;
                else projectiles[i].dir = LEFT;

                break;
            }
        }

        int PosX, PosY, remX, remY;
        for (int i = 0; i < 2; i++){
            if  (!projectiles[i].dir) continue;

            remX = (int)(projectiles[i].position.x - HALF_SIZE) % TILE_SIZE;
            remY = (int)(projectiles[i].position.y - HALF_SIZE) % TILE_SIZE;
            PosX = (projectiles[i].position.x - HALF_SIZE + remX) / TILE_SIZE;
            PosY = (projectiles[i].position.y - HALF_SIZE + remY) / TILE_SIZE;

            if (PosX < 0 || PosX > map.cols-1 || PosY < 0 || PosY > map.rows-1){
                projectiles[i].dir = NOWHERE;
                PosX = PosY = 0;
                continue;
            }

            if (atoi(map.array[PosY][PosX]) == PUDDLE){
                projectiles[i].dir = NOWHERE;

                zapWater(&map, PosX, PosY);
                restoreWater(&map, PosX, PosY);
                continue;
            }

            DrawCircleV((Vector2){(float)projectiles[i].position.x, (float)projectiles[i].position.y}, 15, YELLOW);
            projectiles[i].position.x += (((projectiles[i].dir == RIGHT) - (projectiles[i].dir == LEFT)) * 30);
            projectiles[i].position.y += (((projectiles[i].dir == DOWN) - (projectiles[i].dir == UP)) * 30);
        }

        int moveKeys[] = {KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_W, KEY_S, KEY_A, KEY_D, -1};
        int keyPressed = IsAnyOfKeysPressed(moveKeys);

        if (keyPressed && !timer){
            switch (keyPressed){
                case KEY_W: case KEY_UP:
                    goToDir = UP;
                    break;
                case KEY_S: case KEY_DOWN:
                    goToDir = DOWN;
                    break;
                case KEY_A: case KEY_LEFT:
                    goToDir = LEFT;
                    break;
                case KEY_D: case KEY_RIGHT:
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
        int tileIndex;
        char tileCode[cellSize+1];

        if (IsInsideMap((Vector2){(float)gridX, (float)gridY}, map)){
            strcpy(tileCode, map.array[gridY][gridX]);
            tileIndex = map.dataIndex[gridY][gridX];
    
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
                                cratePosition == DOOR ||
                                cratePosition == PUDDLE ||
                                map.array[crateY][crateX][0] == 'C'
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
            atoi(tileCode) == DOOR &&
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

        //Collision testing conveyor belt
        if (tileCode[0] == 'C' && !timer){
            goToDir = (Direction)(tileCode[1]-'0');
            players[0].box.y += (((goToDir == UP)*-1) + (goToDir == DOWN)) * TILE_SIZE;
            players[0].box.x += (((goToDir == LEFT)*-1) + (goToDir == RIGHT)) * TILE_SIZE;
            timer = TILE_SIZE;
        }

        //Collect key
        if (
            atoi(tileCode) == KEY &&
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
        UnloadTexture(textures[i]);
    }
    ArenaFree(&arena);
    CloseWindow();
}