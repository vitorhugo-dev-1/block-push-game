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

typedef enum Dictionary {
    EMPTY,
    PLAYER,
    WALL,
    CRATE,
    PORTAL,
    DOOR,
    KEY
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
void LoadCSV(const char *fileName, CSV *csv, int cellSize){
    FILE *file = fopen(fileName, "r");
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
            csv->array[i][j] = (char *)malloc((cellSize+1) * sizeof(char));
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
            if (digit == cellSize-1){
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
Texture2D LoadTextureFromFile(const char *fileName) {
    Image image = LoadImage(fileName);
    ImageResizeNN(&image, TILE_SIZE, TILE_SIZE);
    Texture2D texture = LoadTextureFromImage(image);  // Convert image to texture
    UnloadImage(image);  // Unload image from CPU memory (image data is now in GPU memory)
    return texture;
}

//Draws an entity's box and its sprite
void DrawEntity(Entity entity, Color color1, Color color2){
    DrawRectangle(entity.box.x, entity.box.y, TILE_SIZE, TILE_SIZE, color1);
    DrawRectangle(entity.spr.x, entity.spr.y, TILE_SIZE, TILE_SIZE, color2);
}

//Draws a pair of portals
void DrawPortal(Portals portals, Color color1, Color color2){
    DrawRectangle(portals.entrance.x, portals.entrance.y, TILE_SIZE, TILE_SIZE, color1);
    DrawRectangle(portals.exit.x, portals.exit.y, TILE_SIZE, TILE_SIZE, color2);
}


//Append element to the end of array
void* AppendElement(void* array, int byte_size, int length, void* new_element){
    void* new_array = realloc(array, (length + 1) * byte_size);

    if (new_array == NULL) Error("Error: Unable to allocate memory\n");

    memcpy((char*)new_array + (length * byte_size), new_element, byte_size);
    return new_array;
}

//Clamp function to restrict a value within a specified range
float Clamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}