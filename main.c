float deltaTime;

#include <core_includes.h>

static void InitGame(Map *map, MemoryArena *arena){
    size_t structSize[TILE_END] = {
        0,               // Empty space
        sizeof(Entity),  // Player character
        sizeof(Vector2), // Wall
        sizeof(Entity),  // Crate
        sizeof(Item),    // Door
        sizeof(Item),    // Key
        sizeof(Vector2), // Puddle
        sizeof(DirItem), // Conveyor
        sizeof(Portals)  // Portal
    };

    *map   = CountMapObjectsInCSV(MAP_FILENAME);
    *arena = ArenaMalloc(CalcMapMemorySize(*map, structSize));

    GenerateArrayFromCSV(MAP_FILENAME, map, arena);
    ArenaReserveObjects(arena, map, structSize);
    FillMapObjectArrays(map);
}

int main(void){
    Window window = { WINDOW_TITLE, WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 };

    // UI variables
    UIElement keysUI = { "Keys: 0", 0 };

    // Level information variables
    Map map = { 0 };
    MemoryArena arena = { 0 };

    InitGame(&map, &arena);
    InitWindowAndSetFPS(&window, TARGET_FPS);

    Texture2D textures[TILE_END] = { 0 };
    LoadTexturesFromFolder(TEXTURE_FOLDER, textures);

    Animation anim[ANIM_END] = { 0 };
    anim[ANIM_PLAYER] = InitAnimValues(&textures[TILE_PLAYER], 0, 0.5f);
    anim[ANIM_PORTAL] = InitAnimValues(&textures[TILE_PORTAL], 0, 2.5f);

    MoveParams move = { DIR_NONE, 0, 0 };
    Camera2D  camera = { .offset = (Vector2){ window.width / 2, window.height / 2 }, .zoom = 1 };
    DirItem projectiles[MAX_SHOTS] = { 0 };

    while (!WindowShouldClose()){
        deltaTime = GetFrameTime();

        Entity *players = (Entity *)map.objects[TILE_PLAYER];
        Entity playerCenter = GetEntityCenter(players[0]);

        camera.target = (Vector2){
            Clamp(playerCenter.spr.x, window.vCenter, TILE_SIZE * map.cols - window.vCenter),
            Clamp(playerCenter.spr.y, window.hCenter, TILE_SIZE * map.rows - window.hCenter)
        };

        // Cursor values
        Cursor mouse = { GetScreenToWorld2D(GetMousePosition(), camera), 0.0f };
        mouse.angle  = SnapTo90Degrees(CalculateAngle(playerCenter.spr, mouse.position), 315);

        Assets assets = { textures, anim };
        ScreenContext ctx = { &keysUI, &camera, mouse };

        RenderGame(&map, assets, ctx);

        // Controls
        int moveKeys[] = { KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_W, KEY_S, KEY_A, KEY_D };
        int keyPressed = IsAnyOfKeysPressed(moveKeys, ARRAY_LENGTH(moveKeys));
        move.speed = (1 + IsKeyDown(KEY_LEFT_SHIFT)) * 2;

        if (!move.timer){
            if (keyPressed){
                InitiateMovement(keyPressed, &move, &players[0]);
            } else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){
                ShootProjectile(projectiles, playerCenter.box, AngleToDirection(mouse.angle, 1));
            }
        }

        // Collision testing
        TileInfo currentTile = {
            players[0].box.x / TILE_SIZE,
            players[0].box.y / TILE_SIZE,
            -1, ""
        };

        // Move and collide
        HandleProjectiles(map, projectiles);
        MoveAndCollide(&currentTile, &map, &keysUI, &move);

        anim[ANIM_PLAYER].durationModifier = (move.speed == 2 ? 1 : 3);

        EndMode2D();
        EndDrawing();
    }

    // Frees memory and ends the program
    for (unsigned int i = 0; i < TILE_END; i++){
        UnloadTexture(textures[i]);
    }
    ArenaFree(&arena);
    CloseWindow();
}