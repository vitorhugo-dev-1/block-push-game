#pragma once

#define TILE_SIZE 64

//Declaring structs for general usage
typedef struct MemoryArena {void* buffer; size_t size, used;} MemoryArena;
typedef struct CSV {char ***array; int rows, cols;} CSV;
typedef struct TextureData {Texture2D texture; char filename[256];} TextureData;

//Declaring structs for map objects
typedef struct Block {int x, y;} Block;
typedef struct Entity {Block box, spr;} Entity;
typedef struct Portals {Block entrance, exit;} Portals;
typedef struct Item {Block position; bool active;} Item;

typedef struct Animation {
    Texture2D *texture;   //Sprite sheet
    int startingRow;      //Texture row from where each frame will be displayed
    int frameWidth;
    int frameHeight;
    int frameCount;       // Total number of frames
    int currentFrame;
    float frameDuration;  // Duration of each frame in seconds
    float frameTimer;
    float rotation;
} Animation;

typedef enum Dictionary {
    EMPTY,
    PLAYER,
    WALL,
    CRATE,
    PORTAL,
    DOOR,
    KEY,
    END
} Dictionary;

typedef enum Direction {
    NOWHERE,
    UP,
    DOWN,
    RIGHT,
    LEFT
} Direction;

//Initializes Animation struct with the informed values
Animation InitAnimValues(Texture2D *texture, int starting_row, float duration, int totalFrames){
    Animation anim = {
        .texture = texture,
        .startingRow = starting_row,
        .frameWidth = TILE_SIZE,
        .frameHeight = TILE_SIZE,
        .frameCount = totalFrames,
        .currentFrame = 0,
        .frameDuration = duration,
        .frameTimer = 0.0f
    };
    return anim;
};

//Prints an error and terminates the program
void Error(const char *message){
    fprintf(stderr, "%s", message);
    exit(EXIT_FAILURE);
}

//Compares if the first value is higher, lower or equal to the second value
int IsHigherOrLower(int num1, int num2){
    if (num1 > num2){return 1;}
    else if (num1 < num2){return -1;}
    return 0;
}

//Checks if the informed position is inside the informed map
int IsInsideMap(Block block, CSV map){
    if (
        (block.x) < 0 ||
        (block.x) >= map.cols ||
        (block.y) < 0 ||
        (block.y) >= map.rows
    ){
        return false;
    }
    return true;
}

//Checks if any of the passed keys are pressed
int IsAnyOfKeysPressed(int *keys){
    for (int i = 0; keys[i] != -1; i++){
        if (IsKeyDown(keys[i])) return keys[i];
    }
    return 0;
}

//Initializes MemoryArena struct and allocates the informed size
void ArenaMalloc(MemoryArena* arena, size_t size){
    arena->size = size;
    arena->used = 0;
    arena->buffer = malloc(size);
    if (arena->buffer == NULL) Error("Failed to allocate memory\n");
}

// Frees the memory allocated for the informed arena
void ArenaFree(MemoryArena* arena){
    free(arena->buffer);
    arena->buffer = NULL;
    arena->size = 0;
    arena->used = 0;
}

//Reserves memory from the arena
void* ArenaReserveMemory(MemoryArena* arena, size_t size){
    if (arena->used + size > arena->size){
        printf("%lld/%lld\n", arena->used, arena->size);
        ArenaFree(arena);
        Error("Out of memory\n");
    }

    void* result = (char*)arena->buffer + arena->used;
    arena->used += size;
    return result;
}

//Reserves memory for the game's objects
void ArenaReserveMemoryObjects(MemoryArena *arena, void *data[], int data_length[], int struct_size[]){
    data[PLAYER] =  (Entity *)ArenaReserveMemory(arena, data_length[PLAYER] * struct_size[PLAYER]);
    data[WALL]   =   (Block *)ArenaReserveMemory(arena, data_length[WALL]   * struct_size[WALL]);
    data[CRATE]  =  (Entity *)ArenaReserveMemory(arena, data_length[CRATE]  * struct_size[CRATE]);
    data[DOOR]   =    (Item *)ArenaReserveMemory(arena, data_length[DOOR]   * struct_size[DOOR]);
    data[KEY]    =    (Item *)ArenaReserveMemory(arena, data_length[KEY]    * struct_size[KEY]);
    data[PORTAL] = (Portals *)ArenaReserveMemory(arena, ((data_length[PORTAL]/2)-(data_length[PORTAL] % 2)) * struct_size[PORTAL]);
}

//Reset the memory arena
void ArenaReset(MemoryArena* arena){
    arena->used = 0;
}

//Calculates how many bytes of memory the map data will use
size_t ArenaCalcMapMemorySize(CSV map, const int cellSize, const int dataLength[], const int structSize[]){
    size_t size = (map.rows * sizeof(char **)) +
                  (map.rows * (map.cols * sizeof(char *))) +
                  (map.rows * map.cols * (cellSize+1) * sizeof(char));

    for (int i = 1; i < END; i++){
        size += dataLength[i] * structSize[i];
    }
    return size;
}

//Calculates how many elements are present in a CSV file and
//how many of each object are present in that same CSV
void calcSizesCSV(const char *file_name, CSV *csv, int cell_size, int data_length[]){
    FILE *file = fopen(file_name, "r");
    if (file == NULL) Error("Unable to open file\n");

    char element[cell_size];
    int counter = 0;
    char ch;
    csv->cols = csv->rows = 1;
    while ((ch = fgetc(file)) != EOF){
        if (ch == ','){
            csv->cols++;
        } else if (ch == '\n'){
            csv->cols = 1;
            csv->rows++;
        } else {
            element[counter] = ch;
            counter++;
            if (counter != cell_size) continue;
            if (isdigit(*element)){
                data_length[atoi(element)]++;
            } else {
                switch (element[0]){
                    case 'P':
                        data_length[PORTAL]++;
                        break;
                }
            }
            counter = 0;
        }
    }
    fclose(file);
}

//Function to generate a 2D array with csv cell values for the CSV struct
void GenerateArrayFromCSV(const char *file_name, CSV *csv, int cell_size, MemoryArena *arena){
    FILE *file = fopen(file_name, "r");
    if (file == NULL) Error("Unable to open file\n");

    //Allocate memory
    csv->array = (char ***)ArenaReserveMemory(arena, csv->rows * sizeof(char **));
    for (int i = 0; i < csv->rows; i++){
        csv->array[i] = (char **)ArenaReserveMemory(arena, csv->cols * sizeof(char *));
        for (int j = 0; j < csv->cols; j++){
            csv->array[i][j] = (char *)ArenaReserveMemory(arena, (cell_size + 1) * sizeof(char));
            memset(csv->array[i][j], 0, (cell_size + 1) * sizeof(char));
        }
    }

    //Fill array
    char ch;
    int digit = 0;
    csv->cols = csv->rows = 0;
    while ((ch = fgetc(file)) != EOF){
        if (ch == ','){
            csv->cols++;
            digit = 0;
        } else if (ch == '\n'){
            csv->cols = 0;
            csv->rows++;
            digit = 0;
        } else {
            csv->array[csv->rows][csv->cols][digit] = ch;
            if (digit == cell_size - 1){
                csv->array[csv->rows][csv->cols][digit + 1] = '\0';
            }
            digit++;
        }
    }
    csv->rows++;
    csv->cols++;

    fclose(file);
}

//Function to load an image and return a texture
Texture2D LoadTextureFromFile(const char *file_name){
    Image image = LoadImage(file_name);
    if (image.width > 16 || image.height > 16) ImageResizeNN(&image, image.width*4, image.height*4);
    else ImageResizeNN(&image, TILE_SIZE, TILE_SIZE);
    
    Texture2D texture = LoadTextureFromImage(image);  // Convert image to texture
    UnloadImage(image);  // Unload image from CPU memory (image data is now in GPU memory)
    return texture;
}

//Loads into an array a map object that's not directly linked to any other object in the same map
void LoadSingularObject(void *data[], const int object_type, const int iteration, Block position){
    Entity *players = (Entity *)data[PLAYER];
    Block *walls = (Block *)data[WALL];
    Entity *crates = (Entity *)data[CRATE];
    Item *doors = (Item *)data[DOOR];
    Item *keys = (Item *)data[KEY];

    switch (object_type){
        case PLAYER:{
            Entity new_player = {position, position};
            players[iteration] = new_player;
            break;
        }
        case WALL:{
            Block new_wall = position;
            walls[iteration] = new_wall;
            break;
        }
        case CRATE:{
            Entity new_crate = {position, position};
            crates[iteration] = new_crate;
            break;
        }
        case DOOR:{
            Item new_door = {position, false};
            doors[iteration] = new_door;
            break;
        }
        case KEY:{
            Item new_key = {position, false};
            keys[iteration] = new_key;
            break;
        }
    }
}

//Loads into an array a map object that's directly linked to another one in the same map
void LoadLinkedObject(void *data[], const char *object_type, int iterator[], const int data_length[], Block position){
    Portals *portals = (Portals *)data[PORTAL];

    int index = object_type[1] - '0';
    switch (object_type[0]){
        case 'P':
            if (iterator[PORTAL] > data_length[PORTAL]) Error("Array index out of bounds\n");
            Portals new_portal = {position, {-1, -1}};
            iterator[PORTAL]++;

            if (!iterator[PORTAL]){
                portals[iterator[PORTAL]] = new_portal;
                return;
            }

            if (portals[index].exit.x == -1 || portals[index].exit.y == -1){
                portals[index].exit.x = position.x;
                portals[index].exit.y = position.y;
            } else {
                portals[index] = new_portal;
            }
            break;
    }
}

//Loads all data inside a CSV into their specific arrays
void PopulateData(void *data[], const CSV *map, const int data_length[]){
    int iterator[END];
    memset(iterator, 0, sizeof(iterator));

    for (int y = 0; y < map->rows; y++){
        for (int x = 0; x < map->cols; x++){
            Block position = {x * TILE_SIZE, y * TILE_SIZE};
            if (isdigit(*map->array[y][x])){
                int object_type = atoi(map->array[y][x]);
                if (iterator[object_type] > data_length[object_type]) Error("Array index out of bounds\n");
                LoadSingularObject(data, object_type, iterator[object_type], position);
                iterator[object_type]++;
            } else {
                LoadLinkedObject(data, map->array[y][x], iterator, data_length, position);
            }
        }
    }
}

//Loads all images from a folder into an array of textures
void LoadTexturesFromFolder(const char *folder_path, TextureData *textures){
    FilePathList files = LoadDirectoryFiles(folder_path);
    if (!files.count) Error("Could not open texture directory.\n");

    for (unsigned int i = 0; i < files.count; i++){
        textures[i].texture = LoadTextureFromFile(files.paths[i]);
        strncpy(textures[i].filename, files.paths[i], sizeof(textures[i].filename) - 1);
    }

    UnloadDirectoryFiles(files);
}

//Function that updates an animation and draws it into the specified coordinates
void Animate(Animation* anim, int posX, int posY, Color color){
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

    Rectangle sourceRect = { (float)frameX, (float)frameY, (float)anim->frameWidth, (float)anim->frameHeight };
    Rectangle destRect = { (float)posX, (float)posY, (float)anim->frameWidth, (float)anim->frameHeight};
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
    int frameY = ((anim->currentFrame / (anim->texture->width / anim->frameWidth)) * anim->frameHeight)+anim->startingRow;

    Rectangle sourceRect = { (float)frameX, (float)frameY, (float)anim->frameWidth, (float)anim->frameHeight };
    Rectangle destRect = { (float)posX+32, (float)posY+32, (float)anim->frameWidth, (float)anim->frameHeight};
    DrawTexturePro(*anim->texture, sourceRect, destRect, (Vector2){32, 32}, anim->rotation+increment, color);
}

//Draws every element from the loaded map
void DrawMap(void *data[], const int data_length[], TextureData textures[], Animation *anim_player, Animation *anim_portal){
    Entity *players = (Entity *)data[PLAYER];
    Block *walls = (Block *)data[WALL];
    Item *doors = (Item *)data[DOOR];
    Item *keys = (Item *)data[KEY];
    Portals *portals = (Portals *)data[PORTAL];
    Entity *crates = (Entity *)data[CRATE];

    //Draw Walls
    for (int i = 0; i < data_length[WALL]; i++){
        DrawTexture(textures[WALL].texture, walls[i].x, walls[i].y, GRAY);
    }

    //Draw Doors
    Rectangle door_frameRec = { 0.0f, 0.0f, (float)textures[DOOR].texture.width/2, (float)textures[DOOR].texture.height };
    for (int i = 0; i < data_length[DOOR]; i++){
        door_frameRec.x = 0;
        if (doors[i].active) door_frameRec.x = 64;
        DrawTextureRec(textures[DOOR].texture, door_frameRec, (Vector2){(float)doors[i].position.x, (float)doors[i].position.y}, WHITE);
    }

    //Draw Keys
    for (int i = 0; i < data_length[KEY]; i++){
        if (!keys[i].active){
            DrawTexture(textures[KEY].texture, keys[i].position.x, keys[i].position.y, YELLOW);
        }
    }

    //Draw Portals
    for (int i = 0; i < ((data_length[PORTAL]/2)-(data_length[PORTAL] % 2)); i++){
        Color color = i&1 ? (Color){128, 0, 255, 255} : GREEN;
        Vector2 entrance = {(float)portals[i].entrance.x, (float)portals[i].entrance.y};
        Vector2 exit = {(float)portals[i].exit.x, (float)portals[i].exit.y};

        Rotate(anim_portal, entrance.x, entrance.y, color, -90, 90*i);
        Rotate(anim_portal, exit.x, exit.y, color, -90, 90*(i+1));
    }

    //Draw Player
    DrawRectangle(players[0].box.x, players[0].box.y, TILE_SIZE, TILE_SIZE, GRAY);
    Animate(anim_player, players[0].spr.x, players[0].spr.y, BLUE);

    //Draw crates
    for (int i = 0; i < data_length[CRATE]; i++){
        DrawRectangle(crates[i].box.x, crates[i].box.y, TILE_SIZE, TILE_SIZE, WHITE);
        DrawTexture(textures[CRATE].texture, crates[i].spr.x, crates[i].spr.y, BROWN);
    }
}

// Returns true if block1's coordinates are equal to block2's
bool CheckCollisionBlocks(Block block1, Block block2){
    return (block1.x == block2.x && block1.y == block2.y ? true : false);
}

// Returns true if block1's coordinates are equal to block2's
void CheckCollisionPortals(Entity *entity, Portals portals[], Direction goToDir, const int length, int *timer, bool isObj, CSV *map){
    for (int i = 0; i < length; i++){
        Portals current = portals[i];
        if (
            CheckCollisionBlocks(entity->box, entity->spr) &&
            (
                CheckCollisionBlocks(entity->box, current.entrance) ||
                CheckCollisionBlocks(entity->box, current.exit)
            )
        ){
            int portalY = (entity->box.y == current.entrance.y) ? current.exit.y : current.entrance.y;
            int portalX = (entity->box.x == current.entrance.x) ? current.exit.x : current.entrance.x;

            int oldY = entity->box.y/TILE_SIZE;
            int oldX = entity->box.x/TILE_SIZE;

            entity->spr.y = portalY;
            entity->spr.x = portalX;
            entity->box.y = portalY + (((goToDir == UP) * -1) + (goToDir == DOWN)) * TILE_SIZE;
            entity->box.x = portalX + (((goToDir == LEFT) * -1) + (goToDir == RIGHT)) * TILE_SIZE;
            *timer = TILE_SIZE;

            if (isObj){
                strcpy(map->array[oldY][oldX], "00");
                strcpy(map->array[entity->box.y/TILE_SIZE][entity->box.x/TILE_SIZE], "03");
            }
        }
    }
}