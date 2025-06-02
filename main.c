float deltaTime;

#include <mainlibs.h>

int main(void){
    Window window = { "Project", 800, 600, 400, 300 };
    int timer = 0;

    //Map object variables
    void *data[END] = { 0 };
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
    UI_Element keysUI = { "Keys: 0", 0 };

    Direction goToDir = NOWHERE;
    Camera2D camera = { .offset = (Vector2){ window.width/2, window.height/2 }, .zoom = 1 };

    CSV map           = CalcLengthsCSV("map.csv", dataLength);
    MemoryArena arena = ArenaMalloc(CalcMapMemorySize(map, dataLength, structSize));

    GenerateArrayFromCSV("map.csv", &map, &arena);
    ArenaReserveObjects(&arena, data, dataLength, structSize);
    PopulateData(data, &map, dataLength);

    InitWindow(window.width, window.height, window.title);
    SetTargetFPS(60);

    Texture2D textures[END] = { 0 };
    LoadTexturesFromFolder("./images", textures);

    Animation animPlayer = InitAnimValues(&textures[PLAYER], 0, 0.5f, 2);
    Animation animPortal = InitAnimValues(&textures[PORTAL], 0, 5.0f, 1);

    DirItem projectiles[MAX_SHOTS] = { 0 };

    while (!WindowShouldClose()){
        deltaTime = GetFrameTime();

        Entity *players = (Entity *)data[PLAYER];
        Entity playerCenter = GetEntityCenter(players[0]);

        camera.target = (Vector2){
            Clamp(playerCenter.spr.x, window.center_w, TILE_SIZE * map.cols - window.center_w),
            Clamp(playerCenter.spr.y, window.center_h, TILE_SIZE * map.rows - window.center_h)
        };

        //Start rendering
        HideCursor();
        BeginDrawing();
        ClearBackground(BLACK);
        BeginMode2D(camera);

        //Cursor values
        Vector2 cursor = GetScreenToWorld2D(GetMousePosition(), camera);
        int      angle = CalculateAngle(playerCenter.spr, cursor);
        float rotation = ((angle + 315) % 360 / 90 * 90); //Snaps rotation to 90 degrees angles

        //Rendering
        DrawMap(&map, data, dataLength, textures, &animPlayer, &animPortal);
        DrawText(keysUI.display, camera.target.x + 270, camera.target.y + 250, 30, WHITE);
        DrawCursor(textures[0], cursor, (Vector2){ HALF_SIZE, HALF_SIZE }, rotation, WHITE);

        //Controls
        int moveKeys[] = { KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_W, KEY_S, KEY_A, KEY_D, -1 };
        int keyPressed = IsAnyOfKeysPressed(moveKeys);
        int speed = 2 * (IsKeyDown(KEY_LEFT_SHIFT) + 1);

        if (!timer){
            if (keyPressed){
                InitiateMove(keyPressed, &goToDir, &timer, &players[0]);
            } else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){
                ShootProjectile(projectiles, playerCenter.box, ( (rotation / 90) + 1 ));
            }
        }

        //Collision testing
        TileInfo currentTile = { players[0].box.x / TILE_SIZE, players[0].box.y / TILE_SIZE, -1, "" };

        //Move and collide
        HandleProjectiles(map, projectiles);
        MoveAndCollide(&currentTile, &map, data, dataLength, &keysUI, &goToDir, speed, &timer);

        animPlayer.frameDuration = 0.5f / (speed == 2 ? 1 : 3);
        
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