// Struct definition
typedef struct {
    char **array;
    int rows;
    int cols;
} Array2D;

int IsHigherOrLower(int num1, int num2){
    if (num1 > num2){
        return 1;
    } else if (num1 < num2){
        return -1;
    } else {
        return 0;
    }
}

//Checks if any of the keys passed are pressed
int IsAnyOfKeysPressed(int *keys) {
    for (int i = 0; keys[i] != -1; i++) {
        if (IsKeyDown(keys[i])) return keys[i];
    }
    return 0;
}

// Function to create a 2D array
Array2D *create2DArray(int rows, int cols) {
    Array2D *arr = (Array2D *)malloc(sizeof(Array2D));
    if (arr == NULL) {
        fprintf(stderr, "Memory allocation failed for Array2D\n");
        exit(EXIT_FAILURE);
    }

    arr->rows = rows;
    arr->cols = cols;
    arr->array = (char **)malloc(rows * sizeof(char *));
    if (arr->array == NULL) {
        fprintf(stderr, "Memory allocation failed for array rows\n");
        free(arr);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < rows; i++) {
        arr->array[i] = (char *)malloc(cols * sizeof(char));
        if (arr->array[i] == NULL) {
            fprintf(stderr, "Memory allocation failed for array row %d\n", i);
            for (int j = 0; j < i; j++) {
                free(arr->array[j]);
            }
            free(arr->array);
            free(arr);
            exit(EXIT_FAILURE);
        }
    }
    return arr;
}

// Function to free a 2D array
void free2DArray(Array2D *arr) {
    for (int i = 0; i < arr->rows; i++) {
        free(arr->array[i]);
    }
    free(arr->array);
    free(arr);
}

// Function to load the map from a file
void LoadMap(const char *fileName, int *rows, int *cols, Array2D **map) {
    *cols = *rows = 0;
    char ch;

    FILE *file_ptr = fopen(fileName, "r");
    if (file_ptr == NULL) {
        printf("Unable to open file %s\n", fileName);
        return;
    }

    // First pass to get dimensions
    int currentCols = 0;
    while ((ch = fgetc(file_ptr)) != EOF) {
        if (ch == '\n') {
            (*rows)++;
            if (currentCols > *cols) {
                *cols = currentCols;
            }
            currentCols = 0;
        } else {
            currentCols++;
        }
    }

    if (currentCols > 0) {  // Last line without a newline character
        (*rows)++;
        if (currentCols > *cols) {
            *cols = currentCols;
        }
    }

    // Allocate memory for the map
    *map = create2DArray(*rows, *cols);
    if (*map == NULL || (*map)->array == NULL) {
        fprintf(stderr, "Memory allocation failed for the map\n");
        fclose(file_ptr);
        return;
    }

    *cols = *rows = 0;
    rewind(file_ptr);

    // Second pass to fill the map
    while ((ch = fgetc(file_ptr)) != EOF) {
        if (ch == '\n') {
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
    ImageResizeNN(&image, tileSize, tileSize);
    Texture2D texture = LoadTextureFromImage(image);  // Convert image to texture
    UnloadImage(image);  // Unload image from CPU memory (image data is now in GPU memory)
    return texture;
}

void DrawEntity(Entity entity, Color color1, Color color2){
    DrawRectangle(entity.box.x, entity.box.y, tileSize, tileSize, color1);
    DrawRectangle(entity.spr.x, entity.spr.y, tileSize, tileSize, color2);
}