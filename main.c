#define MAX_SHOTS 1
float deltaTime;

#include <mainlibs.h>

int main(void){
    const char title[] = "Project";
    float resolutionW = 800, resolutionH = 600;
    int speed = 2, timer = 0;

    //Map object variables
    void *data[END] = { NULL };
    int dataLength[END] = { 0 };
    int structSize[END] = {
        -1,              //Empty space
        sizeof(Entity),  //Player character
        sizeof(Vector2), //Wall
        sizeof(Entity),  //Crate
        sizeof(Item),    //Door
        sizeof(Item),    //Key
        sizeof(Vector2), //Puddle
        sizeof(DirItem), //Conveyor
        sizeof(Portals)  //Portal
    };

    //UI variables
    char keysUI[] = "Keys: 0";
    int counterKeys = 0;

    Direction goToDir = NOWHERE;
    Camera2D camera = { .offset = (Vector2){ resolutionW/2, resolutionH/2 }, .zoom = 1 };

    int cellSize = 2;
    CSV map           = CalcLengthsCSV("map.csv", cellSize, dataLength);
    size_t size       = ArenaCalcMapMemorySize(map, cellSize, dataLength, structSize);
    MemoryArena arena = ArenaMalloc(size);

    GenerateArrayFromCSV("map.csv", &map, 2, &arena);
    ArenaReserveMemoryObjects(&arena, data, dataLength, structSize);
    PopulateData(data, &map, dataLength);

    Entity    *players   =  (Entity *)data[PLAYER];
    //Vector2 *walls     =   (Block *)data[WALL];
    Entity    *crates    =  (Entity *)data[CRATE];
    Item      *doors     =    (Item *)data[DOOR];
    Item      *keys      =    (Item *)data[KEY];
    //Vector2 *puddles   =   (Block *)data[PUDDLE];
    //DirItem *conveyors = (DirItem *)data[CONVEYOR];
    Portals   *portals   = (Portals *)data[PORTAL];

    InitWindow(resolutionW, resolutionH, title);
    SetTargetFPS(60);

    Texture2D textures[END] = { 0 };
    LoadTexturesFromFolder("./images", textures);

    Animation animPlayer = InitAnimValues(&textures[PLAYER], 0, 0.5f, 2);
    Animation animPortal = InitAnimValues(&textures[PORTAL], 0, 5.0f, 1);

    DirItem projectiles[MAX_SHOTS] = { 0 };

    while (!WindowShouldClose()){
        deltaTime = GetFrameTime();
        animPlayer.frameDuration = 0.5f / (speed == 2 ? 1 : 3);
        Entity playerCenter = { GetTileCenter(players[0].box), GetTileCenter(players[0].spr) };

        camera.target = (Vector2){
            Clamp(playerCenter.spr.x, resolutionW / 2, TILE_SIZE * map.cols - resolutionW / 2),
            Clamp(playerCenter.spr.y, resolutionH / 2, TILE_SIZE * map.rows - resolutionH / 2)
        };

        //Start rendering
        HideCursor();
        BeginDrawing();
        ClearBackground(BLACK);
        BeginMode2D(camera);

        //Cursor values
        Vector2 cursor = GetScreenToWorld2D(GetMousePosition(), camera);
        Vector2 origin = { textures[0].width / 2.0f, textures[0].height / 2.0f };
        float    angle = CalculateAngle(playerCenter.spr, cursor);
        float rotation = (((int)(angle + 315) % 360) / 90 * 90); //Snaps rotation to 90 degrees angles

        //Rendering
        DrawMap(&map, data, dataLength, textures, &animPlayer, &animPortal);
        DrawText(keysUI, camera.target.x + 270, camera.target.y + 250, 30, WHITE);
        DrawCursor(textures[0], cursor, origin, rotation, WHITE);

        //Controls
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){
            for (int i = 0; i < MAX_SHOTS; i++){
                if  (projectiles[i].dir) continue;

                projectiles[i].position = playerCenter.box;
                projectiles[i].dir      = ( (rotation / 90) + 1 );

                break;
            }
        }

        for (int i = 0; i < MAX_SHOTS; i++){
            if  (!projectiles[i].dir) continue;

            Vector2 shotTile = {
                SnapToTile(projectiles[i].position.x),
                SnapToTile(projectiles[i].position.y)
            };
            int PosX = shotTile.x, PosY = shotTile.y;

            if (!IsInsideMap(shotTile, map, true)){
                projectiles[i].dir = NOWHERE;
                PosX = PosY = 0;
                continue;
            }

            int currentCode = atoi(map.array[PosY][PosX]);

            if (currentCode == PUDDLE){
                projectiles[i].dir = NOWHERE;
                zapWater(&map, shotTile);
                restoreWater(&map, shotTile);
                continue;
            }

            if (currentCode != PUDDLE && currentCode != EMPTY && currentCode != PLAYER){
                projectiles[i].dir = NOWHERE;
                continue;
            }

            DrawCircleV(projectiles[i].position, 15, YELLOW);
            projectiles[i].position.x += (DetectDestinationDir(projectiles[i].dir, true)  * 30);
            projectiles[i].position.y += (DetectDestinationDir(projectiles[i].dir, false) * 30);
        }

        int moveKeys[] = { KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_W, KEY_S, KEY_A, KEY_D, -1 };
        int keyPressed = IsAnyOfKeysPressed(moveKeys);

        if (keyPressed && !timer){
            switch (keyPressed){
                case KEY_W: case KEY_UP:    { goToDir = UP;    break; }
                case KEY_S: case KEY_DOWN:  { goToDir = DOWN;  break; }
                case KEY_A: case KEY_LEFT:  { goToDir = LEFT;  break; }
                case KEY_D: case KEY_RIGHT: { goToDir = RIGHT; break; }
                default: goToDir = NOWHERE; break;
            }

            players[0].box.y += (DetectDestinationDir(goToDir, false) * TILE_SIZE);
            players[0].box.x += (DetectDestinationDir(goToDir, true)  * TILE_SIZE);
            timer = TILE_SIZE;
        }

        //Collision testing
        int gridX = players[0].box.x / TILE_SIZE;
        int gridY = players[0].box.y / TILE_SIZE;
        int tileIndex;
        char tileCode[cellSize+1];

        if (!IsInsideMap((Vector2){ gridX, gridY }, map, false)){
            players[0].box = players[0].spr;
            timer = 0;
        } else {
            strcpy(tileCode, map.array[gridY][gridX]);
            tileIndex = map.dataIndex[gridY][gridX];
    
            if (isdigit(*tileCode)){
                switch (atoi(tileCode)){
                    case WALL:{
                        players[0].box = players[0].spr;
                        timer = 0;
                        break;
                    }
                    case CRATE:{
                        if (CheckCollisionPoints(crates[tileIndex].box, players[0].box)){
                            if (!CheckCollisionPoints(players[0].box, players[0].spr)){
                                crates[tileIndex].box.y += IsHigherOrLower(players[0].box.y, players[0].spr.y) * TILE_SIZE;
                                crates[tileIndex].box.x += IsHigherOrLower(players[0].box.x, players[0].spr.x) * TILE_SIZE;
                            }

                            int crateY = crates[tileIndex].box.y / TILE_SIZE;
                            int crateX = crates[tileIndex].box.x / TILE_SIZE;
                            bool isInsideBounds = IsInsideMap((Vector2){ crateX, crateY }, map, false);
                            int cratePosition = (isInsideBounds ? atoi(map.array[crateY][crateX]) : -1);

                            if (
                                !isInsideBounds ||
                                map.array[crateY][crateX][0] == 'C' ||
                                (cratePosition != EMPTY && cratePosition != PORTAL)
                            ){
                                players[0].box = players[0].spr;
                                crates[tileIndex].box = crates[tileIndex].spr;
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
        }

        tileIndex = map.dataIndex[gridY][gridX];

        //Collision testing door
        if (
            atoi(tileCode) == DOOR &&
            !doors[tileIndex].active &&
            CheckCollisionPoints(players[0].box, doors[tileIndex].position)
        ){
            if (counterKeys){
                doors[tileIndex].active = true;
                counterKeys--;
                sprintf(keysUI, "Keys: %d", counterKeys);
            } else {
                players[0].box = players[0].spr;
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
            players[0].spr = players[0].box;

            for (int i = 0; i < dataLength[CRATE]; i++){
                if (!CheckCollisionPoints(crates[i].box, crates[i].spr)){
                    crates[i].box = crates[i].spr;
                }
            }
            timer = 0;
        }

        // Collide with portals
        if (tileCode[0] == 'P') CheckCollisionPortals(&players[0], portals[tileIndex], goToDir, &timer, false, &map);
        for (int i = 0; i < dataLength[CRATE]; i++){
            if (map.array[(int)crates->box.y / TILE_SIZE][(int)crates->box.x / TILE_SIZE][0] == 'P'){
                int index = map.dataIndex[(int)crates->box.y / TILE_SIZE][(int)crates->box.x / TILE_SIZE];
                CheckCollisionPortals(&crates[i], portals[index], goToDir, &timer, true, &map);
            }
        }

        //Collision testing conveyor belt
        if (tileCode[0] == 'C' && !timer){
            goToDir = (Direction)(tileCode[1]-'0');
            players[0].box.y += (DetectDestinationDir(goToDir, false) * TILE_SIZE);
            players[0].box.x += (DetectDestinationDir(goToDir, true) * TILE_SIZE);
            timer = TILE_SIZE;
        }

        //Collect key
        if (
            atoi(tileCode) == KEY &&
            !keys[tileIndex].active &&
            CheckCollisionPoints(players[0].spr, keys[tileIndex].position)
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