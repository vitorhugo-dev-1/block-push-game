#pragma once

#include <stdio.h>
#include <math.h>

//Prints an error and terminates the program
void Error(const char *message){
    perror(message);
    exit(EXIT_FAILURE);
}

//Returns a angle that goes from 0-359 that two Vector2 coordinates point to
float CalculateAngle(Vector2 origin, Vector2 end){
    // Calculate the differences in the x and y coordinates
    float deltaX = end.x - origin.x;
    float deltaY = end.y - origin.y;

    // Calculate the angle in radians and convert it to degrees
    float angle = (atan2f(deltaY, deltaX) * (180.0f / 3.14159265358979323846)) + 180;

    return angle;
}

//Compares if the first value is higher, lower or equal to the second value
int IsHigherOrLower(int num1, int num2){
    if (num1 > num2){ return 1; }
    else if (num1 < num2){ return -1; }
    return 0;
}

//Determines the destination of a Direction variable along a specified axis
int DetectDestinationDir(Direction dir, bool isHorizontal){
    if (isHorizontal) return ((dir == RIGHT) - (dir == LEFT));
    else return ((dir == DOWN) - (dir == UP));
}

Vector2 GetTileCenter(Vector2 position){
    return (Vector2){ position.x + HALF_SIZE, position.y + HALF_SIZE };
}

int SnapToTile(int value){
    int remainder = (int)(value - HALF_SIZE) % TILE_SIZE;
    int position  = (int)(value - HALF_SIZE + remainder) / TILE_SIZE;

    return position;
}