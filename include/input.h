#pragma once

#include <raylib.h>

//Checks if any of the passed keys are pressed
int IsAnyOfKeysPressed(int *keys){
    for (int i = 0; keys[i] != -1; i++){
        if (IsKeyDown(keys[i])) return keys[i];
    }
    return 0;
}