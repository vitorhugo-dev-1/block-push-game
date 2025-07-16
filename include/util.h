#pragma once

#include <core_defs.h>
#include <raylib.h>
#include <types.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Prints an error and terminates the program
static inline void Error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
    exit(EXIT_FAILURE);
}

// Returns the angle in degrees (0-359) that two Vector2 coordinates point to
static inline float CalculateAngle(Vector2 origin, Vector2 end){
    // Calculate the differences in the x and y coordinates
    float deltaX = end.x - origin.x;
    float deltaY = end.y - origin.y;

    // Calculate the angle in radians and convert it to degrees
    float angle = (atan2f(deltaY, deltaX) * (180.0f / M_PI)) + 180;

    return angle;
}

// Compares two floats: returns 1 if num1 > num2, -1 if num1 < num2, 0 if equal
static inline float CompareF(float num1, float num2){
    return (num1 > num2) - (num1 < num2);
}

// Determines the direction along a specified axis
static inline int GetDirectionDelta(Direction dir, bool isHorizontal){
    if (isHorizontal) return (dir == DIR_RIGHT) - (dir == DIR_LEFT);
    else return (dir == DIR_DOWN) - (dir == DIR_UP);
}

// Returns the center position of an Entity
static inline Entity GetEntityCenter(Entity position){
    return (Entity){
        { position.box.x + HALF_SIZE, position.box.y + HALF_SIZE },
        { position.spr.x + HALF_SIZE, position.spr.y + HALF_SIZE }
    };
}

// Converts a value to the nearest tile index
static inline int SnapToTile(int value){
    int remainder = (int)(value - HALF_SIZE) % TILE_SIZE;
    int position  = (int)(value - HALF_SIZE + remainder) / TILE_SIZE;

    return position;
}

// Snaps angle to the nearest multiple of 90 degrees
static inline float SnapTo90Degrees(int angle, int angleModifier){
    return (float)((angle + angleModifier) / 90 * 90 % 360);
}

// Converts an angle to a Direction enum, with optional modifier
static inline Direction AngleToDirection(float angle, float modifier){
    return (angle / 90.0f) + modifier;
}

// Get tile type from character code
static inline TileType GetTileType(char ch){
    switch (ch){
        case 'C': return TILE_CONVEYOR;
        case 'P': return TILE_PORTAL;
        default:  return TILE_EMPTY;
    }
}

// Initializes a Window with title and dimensions, and sets the target FPS
static void InitWindowAndSetFPS(Window *window, int targetFPS){
    InitWindow(window->width, window->height, window->title);
    SetTargetFPS(targetFPS);
}