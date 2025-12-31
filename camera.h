#ifndef CAMERA_H
#define CAMERA_H

#include <raylib.h>

void init_camera(void);
void update_camera(void);
void set_camera_position(Vector3 pos);
void set_camera_target(Vector3 target);
void set_camera_up(Vector3 up);
void set_camera_fovy(float angle);
void set_camera_projection(void);
Camera3D *get_camera(void);

#endif // CAMERA_H
