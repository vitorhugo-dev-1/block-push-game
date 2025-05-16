#include <defndec.h>
#include <csv.h>
#include <util.h>

//Function to load an image and return a texture
Texture2D LoadTextureFromFile(const char *fileName){
    Image image = LoadImage(fileName);
    
    if (image.width > 16 || image.height > 16)
        ImageResizeNN(&image, image.width * 4, image.height * 4);
    else
        ImageResizeNN(&image, TILE_SIZE, TILE_SIZE);

    Texture2D texture = LoadTextureFromImage(image);  // Convert image to texture
    UnloadImage(image);  // Unload image from CPU memory (image data is now in GPU memory)
    return texture;
}

//Loads all images from a folder into an array of textures
void LoadTexturesFromFolder(const char *folderPath, Texture2D *textures){
    FilePathList files = LoadDirectoryFiles(folderPath);
    if (!files.count) Error("Could not open texture directory.\n");

    for (unsigned int i = 0; i < files.count; i++){
        char fileCode[3];
        char *fileName = strrchr(files.paths[i], '\\') + 1;
        strncpy(fileCode, fileName, 2);
        int objectCode = atoi(fileCode);

        if (objectCode >= END){
            printf(
                "WARNING: Texture with ID=%d is not one of the known objects, "
                "will continue without it.\n", objectCode
            );
            continue;
        }

        textures[objectCode] = LoadTextureFromFile(files.paths[i]);
    }

    for (unsigned int i = 0; i < END; i++){
        if (!textures[i].height) 
            printf("WARNING: Couldn't find texture with ID=%d, proceeding without it.\n", i);
    }

    UnloadDirectoryFiles(files);
}

//Function that updates an animation and draws it into the specified coordinates
void Animate(Animation* anim, int posX, int posY, Color color, bool halved){
    //Update animation
    anim->frameTimer += deltaTime;
    if (anim->frameTimer >= anim->frameDuration){
        anim->frameTimer = 0.0f;
        anim->currentFrame++;
        if (anim->currentFrame >= anim->frameCount){
            anim->currentFrame = 0;
        }
    }

    //Draw animation
    int totalFrames = anim->texture->width / anim->frameWidth;
    int frameX      = anim->currentFrame % totalFrames * anim->frameWidth;
    int frameY      = anim->currentFrame / totalFrames * anim->frameHeight + anim->startingRow;

    Rectangle source = { frameX, frameY, anim->frameWidth, anim->frameHeight };
    Rectangle dest   = {  posX ,  posY , anim->frameWidth, anim->frameHeight };
    Vector2 origin   = { 0 };

    if (halved) source.height = dest.height = anim->frameHeight / 2;

    DrawTexturePro(*anim->texture, source, dest, origin, anim->rotation, color);
}

//Function that updates rotates a sprite and draws the result into the specified coordinates
void Rotate(Animation* anim, int posX, int posY, Color color, float degrees, int increment){
    //Update animation
    anim->frameTimer += deltaTime;
    if (anim->frameTimer >= anim->frameDuration){
        anim->frameTimer = 0.0f;
        anim->rotation = (anim->rotation == 360 ? degrees : anim->rotation + degrees);
    }

    //Draw animation
    int totalFrames = anim->texture->width / anim->frameWidth;
    int frameX      = anim->currentFrame % totalFrames * anim->frameWidth;
    int frameY      = anim->currentFrame / totalFrames * anim->frameHeight + anim->startingRow;

    Rectangle source = {      frameX     ,      frameY     , anim->frameWidth, anim->frameHeight };
    Rectangle dest   = { posX + HALF_SIZE, posY + HALF_SIZE, anim->frameWidth, anim->frameHeight };
    Vector2   origin = { HALF_SIZE, HALF_SIZE };

    DrawTexturePro(*anim->texture, source, dest, origin, anim->rotation + increment, color);
}

//Draws every element from the loaded map
void DrawMap(
    const CSV *map,
    void *data[],
    const int dataLength[],
    const Texture2D textures[],
    Animation *animPlayer,
    Animation *animPortal
){
    //Draw Walls
    for (int i = 0; i < dataLength[WALL]; i++){
        Vector2 *walls = (Vector2 *)data[WALL];
        DrawTexture(textures[WALL], walls[i].x, walls[i].y, GRAY);
    }

    //Draw Puddles
    for (int i = 0; i < dataLength[PUDDLE]; i++){
        Vector2 *puddles = (Vector2 *)data[PUDDLE];
        DrawRectangle(puddles[i].x, puddles[i].y, TILE_SIZE, TILE_SIZE, BLUE);
    }

    //Draw Conveyor Belts
    for (int i = 0; i < dataLength[CONVEYOR]; i++){
        DirItem *conveyors = (DirItem *)data[CONVEYOR];
        int rotation = (conveyors[i].dir * 90) - 90;
        DrawTexturePro(textures[CONVEYOR], (Rectangle){ 0, 0, textures[CONVEYOR].width, textures[CONVEYOR].height }, (Rectangle){conveyors[i].position.x + HALF_SIZE, conveyors[i].position.y + HALF_SIZE, (float)textures[CONVEYOR].width, (float)textures[CONVEYOR].height}, (Vector2){HALF_SIZE, HALF_SIZE}, rotation, WHITE);
    }

    //Draw Doors
    Rectangle doorFrameRec = { 0.0f, 0.0f, textures[DOOR].width/2, textures[DOOR].height };
    for (int i = 0; i < dataLength[DOOR]; i++){
        Item *doors = (Item *)data[DOOR];
        if (doors[i].active) doorFrameRec.x = 64;
        DrawTextureRec(textures[DOOR], doorFrameRec, doors[i].position, WHITE);
    }

    //Draw Keys
    for (int i = 0; i < dataLength[KEY]; i++){
        Item *keys = (Item *)data[KEY];
        if (!keys[i].active) DrawTexture(textures[KEY], keys[i].position.x, keys[i].position.y, YELLOW);
    }

    //Draw Portals
    for (int i = 0; i < ((dataLength[PORTAL]/2) - (dataLength[PORTAL] % 2)); i++){
        Portals *portals = (Portals *)data[PORTAL];
        Vector2 entrance = portals[i].entrance;
        Vector2 exit     = portals[i].exit;
        Color   color    = i&1 ? (Color){ 128, 0, 255, 255 } : GREEN;

        Rotate(animPortal, entrance.x, entrance.y, color, -90, 90 * i);
        Rotate(animPortal,   exit.x  ,   exit.y  , color, -90, 90 * (i + 1));
    }

    //Draw Player
    Entity *players = (Entity *)data[PLAYER];
    int yStart = players[0].box.y / TILE_SIZE, xStart = players[0].box.x / TILE_SIZE;
    int yEnd = yStart, xEnd = xStart;
    bool halved = false;

    if (atoi(map->array[yStart][xStart]) == PUDDLE){
        if (yStart < map->rows && atoi(map->array[yStart+1][ xStart ]) == PUDDLE) yEnd++  ;
        if (xStart < map->cols && atoi(map->array[ yStart ][xStart+1]) == PUDDLE) xEnd++  ;
        if (yStart >     0     && atoi(map->array[yStart-1][ xStart ]) == PUDDLE) yStart--;
        if (xStart >     0     && atoi(map->array[ yStart ][xStart-1]) == PUDDLE) xStart--;

        if (
            players[0].spr.x >= xStart * TILE_SIZE && players[0].spr.x <= xEnd * TILE_SIZE &&
            players[0].spr.y >= yStart * TILE_SIZE && players[0].spr.y <= yEnd * TILE_SIZE
        ) halved = true;
    }

    DrawRectangle(players[0].box.x, players[0].box.y, TILE_SIZE, TILE_SIZE, GRAY);
    Animate(animPlayer, players[0].spr.x, players[0].spr.y, BLUE, halved);

    //Draw crates
    for (int i = 0; i < dataLength[CRATE]; i++){
        Entity *crates = (Entity *)data[CRATE];
        //DrawRectangle(crates[i].box.x, crates[i].box.y, TILE_SIZE, TILE_SIZE, WHITE);
        DrawTexture(textures[CRATE], crates[i].spr.x, crates[i].spr.y, BROWN);
    }
}

void DrawCursor(Texture2D texture, Vector2 cursor, Vector2 origin, int rotation, Color color){
    Rectangle source = {    0    ,    0    , texture.width, texture.height };
    Rectangle dest   = { cursor.x, cursor.y, texture.width, texture.height };

    DrawTexturePro(texture, source, dest, origin, rotation, color);
}