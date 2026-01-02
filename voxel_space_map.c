#include "voxel_space_map.h"
#include "camera.h"
#include <math.h>

Color *colorMap = NULL;
Color *heightMap = NULL;
Color *screenBuffer = NULL;
Texture2D screenTexture = { 0 };
Image colorMapImage = {0};
Image heightMapImage = {0}; // LoadImage(maps[selectedMap].heightMap);

float voxel_horizon = 100.0f;
float voxel_tilt = 0.0f;
float voxel_zfar = 600.0f;

// Pre-calculated tables for optimization
static float fogTable[1024];
static float invZTable[1024];
static float currentFogDensity = -1.0f;

map_t maps[NUM_MAPS];

int fogType = 0;
float fogDensity = 0.0025;
float fogStart = 300.0;
float fogEnd = 600.0;
int selectedMap = 0;

int GetLinearFogFactor(int fogEnd, int fogStart, int z) 
{
    // module for creating fog factor using linear fog
    // https://learn.microsoft.com/en-us/windows/win32/direct3d9/fog-formulas
    return (int)((fogEnd - z) / (fogEnd - fogStart));
}

float GetExponentialFogFactor(float fogDensity, int z) 
{
    // module for creating fog factor applying exponential density
    // https://learn.microsoft.com/en-us/windows/win32/direct3d9/fog-formulas
    return (1 / exp(z * fogDensity));
}

Color GetScaledPixel(Color pixel, Color fog, float fogFactor) 
{
    // Scaling a given fixel with the fog color using our fog factor
    // p = original pixel
    // s = scaled pixel
    // f = fog pixel
    // ff = fog factor
    // s = p*ff + (1-f)*ff
    pixel.r = pixel.r * fogFactor;
    pixel.g = pixel.g * fogFactor;
    pixel.b = pixel.b * fogFactor;
    pixel.a = pixel.a * fogFactor;
    fog.r = fog.r * (1 - fogFactor);
    fog.g = fog.g * (1 - fogFactor);
    fog.b = fog.b * (1 - fogFactor);
    fog.a = fog.a * (1 - fogFactor);
    pixel.r = pixel.r + fog.r;
    pixel.g = pixel.g + fog.g;
    pixel.b = pixel.b + fog.b;
    pixel.a = pixel.a + fog.a;

    return pixel;
}

void LoadMaps() 
{
    for (size_t i = 0; i < NUM_MAPS; i++) {
        map_t map;
        sprintf(map.colorMap, "resources/map%d.color.gif", (int)i);
        sprintf(map.heightMap, "resources/map%d.height.gif", (int)i);
        maps[i] = map;
    }
}

void change_map(int map_index)
{
    selectedMap = map_index;
    init_map();
}

int get_current_map()
{
    return selectedMap;
}

void init_map()
{
    LoadMaps();

    colorMapImage = LoadImage(maps[selectedMap].colorMap);
    heightMapImage = LoadImage(maps[selectedMap].heightMap);

    if (colorMapImage.data == NULL || heightMapImage.data == NULL) {
        TraceLog(LOG_ERROR, "VOXEL: Failed to load map files. Check if 'resources' directory exists in working directory.");
    } else {
        colorMap = LoadImageColors(colorMapImage);
        heightMap = LoadImageColors(heightMapImage);
    }

    screenBuffer = (Color *)malloc(RENDER_WIDTH * RENDER_HEIGHT * sizeof(Color));
    if (screenBuffer) {
        for (int i = 0; i < RENDER_WIDTH * RENDER_HEIGHT; i++) screenBuffer[i] = BLACK;
    }
    
    // Create an image that references the screenBuffer
    // We will use this to initialize the texture
    Image screenImage = {
        .data = screenBuffer,
        .width = RENDER_WIDTH,
        .height = RENDER_HEIGHT,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
        .mipmaps = 1
    };

    screenTexture = LoadTextureFromImage(screenImage);
}

void render_map() 
{
    // Sync with engine camera
    Camera3D *engineCamera = get_camera();

    float camX = engineCamera->position.x;
    float camY = engineCamera->position.z; 
    float camHeight = engineCamera->position.y;
    float camAngle = atan2f(engineCamera->target.z - engineCamera->position.z, engineCamera->target.x - engineCamera->position.x);

    // Calculate fractional movement to fix Z-judder
    // We assume the forward direction is roughly aligned with the camera target
    float dirX = engineCamera->target.x - engineCamera->position.x;
    float dirZ = engineCamera->target.z - engineCamera->position.z;
    float dirLen = sqrtf(dirX*dirX + dirZ*dirZ);
    if (dirLen > 0) {
        dirX /= dirLen;
        dirZ /= dirLen;
    }

    // depthOffset is how much we have moved "into" the current map grid unit
    // along the look direction.
    float depthOffset = (camX * dirX + camY * dirZ);
    depthOffset -= floorf(depthOffset);

   
    if (colorMap) UnloadImageColors(colorMap);
    if (heightMap) UnloadImageColors(heightMap);
    
    colorMap = LoadImageColors(colorMapImage);
    heightMap = LoadImageColors(heightMapImage);
    
    // Pre-calculate tables if needed
    if (currentFogDensity != fogDensity) {
        for (int z = 0; z < 1024; z++) {
            fogTable[z] = 1.0f / expf(z * fogDensity);
            invZTable[z] = 1.0f / (float)(z > 0 ? z : 1);
        }
        currentFogDensity = fogDensity;
    }

    // Clear backbuffer
    for (int i = 0; i < RENDER_WIDTH * RENDER_HEIGHT; i++) {
        screenBuffer[i] = (Color){ 0, 0, 0, 0 }; // Transparent clear
    }

    float sinangle = sin(camAngle);
    float cosangle = cos(camAngle);

    float plx = cosangle * voxel_zfar + sinangle * voxel_zfar;
    float ply = sinangle * voxel_zfar - cosangle * voxel_zfar;

    float prx = cosangle * voxel_zfar - sinangle * voxel_zfar;
    float pry = sinangle * voxel_zfar + cosangle * voxel_zfar;

    float inv_zfar = 1.0f / voxel_zfar;
    float inv_render_width = 1.0f / (float)RENDER_WIDTH;
    int zfar_int = (int)voxel_zfar;
    if (zfar_int > 1024) zfar_int = 1024;

    // Use fractional Y (depth) to offset the starting sampling position
    // We offset the start to align exactly with the camera world position
    float startRX = camX;
    float startRY = camY;

    // Adjust start position by one half step to center sampling on the first slice
    // This further stabilizes the forward movement
    float initialStep = 1.0f - depthOffset;
    
    for (int i = 0; i < RENDER_WIDTH; i++) {
        float deltaX = (plx + (prx - plx) * i * inv_render_width) * inv_zfar;
        float deltaY = (ply + (pry - ply) * i * inv_render_width) * inv_zfar;

        float rx = startRX + deltaX * initialStep;
        float ry = startRY + deltaY * initialStep;

        float maxHeight = (float)RENDER_HEIGHT;
        float lean = (voxel_tilt * (i * inv_render_width - 0.5f) + 0.5f) * RENDER_HEIGHT / 6.0f;

        for (int z = 1; z < zfar_int; z++) {
            rx += deltaX;
            ry += deltaY;

            if (heightMap && colorMap) {
                // Bilinear interpolation for smooth height sampling
                float floorX = floorf(rx);
                float floorY = floorf(ry);
                float fx = rx - floorX;
                float fy = ry - floorY;
                
                int x0 = ((int)floorX) & (MAP_N - 1);
                int y0 = ((int)floorY) & (MAP_N - 1);
                int x1 = (x0 + 1) & (MAP_N - 1);
                int y1 = (y0 + 1) & (MAP_N - 1);

                float h00 = heightMap[y0 * MAP_N + x0].r;
                float h10 = heightMap[y0 * MAP_N + x1].r;
                float h01 = heightMap[y1 * MAP_N + x0].r;
                float h11 = heightMap[y1 * MAP_N + x1].r;

                // Bilinear blend
                float h = h00 * (1.0f - fx) * (1.0f - fy) + 
                          h10 * fx * (1.0f - fy) + 
                          h01 * (1.0f - fx) * fy + 
                          h11 * fx * fy;
                
                // Use a continuous distance for projection to eliminate Z-judder
                // We subtract the fractional progress into the current grid cell
                float continuousZ = (float)z - depthOffset;
                if (continuousZ < 0.1f) continuousZ = 0.1f;
                
                int projHeight = (int)((camHeight - h) / continuousZ * SCALE_FACTOR + voxel_horizon);
                if (projHeight < 0) projHeight = 0;
                if (projHeight >= RENDER_HEIGHT) projHeight = RENDER_HEIGHT - 1;

                if (projHeight < maxHeight) {
                    float fogFactor = fogTable[z];
                    // Still sample color from nearest to keep it fast
                    int mapoffset = (MAP_N * y0) + x0;
                    Color pixel = colorMap[mapoffset];
                    Color scaledPixel = GetScaledPixel(pixel, (Color){180, 180, 180, 255}, fogFactor);

                    if (fogType == 1) {
                        float fStart = fogStart;
                        float fEnd = fogEnd;
                        if (fEnd <= fStart) fEnd = fStart + 1.0f;
                        scaledPixel = GetScaledPixel(pixel, (Color){180, 180, 180, 100}, GetLinearFogFactor((int)fEnd, (int)fStart, z));
                    }

                    int startY = (int)(projHeight + lean);
                    int endY = (int)(maxHeight + lean);
                    
                    if (startY < 0) startY = 0;
                    if (endY > RENDER_HEIGHT) endY = RENDER_HEIGHT;

                    if (screenBuffer) {
                        for (int y = startY; y < endY; y++) {
                            screenBuffer[y * RENDER_WIDTH + i] = scaledPixel;
                        }
                    }
                    maxHeight = (float)projHeight;
                }
            }
        }
    }

    // Update texture and draw upscaled
    UpdateTexture(screenTexture, screenBuffer);
    DrawTexturePro(screenTexture, 
        (Rectangle){ 0, 0, RENDER_WIDTH, RENDER_HEIGHT },
        (Rectangle){ 0, 0, GetScreenWidth(), GetScreenHeight() },
        (Vector2){ 0, 0 }, 0.0f, WHITE);
}

void cleanup_map()
{
    if (colorMap) UnloadImageColors(colorMap);
    if (heightMap) UnloadImageColors(heightMap);
    if (screenBuffer) free(screenBuffer);
    UnloadTexture(screenTexture);
}

