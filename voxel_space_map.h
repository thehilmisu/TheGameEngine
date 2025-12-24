#ifndef VOXELSPACEMAP
#define VOXELSPACEMAP

#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define FPS 60
#define MAP_N 1024
#define SCALE_FACTOR 100.0
#define NUM_MAPS 29


typedef struct {
    float x;
    float y;
    float height;
    float angle;
    float horizon;
    float speed;
    float rotspeed;
    float heightspeed;
    float horizonspeed;
    float tiltspeed;
    float tilt;
    float zfar;
} camera_t;

typedef struct {
    char colorMap[50];
    char heightMap[50];
} map_t;

void ProcessInput(float timeDelta);

int GetLinearFogFactor(int fogEnd, int fogStart, int z);

float GetExponentialFogFactor(float fogDensity, int z);

Color GetScaledPixel(Color pixel, Color fog, float fogFactor);

void LoadMaps();

char* DropdownOptions();

void init_map();

void draw_map(); 

#endif
