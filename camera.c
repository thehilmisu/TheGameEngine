#include "camera.h"

static Camera3D camera = {0};

void init_camera(void)
{
    camera.position = (Vector3){ 10.0f, 10.0f, 10.0f };
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;
}

void update_camera(void)
{
    // Orbital camera for editor navigation
    UpdateCamera(&camera, CAMERA_ORBITAL);
}

void set_camera_position(Vector3 pos)
{
    camera.position = pos;
}

void set_camera_target(Vector3 target)
{
    camera.target = target;
}

void set_camera_up(Vector3 up)
{
    camera.up = up;
}

void set_camera_fovy(float angle)
{
    camera.fovy = angle;
}

void set_camera_projection(void)
{
    camera.projection = CAMERA_PERSPECTIVE;
}

Camera3D get_camera(void)
{
    return camera;
}
