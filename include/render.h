#include <defndec.h>
#include <csv.h>
#include <util.h>

//Function to load an image and return a texture
Texture2D LoadTextureFromFile(const char *fileName){
    Image image = LoadImage(fileName);
    if (image.width > 16 || image.height > 16) ImageResizeNN(&image, image.width*4, image.height*4);
    else ImageResizeNN(&image, TILE_SIZE, TILE_SIZE);

    Texture2D texture = LoadTextureFromImage(image);  // Convert image to texture
    UnloadImage(image);  // Unload image from CPU memory (image data is now in GPU memory)
    return texture;
}

//Loads all images from a folder into an array of textures
void LoadTexturesFromFolder(const char *folderPath, TextureData *textures){
    FilePathList files = LoadDirectoryFiles(folderPath);
    if (!files.count) Error("Could not open texture directory.\n");

    for (unsigned int i = 0; i < files.count; i++){
        textures[i].texture = LoadTextureFromFile(files.paths[i]);
        strncpy(textures[i].filename, files.paths[i], sizeof(textures[i].filename) - 1);
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
    int frameX = (anim->currentFrame % (anim->texture->width / anim->frameWidth)) * anim->frameWidth;
    int frameY = ((anim->currentFrame / (anim->texture->width / anim->frameWidth)) * anim->frameHeight)+anim->startingRow;

    Rectangle sourceRect = { (float)frameX, (float)frameY, (float)anim->frameWidth, (float)anim->frameHeight};
    Rectangle destRect = { (float)posX, (float)posY, (float)anim->frameWidth, (float)anim->frameHeight};

    if (halved){
        sourceRect.height = (float)anim->frameHeight/2;
        destRect.height = (float)anim->frameHeight/2;
    }
    DrawTexturePro(*anim->texture, sourceRect, destRect, (Vector2){0, 0}, anim->rotation, color);
}

//Function that updates rotates a sprite and draws the result into the specified coordinates
void Rotate(Animation* anim, int posX, int posY, Color color, float degrees, int increment){
    //Update animation
    anim->frameTimer += deltaTime;
    if (anim->frameTimer >= anim->frameDuration){
        anim->frameTimer = 0.0f;
        int direction = pow(degrees, 0);
        if (anim->rotation == 360*direction) anim->rotation = degrees;
        else anim->rotation += degrees*direction;
    }

    //Draw animation
    int frameX = (anim->currentFrame % (anim->texture->width / anim->frameWidth)) * anim->frameWidth;
    int frameY = ((anim->currentFrame / (anim->texture->width / anim->frameWidth)) * anim->frameHeight) + anim->startingRow;

    Rectangle sourceRect = { (float)frameX, (float)frameY, (float)anim->frameWidth, (float)anim->frameHeight };
    Rectangle destRect = { (float)posX + HALF_SIZE, (float)posY + HALF_SIZE, (float)anim->frameWidth, (float)anim->frameHeight};
    DrawTexturePro(*anim->texture, sourceRect, destRect, (Vector2){HALF_SIZE, HALF_SIZE}, anim->rotation + increment, color);
}

//Draws every element from the loaded map
void DrawMap(const CSV *map, void *data[], const int dataLength[], TextureData textures[], Animation *animPlayer, Animation *animPortal){
    Entity *players = (Entity *)data[PLAYER];
    Vector2 *walls = (Vector2 *)data[WALL];
    Vector2 *puddles = (Vector2 *)data[PUDDLE];
    Item *doors = (Item *)data[DOOR];
    Item *keys = (Item *)data[KEY];
    Portals *portals = (Portals *)data[PORTAL];
    Entity *crates = (Entity *)data[CRATE];

    //Draw Walls
    for (int i = 0; i < dataLength[WALL]; i++){
        DrawTexture(textures[WALL].texture, walls[i].x, walls[i].y, GRAY);
    }

    //Draw Puddles
    for (int i = 0; i < dataLength[PUDDLE]; i++){
        DrawRectangle(puddles[i].x, puddles[i].y, TILE_SIZE, TILE_SIZE, BLUE);
    }

    //Draw Doors
    Rectangle doorFrameRec = { 0.0f, 0.0f, (float)textures[DOOR].texture.width/2, (float)textures[DOOR].texture.height };
    for (int i = 0; i < dataLength[DOOR]; i++){
        doorFrameRec.x = 0;
        if (doors[i].active) doorFrameRec.x = 64;
        DrawTextureRec(textures[DOOR].texture, doorFrameRec, (Vector2){(float)doors[i].position.x, (float)doors[i].position.y}, WHITE);
    }

    //Draw Keys
    for (int i = 0; i < dataLength[KEY]; i++){
        if (!keys[i].active){
            DrawTexture(textures[KEY].texture, keys[i].position.x, keys[i].position.y, YELLOW);
        }
    }

    //Draw Portals
    for (int i = 0; i < ((dataLength[PORTAL]/2) - (dataLength[PORTAL] % 2)); i++){
        Color color = i&1 ? (Color){128, 0, 255, 255} : GREEN;
        Vector2 entrance = {(float)portals[i].entrance.x, (float)portals[i].entrance.y};
        Vector2 exit = {(float)portals[i].exit.x, (float)portals[i].exit.y};

        Rotate(animPortal, entrance.x, entrance.y, color, -90, 90*i);
        Rotate(animPortal, exit.x, exit.y, color, -90, 90*(i+1));
    }

    //Draw Player
    int yStart = players[0].box.y / TILE_SIZE, xStart = players[0].box.x / TILE_SIZE;
    int yEnd = yStart, xEnd = xStart;
    bool halved = false;

    if (atoi(map->array[yStart][xStart]) == PUDDLE){
        if (yStart < map->rows && atoi(map->array[yStart+1][xStart]) == PUDDLE) yEnd++;
        if (xStart < map->cols && atoi(map->array[yStart][xStart+1]) == PUDDLE) xEnd++;
        if (yStart > 0 && atoi(map->array[yStart-1][xStart]) == PUDDLE) yStart--;
        if (xStart > 0 && atoi(map->array[yStart][xStart-1]) == PUDDLE) xStart--;

        if (
            players[0].spr.x >= xStart * TILE_SIZE && players[0].spr.x <= xEnd * TILE_SIZE &&
            players[0].spr.y >= yStart * TILE_SIZE && players[0].spr.y <= yEnd * TILE_SIZE
        ) halved = true;
    }

    DrawRectangle(players[0].box.x, players[0].box.y, TILE_SIZE, TILE_SIZE, GRAY);
    Animate(animPlayer, players[0].spr.x, players[0].spr.y, BLUE, halved);

    //Draw crates
    for (int i = 0; i < dataLength[CRATE]; i++){
        DrawRectangle(crates[i].box.x, crates[i].box.y, TILE_SIZE, TILE_SIZE, WHITE);
        DrawTexture(textures[CRATE].texture, crates[i].spr.x, crates[i].spr.y, BROWN);
    }
}