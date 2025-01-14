//Structs for singular objects definition
typedef struct Block {int x, y;} Block;
typedef struct Entity {Block box, spr;} Entity;
typedef struct Portals {Block entrance, exit;} Portals;
typedef struct Item {Block position; bool active;} Item;

//Structs for arrays of objects definition
typedef struct CSV {char ***array; int rows, cols;} CSV;
typedef struct BlockArr {Block *instance; int length;} BlockArr;
typedef struct EntityArr {Entity *instance; int length;} EntityArr;
typedef struct PortalsArr {Portals *instance; int length;} PortalsArr;
typedef struct ItemArr {Item *instance; int length;} ItemArr;
typedef struct TextureData {Texture2D texture; char filename[256];} TextureData;

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

//Prints an error and terminates the program
void Error(const char *message){
    printf("Error: %s\n", message);
    exit(1);
}

//Compares if the first value is higher, lower or equal to the second value
int IsHigherOrLower(int num1, int num2){
    if (num1 > num2){
        return 1;
    } else if (num1 < num2){
        return -1;
    } else {
        return 0;
    }
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
int IsAnyOfKeysPressed(int *keys) {
    for (int i = 0; keys[i] != -1; i++) {
        if (IsKeyDown(keys[i])) return keys[i];
    }
    return 0;
}

//Function to create an array with csv cell values for the CSV struct
void LoadCSV(const char *file_name, CSV *csv, int cell_size){
    FILE *file = fopen(file_name, "r");
    if (file == NULL) Error("Unable to open file\n");

    // First pass to get the dimensions of the CSV
    char ch;
    while ((ch = fgetc(file)) != EOF){
        if (ch == ','){
            csv->cols++;
        } else if (ch == '\n'){
            csv->cols = 1;
            csv->rows++;
        }
    }

    //Allocate memory for the array
    csv->array = (char ***)malloc(csv->rows * sizeof(char **));
    if (csv->array == NULL) Error("Memory allocation failed for rows");

    for (int i = 0; i < csv->rows; i++){
        csv->array[i] = (char **)malloc(csv->cols * sizeof(char*));
        if (csv->array[i] == NULL) Error("Memory allocation failed for columns");

        for (int j = 0; j < csv->cols; j++){
            csv->array[i][j] = (char *)malloc((cell_size+1) * sizeof(char));
            if (csv->array[i][j] == NULL) Error("Memory allocation failed for elements");
        }
    }
    
    // Second pass to fill the map
    rewind(file);
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
            if (digit == cell_size-1){
                csv->array[csv->rows][csv->cols][digit+1] = '\0';
            }
            digit++;
        }
    }
    csv->rows++;
    csv->cols++;

    fclose(file);
}

void FreeCSV(CSV *csv){
    for (int i = 0; i < csv->rows; i++){
        for (int j = 0; j < csv->cols; j++){
            free(csv->array[i][j]);
        }
        free(csv->array[i]);
    }
    free(csv->array);
}

// Function to load an image and return a texture
Texture2D LoadTextureFromFile(const char *file_name){
    Image image = LoadImage(file_name);
    if (image.width > 16 || image.height > 16){
        ImageResizeNN(&image, image.width*4, image.height*4);
    } else {
        ImageResizeNN(&image, TILE_SIZE, TILE_SIZE);
    }
    Texture2D texture = LoadTextureFromImage(image);  // Convert image to texture
    UnloadImage(image);  // Unload image from CPU memory (image data is now in GPU memory)
    return texture;
}

//Draws an entity's box and its sprite
void DrawEntity(Entity entity, Color color1, Color color2){
    DrawRectangle(entity.box.x, entity.box.y, TILE_SIZE, TILE_SIZE, color1);
    DrawRectangle(entity.spr.x, entity.spr.y, TILE_SIZE, TILE_SIZE, color2);
}

//Append element to the end of array
void* AppendElement(void* array, int byte_size, int length, void* new_element){
    void* new_array = realloc(array, (length + 1) * byte_size);

    if (new_array == NULL) Error("Error: Unable to allocate memory\n");

    memcpy((char*)new_array + (length * byte_size), new_element, byte_size);
    return new_array;
}

//Clamp function to restrict a value within a specified range
float Clamp(float value, float min, float max){
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

//Loads into an array a map object that's not directly linked to any other object in the same map
void LoadSingularObject(void *data[], const char *object_type, int y, int x){
    Entity *player = (Entity *)data[PLAYER];
    BlockArr *walls = (BlockArr *)data[WALL];
    EntityArr *crates = (EntityArr *)data[CRATE];
    ItemArr *doors = (ItemArr *)data[DOOR];
    ItemArr *keys = (ItemArr *)data[KEY];

    switch (atoi(object_type)){
        case PLAYER:{
            player->spr.y = player->box.y = y * TILE_SIZE;
            player->spr.x = player->box.x = x * TILE_SIZE;
            break;
        }
        case WALL:{
            Block new_wall = {x * TILE_SIZE, y * TILE_SIZE};
            walls->instance = (Block *)AppendElement(walls->instance, sizeof(Block), walls->length, &new_wall);
            walls->length++;
            break;
        }
        case CRATE:{
            Entity new_crate = {{x * TILE_SIZE, y * TILE_SIZE}, {x * TILE_SIZE, y * TILE_SIZE}};
            crates->instance = (Entity *)AppendElement(crates->instance, sizeof(Entity), crates->length, &new_crate);
            crates->length++;
            break;
        }
        case DOOR:{
            Item new_door = {{x * TILE_SIZE, y * TILE_SIZE}, 0};
            doors->instance = (Item *)AppendElement(doors->instance, sizeof(Item), doors->length, &new_door);
            doors->length++;
            break;
        }
        case KEY:{
            Item new_key = {{x * TILE_SIZE, y * TILE_SIZE}, 0};
            keys->instance = (Item *)AppendElement(keys->instance, sizeof(Item), keys->length, &new_key);
            keys->length++;
            break;
        }
    }
}

//Loads into an array a map object that's directly linked to another one in the same map
void LoadLinkedObject(void *data[], const char *object_type, int y, int x){
    PortalsArr *portals = (PortalsArr *)data[PORTAL];

    switch (object_type[0]){
        case 'P':
            int index = object_type[1] - '0';
            Portals new_portal = {{x * TILE_SIZE, y * TILE_SIZE}, {-1, -1}};

            if (!portals->length) {
                portals->instance = (Portals*)AppendElement(portals->instance, sizeof(Portals), portals->length, &new_portal);
                portals->length++;
                return;
            }

            if (portals->instance[index].exit.x == -1 || portals->instance[index].exit.y == -1) {
                portals->instance[index].exit.x = x * TILE_SIZE;
                portals->instance[index].exit.y = y * TILE_SIZE;
            } else {
                portals->instance = (Portals*)AppendElement(portals->instance, sizeof(Portals), portals->length, &new_portal);
                portals->length++;
            }
            break;
    }
}

//Loads all data inside a CSV into their specific arrays
void LoadData(void *data[], CSV *map){
    for (int y = 0; y < map->rows; y++){
        for (int x = 0; x < map->cols; x++){
            if (isdigit(*map->array[y][x])){
                LoadSingularObject(data, map->array[y][x], y, x);
            } else {
                LoadLinkedObject(data, map->array[y][x], y, x);
            }
        }
    }
}

//Loads all images from a folder into an array of textures
int LoadTexturesFromFolder(const char *folder_path, TextureData *textures, int max_textures) {
    int texture_count = 0;

    // Open directory
    DIR *dir = opendir(folder_path);
    if (dir == NULL) Error("Could not open texture directory.\n");

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && texture_count < max_textures){
        // Skip current and parent directory entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        // Construct full file path
        char file_path[512];
        snprintf(file_path, sizeof(file_path), "%s/%s", folder_path, entry->d_name);

        // Load texture
        textures[texture_count].texture = LoadTextureFromFile(file_path);
        strncpy(textures[texture_count].filename, entry->d_name, sizeof(textures[texture_count].filename) - 1);
        texture_count++;
    }
    closedir(dir);

    return texture_count;
}

//Draws every element from the loaded map
void DrawMap(void *data[], TextureData textures[], Rectangle player_frameRec, float *rotation, int *counter){
    Entity *player = (Entity *)data[PLAYER];
    BlockArr *walls = (BlockArr *)data[WALL];
    ItemArr *doors = (ItemArr *)data[DOOR];
    ItemArr *keys = (ItemArr *)data[KEY];
    PortalsArr *portals = (PortalsArr *)data[PORTAL];
    EntityArr *crates = (EntityArr *)data[CRATE];

    //Draw Walls
    for (int i = 0; i < walls->length; i++){
        DrawTexture(textures[WALL].texture, walls->instance[i].x, walls->instance[i].y, GRAY);
    }

    //Draw Doors
    Rectangle door_frameRec = { 0.0f, 0.0f, (float)textures[DOOR].texture.width/2, (float)textures[DOOR].texture.height };
    for (int i = 0; i < doors->length; i++){
        door_frameRec.x = 0;
        if (doors->instance[i].active) door_frameRec.x = 64;
        DrawTextureRec(textures[DOOR].texture, door_frameRec, (Vector2){(float)doors->instance[i].position.x, (float)doors->instance[i].position.y}, WHITE);
    }

    //Draw Keys
    for (int i = 0; i < keys->length; i++){
        if (!keys->instance[i].active){
            DrawTexture(textures[KEY].texture, keys->instance[i].position.x, keys->instance[i].position.y, YELLOW);
        }
    }

    //Draw Portals
    for (int i = 0; i < portals->length; i++){
        Color color = i % 2 ? (Color){128, 0, 255, 255} : GREEN;
        Vector2 entrance = {(float)portals->instance[i].entrance.x, (float)portals->instance[i].entrance.y};
        Vector2 exit = {(float)portals->instance[i].exit.x, (float)portals->instance[i].exit.y};

        Rectangle sourceRec = {0, 0, (float)textures[PORTAL].texture.width, (float)textures[PORTAL].texture.height};
        Rectangle destRec1 = {entrance.x+32, entrance.y+32, (float)textures[PORTAL].texture.width, (float)textures[PORTAL].texture.height};
        Rectangle destRec2 = {exit.x+32, exit.y+32, (float)textures[PORTAL].texture.width, (float)textures[PORTAL].texture.height};
        Vector2 origin = {(float)textures[PORTAL].texture.width / 2, (float)textures[PORTAL].texture.height / 2};

        DrawTexturePro(textures[PORTAL].texture, sourceRec, destRec1, origin, (*rotation)+90*i, color);
        DrawTexturePro(textures[PORTAL].texture, sourceRec, destRec2, origin, (*rotation)+90*(i+1), color);

        (*counter)++;
        if ((*counter) == 180){
            if ((*rotation) == -360) (*rotation) = -90;
            else (*rotation)-=90;
            (*counter) = 0;
        }
    }

    //Draw Player
    DrawEntity(*player, GRAY, RED);
    DrawTextureRec(textures[PLAYER].texture, player_frameRec, (Vector2){(float)player->spr.x, (float)player->spr.y}, BLUE);

    //Draw crates
    for (int i = 0; i < crates->length; i++){
        DrawEntity(crates->instance[i], WHITE, BROWN);
        DrawTexture(textures[CRATE].texture, crates->instance[i].spr.x, crates->instance[i].spr.y, BROWN);
    }
}