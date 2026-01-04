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

#define RENDER_WIDTH 960
#define RENDER_HEIGHT 540

typedef struct {
    char colorMap[50];
    char heightMap[50];
} map_t;

int GetLinearFogFactor(int fogEnd, int fogStart, int z);

float GetExponentialFogFactor(float fogDensity, int z);

Color GetScaledPixel(Color pixel, Color fog, float fogFactor);

void LoadMaps();

char* DropdownOptions();

void init_map();

void render_map(); 

void cleanup_map();

int get_current_map();

void change_map(int map_index);

// Get terrain height at world position (returns height value 0-255)
float get_terrain_height(float world_x, float world_z);

// Get color at world position
Color get_terrain_color(float world_x, float world_z);

#endif
