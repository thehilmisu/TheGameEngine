#ifndef ENGINE_H
#define ENGINE_H

#include <stdint.h>
#include <stdbool.h>
#include <raylib.h>
#include "nob.h"

// --- ECS CORE ---

#define ENTITY_INVALID 0

typedef struct 
{
    Vector3 position;
    Vector3 rotation; // Euler angles in degrees
    Vector3 scale;
} TransformComponent;

typedef enum 
{
    MESH_CUBE,
    MESH_SPHERE,
    MESH_PLANE
} MeshType;

typedef struct {
    MeshType type;
    Color color;
} MeshComponent;

typedef struct
{
    uint32_t id;
    TransformComponent transform;
    MeshComponent mesh;
}Entity;

typedef struct {
    bool is_selected;
    bool is_hovered;
} EditorComponent;

// Component Arrays (SoA - Structure of Arrays style for ECS-Lite)
typedef struct {
    Entity *entities;
    EditorComponent *editors;
    size_t count;
    size_t capacity;
} Registry;

// --- Registry API ---

Registry registry_init(void);
Entity registry_create_entity(Registry *reg);
void registry_free(Registry *reg);

// --- Component Helpers (using bitsets or indices, simplified here) ---
// For this lite version, every entity will have an entry in all component arrays 
// but we'll use a "presence" check if needed, or just assume they are there for now.

// --- Systems ---

typedef enum 
{
    GIZMO_NONE,
    GIZMO_X,
    GIZMO_Y,
    GIZMO_Z
} GizmoAxis;

typedef struct 
{
    GizmoAxis active_axis;
    float drag_start_t;
    Vector3 drag_entity_start_pos;
    Entity selected_entity;
} EditorState;

void system_render(Registry *reg, Camera3D camera);
void system_editor_update(Registry *reg, EditorState *editor, Camera3D camera);
void system_editor_render(Registry *reg, EditorState *editor, Camera3D camera);

#endif // ENGINE_H

