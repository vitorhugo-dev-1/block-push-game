// Struct definition
typedef struct Block {int x, y;} Block;
typedef struct Entity {Block box, spr;} Entity;
//typedef struct Portals {Block entry, exit;} Portals;
typedef struct Item {Block position; bool active;} Item;

typedef struct Array2D {char **array; int rows, cols;} Array2D;
typedef struct BlockArr {Block *instance; int size;} BlockArr;
typedef struct EntityArr {Entity *instance; int size;} EntityArr;
typedef struct ItemArr {Item *instance; int size;} ItemArr;


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
int IsInsideMap(Block block, Array2D map){
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

// Function to create a 2D array
Array2D Create2DArray(int rows, int cols) {
    Array2D arr = {
        .array = (char **)malloc(rows * sizeof(char *)),
        .rows = rows,
        .cols = cols
    };

    if (arr.array == NULL) {
        Error("Memory allocation failed for array\n");
    }

    for (int i = 0; i <= rows; i++) {
        arr.array[i] = (char *)malloc(cols * sizeof(char));

        if (arr.array[i] == NULL) {
            for (int j = 0; j < i; j++) {
                free(arr.array[j]);
            }

            free(arr.array);

            fprintf(stderr, "Memory allocation failed for array row %d\n", i);
            exit(EXIT_FAILURE);
        }
    }

    return arr;
}

// Function to load the map from a file
void LoadMap(const char *fileName, Array2D *map) {
    map->rows = map->cols = 0;
    char ch;

    FILE *file_ptr = fopen(fileName, "r");

    if (file_ptr == NULL) {
        Error("Unable to open file\n");
    }

    // First pass to get the dimensions
    int currentCols = 0;
    while ((ch = fgetc(file_ptr)) != EOF) {
        if (ch == ',') {
            continue;
        } else if (ch == '\n') {
            map->rows++;
            if (currentCols > map->cols) {
                map->cols = currentCols;
            }
            currentCols = 0;
        } else {
            currentCols++;
        }
    }

    // Allocate memory for the map
    *map = Create2DArray(map->rows, map->cols);

    if (map == NULL || map->array == NULL) {
        fclose(file_ptr);
        Error("Memory allocation failed for the map\n");
    }

    map->rows = map->cols = 0;
    rewind(file_ptr);

    // Second pass to fill the map
    while ((ch = fgetc(file_ptr)) != EOF) {
        if (ch == ','){
            continue;
        } else if (ch == '\n') {
            map->rows++;
            map->cols = 0;
        } else {
            map->array[map->rows][map->cols] = ch;
            map->cols++;
        }
    }

    fclose(file_ptr);
}

// Function to free the memory of a 2D array
void Free2DArray(Array2D *arr) {
    for (int i = 0; i < arr->rows; i++) {
        free(arr->array[i]);
    }
}

// Function to load an image and return a texture
Texture2D LoadTextureFromFile(const char *fileName) {
    Image image = LoadImage(fileName);
    ImageResizeNN(&image, TILE_SIZE, TILE_SIZE);
    Texture2D texture = LoadTextureFromImage(image);  // Convert image to texture
    UnloadImage(image);  // Unload image from CPU memory (image data is now in GPU memory)
    return texture;
}

//Draws an object's box and its sprite
void DrawEntity(Entity entity, Color color1, Color color2){
    DrawRectangle(entity.box.x, entity.box.y, TILE_SIZE, TILE_SIZE, color1);
    DrawRectangle(entity.spr.x, entity.spr.y, TILE_SIZE, TILE_SIZE, color2);
}

//Append element to the end of array
void* AppendElement(void* array, int byte_size, int length, void* new_element){
    void* new_array = realloc(array, (length + 1) * byte_size);

    if (new_array == NULL) {
        Error("Error: Unable to allocate memory\n");
    }

    memcpy((char*)new_array + (length * byte_size), new_element, byte_size);
    return new_array;
}