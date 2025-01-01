// Struct definition
typedef struct Block {int x, y;} Block;
typedef struct Entity {Block box, spr;} Entity;
typedef struct EntityArr {Entity *instance; int size;} EntityArr;
typedef struct Portals {Block entry, exit;} Portals;

//Delete line below later
typedef struct Key {Block box; bool collected;} Key;

typedef struct Item {Block position; bool active;} Item;
typedef struct Array2D {char **array; int rows, cols;} Array2D;

enum Dictionary {
    EMPTY,
    PLAYER,
    WALL,
    CRATE,
    PORTAL,
    DOOR,
    KEY
};

enum Direction {
    NOWHERE,
    UP,
    DOWN,
    RIGHT,
    LEFT
};

void Error(const char *message){
    printf("Error: %s\n", message);
    exit(1);
}

int IsHigherOrLower(int num1, int num2){
    if (num1 > num2){
        return 1;
    } else if (num1 < num2){
        return -1;
    } else {
        return 0;
    }
}

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

//Checks if any of the keys passed are pressed
int IsAnyOfKeysPressed(int *keys) {
    for (int i = 0; keys[i] != -1; i++) {
        if (IsKeyDown(keys[i])) return keys[i];
    }
    return 0;
}

// Function to create a 2D array
Array2D *Create2DArray(int rows, int cols) {
    Array2D *arr = (Array2D *)malloc(sizeof(Array2D));

    if (arr == NULL) {
        Error("Memory allocation failed for Array2D\n");
    }

    arr->rows = rows;
    arr->cols = cols;
    arr->array = (char **)malloc(rows * sizeof(char *));

    if (arr->array == NULL) {
        free(arr);
        Error("Memory allocation failed for array rows\n");
    }

    for (int i = 0; i < rows; i++) {
        arr->array[i] = (char *)malloc(cols * sizeof(char));
        
        if (arr->array[i] == NULL) {
            for (int j = 0; j < i; j++) {
                free(arr->array[j]);
            }

            free(arr->array);
            free(arr);

            fprintf(stderr, "Memory allocation failed for array row %d\n", i);
            exit(EXIT_FAILURE);
        }
    }
    return arr;
}

Entity* Append(Entity* array, int size, Entity new_element) {
    Entity* new_array = (Entity*)malloc((size + 1) * sizeof(Entity));

    for (int i = 0; i < size; i++) {
        new_array[i] = array[i];
    }

    new_array[size] = new_element;
    return new_array;
}

// Function to free a 2D array
void Free2DArray(Array2D *arr) {
    for (int i = 0; i < arr->rows; i++) {
        free(arr->array[i]);
    }
    free(arr->array);
    free(arr);
}

// Function to load the map from a file
void LoadMap(const char *fileName, int *rows, int *cols, Array2D **map) {
    *cols = 0;
    *rows = 1;
    char ch;

    FILE *file_ptr = fopen(fileName, "r");
    if (file_ptr == NULL) {
        Error("Unable to open file\n");
    }

    // First pass to get dimensions
    int currentCols = 0;
    while ((ch = fgetc(file_ptr)) != EOF) {
        if (ch == ',') {
            continue;
        } else if (ch == '\n') {
            (*rows)++;
            if (currentCols > *cols) {
                *cols = currentCols;
            }
            currentCols = 0;
        } else {
            currentCols++;
        }
    }

    // Allocate memory for the map
    *map = Create2DArray(*rows, *cols);
    if (*map == NULL || (*map)->array == NULL) {
        fclose(file_ptr);
        Error("Memory allocation failed for the map\n");
    }

    *cols = *rows = 0;
    rewind(file_ptr);

    // Second pass to fill the map
    while ((ch = fgetc(file_ptr)) != EOF) {
        if (ch == ','){
            continue;
        } else if (ch == '\n') {
            (*rows)++;
            *cols = 0;
        } else {
            (*map)->array[*rows][*cols] = ch;
            (*cols)++;
        }
    }
    fclose(file_ptr);
}

// Function to load an image and return a texture
Texture2D LoadTextureFromFile(const char *fileName) {
    Image image = LoadImage(fileName);
    ImageResizeNN(&image, TILE_SIZE, TILE_SIZE);
    Texture2D texture = LoadTextureFromImage(image);  // Convert image to texture
    UnloadImage(image);  // Unload image from CPU memory (image data is now in GPU memory)
    return texture;
}

void DrawEntity(Entity entity, Color color1, Color color2){
    DrawRectangle(entity.box.x, entity.box.y, TILE_SIZE, TILE_SIZE, color1);
    DrawRectangle(entity.spr.x, entity.spr.y, TILE_SIZE, TILE_SIZE, color2);
}