#include "plants.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <raymath.h>

typedef struct {
    Matrix transform;
    Vector3 position;
    unsigned int depth;
} BranchProperties;

typedef struct {
    BranchProperties *items;
    int count;
    int capacity;
} BranchStack;

static void stack_push(BranchStack *stack, BranchProperties item) {
    if (stack->count >= stack->capacity) {
        int new_capacity = (stack->capacity == 0) ? 32 : stack->capacity * 2;
        BranchProperties *new_items = (BranchProperties *)MemAlloc(new_capacity * sizeof(BranchProperties));
        if (new_items) {
            if (stack->items) {
                memcpy(new_items, stack->items, stack->count * sizeof(BranchProperties));
                MemFree(stack->items);
            }
            stack->items = new_items;
            stack->capacity = new_capacity;
        } else {
            TraceLog(LOG_ERROR, "PLANTS: Stack push failed (out of memory)");
            return;
        }
    }
    stack->items[stack->count++] = item;
}

static BranchProperties stack_pop(BranchStack *stack) {
    if (stack->count > 0) return stack->items[--stack->count];
    TraceLog(LOG_WARNING, "PLANTS: Stack pop underflow");
    return (BranchProperties){ MatrixIdentity(), Vector3Zero(), 0 };
}

char* lsystem(uint32_t iterations, const char* axiom, const char* rule) {
    if (!axiom) return NULL;
    
    // Initial copy
    size_t axiom_len = strlen(axiom);
    char *result = (char *)MemAlloc(axiom_len + 1);
    if (!result) return NULL;
    memcpy(result, axiom, axiom_len + 1);

    for (uint32_t i = 0; i < iterations; i++) {
        size_t next_size = 0;
        for (int j = 0; result[j] != '\0'; j++) {
            if (result[j] == 'F') next_size += strlen(rule);
            else next_size += 1;
        }
        
        char *next = (char *)MemAlloc(next_size + 1);
        if (!next) {
            TraceLog(LOG_ERROR, "PLANTS: L-system allocation failed at iteration %d", i);
            return result; 
        }
        
        int pos = 0;
        size_t rule_len = strlen(rule);
        for (int j = 0; result[j] != '\0'; j++) {
            if (result[j] == 'F') {
                memcpy(next + pos, rule, rule_len);
                pos += (int)rule_len;
            } else {
                next[pos++] = result[j];
            }
        }
        next[pos] = '\0';
        MemFree(result);
        result = next;
    }
    return result;
}

// Helper to merge two meshes into one
Mesh MeshMerge(Mesh mesh1, Mesh mesh2) {
    // Check for NULL or empty meshes
    if (mesh1.vertexCount == 0 || !mesh1.vertices) return mesh2;
    if (mesh2.vertexCount == 0 || !mesh2.vertices) return mesh1;

    Mesh res = { 0 };
    res.vertexCount = mesh1.vertexCount + mesh2.vertexCount;
    res.triangleCount = mesh1.triangleCount + mesh2.triangleCount;

    // Use calloc for zero-initialization
    res.vertices = (float *)calloc(res.vertexCount * 3, sizeof(float));
    res.indices = (unsigned short *)calloc(res.triangleCount * 3, sizeof(unsigned short));
    
    if (mesh1.normals || mesh2.normals) {
        res.normals = (float *)calloc(res.vertexCount * 3, sizeof(float));
    }
    if (mesh1.texcoords || mesh2.texcoords) {
        res.texcoords = (float *)calloc(res.vertexCount * 2, sizeof(float));
    }

    if (!res.vertices || !res.indices) {
        TraceLog(LOG_ERROR, "PLANTS: MeshMerge failed to allocate memory");
        if (res.vertices) free(res.vertices);
        if (res.normals) free(res.normals);
        if (res.texcoords) free(res.texcoords);
        if (res.indices) free(res.indices);
        return mesh1;
    }

    // Copy mesh1 data with NULL checks
    if (mesh1.vertices) memcpy(res.vertices, mesh1.vertices, mesh1.vertexCount * 3 * sizeof(float));
    if (res.normals && mesh1.normals) {
        memcpy(res.normals, mesh1.normals, mesh1.vertexCount * 3 * sizeof(float));
    }
    if (res.texcoords && mesh1.texcoords) {
        memcpy(res.texcoords, mesh1.texcoords, mesh1.vertexCount * 2 * sizeof(float));
    }
    if (mesh1.indices) memcpy(res.indices, mesh1.indices, mesh1.triangleCount * 3 * sizeof(unsigned short));

    // Copy mesh2 data with offset and NULL checks
    int offset = mesh1.vertexCount;
    if (mesh2.vertices) {
        memcpy(res.vertices + offset * 3, mesh2.vertices, mesh2.vertexCount * 3 * sizeof(float));
    }
    if (res.normals && mesh2.normals) {
        memcpy(res.normals + offset * 3, mesh2.normals, mesh2.vertexCount * 3 * sizeof(float));
    }
    if (res.texcoords && mesh2.texcoords) {
        memcpy(res.texcoords + offset * 2, mesh2.texcoords, mesh2.vertexCount * 2 * sizeof(float));
    }
    
    // Copy indices with vertex offset
    if (mesh2.indices) {
        for (int i = 0; i < mesh2.triangleCount * 3; i++) {
            res.indices[mesh1.triangleCount * 3 + i] = mesh2.indices[i] + (unsigned short)offset;
        }
    }

    return res;
}

// Helper to transform a mesh
void MeshTransform(Mesh *mesh, Matrix transform) {
    if (!mesh->vertices) return;
    for (int i = 0; i < mesh->vertexCount; i++) {
        Vector3 v = { mesh->vertices[i * 3 + 0], mesh->vertices[i * 3 + 1], mesh->vertices[i * 3 + 2] };
        v = Vector3Transform(v, transform);
        mesh->vertices[i * 3 + 0] = v.x;
        mesh->vertices[i * 3 + 1] = v.y;
        mesh->vertices[i * 3 + 2] = v.z;

        if (mesh->normals) {
            Vector3 n = { mesh->normals[i * 3 + 0], mesh->normals[i * 3 + 1], mesh->normals[i * 3 + 2] };
            n = Vector3RotateByQuaternion(n, QuaternionFromMatrix(transform)); 
            mesh->normals[i * 3 + 0] = n.x;
            mesh->normals[i * 3 + 1] = n.y;
            mesh->normals[i * 3 + 2] = n.z;
        }
    }
}

// Helper to transform texcoords
void MeshTransformTexcoords(Mesh *mesh, Matrix transform) {
    if (!mesh->texcoords) return;
    for (int i = 0; i < mesh->vertexCount; i++) {
        Vector2 tc = { mesh->texcoords[i * 2 + 0], mesh->texcoords[i * 2 + 1] };
        Vector3 tc3 = { tc.x, tc.y, 0.0f };
        tc3 = Vector3Transform(tc3, transform);
        mesh->texcoords[i * 2 + 0] = tc3.x;
        mesh->texcoords[i * 2 + 1] = tc3.y;
    }
}

static Mesh CreateFrustumMesh(unsigned int detail, float radius1, float radius2, float length) {
    Mesh mesh = GenMeshCylinder(1.0f, length, detail);
    MeshTransform(&mesh, MatrixTranslate(0, length / 2.0f, 0));
    
    for (int i = 0; i < mesh.vertexCount; i++) {
        float y = mesh.vertices[i * 3 + 1];
        float t = y / length;
        float radius = radius1 * (1.0f - t) + radius2 * t;
        
        Vector2 v = { mesh.vertices[i * 3 + 0], mesh.vertices[i * 3 + 2] };
        float dist = sqrtf(v.x*v.x + v.y*v.y);
        if (dist > 0.0001f) {
            float scale = radius / dist;
            mesh.vertices[i * 3 + 0] *= scale;
            mesh.vertices[i * 3 + 2] *= scale;
        }
    }
    return mesh;
}

static Mesh createBranchSegment(const BranchProperties branch, float thickness, float decreaseAmt, float length, unsigned int detail) {
    float radius1 = fmaxf(thickness - decreaseAmt * (float)branch.depth, 0.01f);
    float radius2 = fmaxf(thickness - decreaseAmt * (float)(branch.depth + 1), 0.01f);
    
    Mesh segment = CreateFrustumMesh(detail, radius1, radius2, 1.0f);
    
    Matrix transformtc = MatrixMultiply(MatrixTranslate(0.01f, 0.0f, 0.0f), MatrixScale(0.48f, 2.0f, 1.0f));
    MeshTransformTexcoords(&segment, transformtc);
    
    Matrix mat = MatrixMultiply(MatrixScale(1.0f, length, 1.0f), branch.transform);
    mat = MatrixMultiply(mat, MatrixTranslate(branch.position.x, branch.position.y, branch.position.z));
    MeshTransform(&segment, mat);
    
    return segment;
}

static Mesh createBranchEnd(const BranchProperties branch, float thickness, float decreaseAmt, float length, unsigned int detail) {
    float radius = fmaxf(thickness - decreaseAmt * (float)branch.depth, 0.01f);
    Mesh endsegment = GenMeshCone(radius, length, detail);
    MeshTransform(&endsegment, MatrixTranslate(0, length / 2.0f, 0));
    
    Matrix transformtc = MatrixMultiply(MatrixTranslate(0.01f, 0.0f, 0.0f), MatrixScale(0.48f, 2.0f, 1.0f));
    MeshTransformTexcoords(&endsegment, transformtc);
    
    Matrix mat = MatrixMultiply(branch.transform, MatrixTranslate(branch.position.x, branch.position.y, branch.position.z));
    MeshTransform(&endsegment, mat);
    
    float leafScale = (branch.depth <= 2) ? 0.5f : 1.0f;
    int leafcount = (2 * (int)detail) / 3 - 1;
    if (leafcount < 1) leafcount = 1;
    
    Matrix leafTC = MatrixMultiply(MatrixTranslate(0.51f, 0.0f, 0.0f), MatrixScale(0.48f, 1.0f, 1.0f));
    
    for (int i = 0; i < leafcount; i++) {
        Mesh leaves = GenMeshPlane(1.0f, 1.0f, 1, 1);
        MeshTransformTexcoords(&leaves, leafTC);
        
        Vector3 forward = { 0, length, 0 };
        forward = Vector3Transform(forward, branch.transform);
        Vector3 offset = Vector3Scale(forward, (1.0f - leafScale));
        
        Matrix rotation = MatrixRotateY((120.0f * (float)i) * DEG2RAD);
        Matrix matLeaf = MatrixMultiply(MatrixTranslate(0, 1.0f, 0), MatrixScale(leafScale, leafScale, leafScale));
        matLeaf = MatrixMultiply(matLeaf, rotation);
        matLeaf = MatrixMultiply(matLeaf, branch.transform);
        matLeaf = MatrixMultiply(matLeaf, MatrixTranslate(branch.position.x + offset.x, branch.position.y + offset.y, branch.position.z + offset.z));
        
        MeshTransform(&leaves, matLeaf);
        endsegment = MeshMerge(endsegment, leaves);
    }
    
    return endsegment;
}

Model create_plant_from_str(const char* str, float angle, float length, float thickness, float decreaseAmt, unsigned int detail) {
    Mesh plantMesh = { 0 };
    BranchStack stack = { 0 };
    BranchProperties branch = { MatrixIdentity(), Vector3Zero(), 0 };
    
    size_t len = strlen(str);
    for (size_t i = 0; i < len; i++) {
        char ch = str[i];
        
        if (ch == '+') branch.transform = MatrixMultiply(MatrixRotateZ(angle), branch.transform);
        else if (ch == '-') branch.transform = MatrixMultiply(MatrixRotateZ(-angle), branch.transform);
        else if (ch == '&') branch.transform = MatrixMultiply(MatrixRotateY(angle), branch.transform);
        else if (ch == '<') branch.transform = MatrixMultiply(MatrixRotateX(angle), branch.transform);
        else if (ch == '>') branch.transform = MatrixMultiply(MatrixRotateX(-angle), branch.transform);
        else if (ch == 'F') {
            Mesh treepart;
            if (i == len - 1 || str[i+1] == ']') {
                treepart = createBranchEnd(branch, thickness, decreaseAmt, length, detail);
            } else {
                treepart = createBranchSegment(branch, thickness, decreaseAmt, length, detail);
            }
            
            plantMesh = MeshMerge(plantMesh, treepart);
            
            Vector3 forward = { 0, length, 0 };
            forward = Vector3Transform(forward, branch.transform);
            branch.position = Vector3Add(branch.position, forward);
            branch.depth++;
        }
        else if (ch == '[') stack_push(&stack, branch);
        else if (ch == ']') branch = stack_pop(&stack);
    }
    
    if (stack.items) MemFree(stack.items);
    return LoadModelFromMesh(plantMesh);
}

Model create_pine_tree_model(unsigned int detail) {
    // Start with the trunk
    Mesh trunk = GenMeshCone(0.3f, 5.0f, detail);
    if (!trunk.vertices) {
        TraceLog(LOG_ERROR, "PLANTS: Failed to generate trunk cone mesh");
        return (Model){ 0 };
    }
    MeshTransform(&trunk, MatrixTranslate(0, 2.5f, 0));
    if (trunk.texcoords) {
        MeshTransformTexcoords(&trunk, MatrixScale(0.5f, 8.0f, 1.0f));
    }
    
    // Add trunk bottom if detail is high enough
    if (detail > 4) {
        Mesh bottom = GenMeshCylinder(0.3f, 1.0f, detail);
        if (!bottom.vertices) {
            TraceLog(LOG_WARNING, "PLANTS: Failed to generate trunk bottom mesh, skipping");
        } else {
            MeshTransform(&bottom, MatrixTranslate(0, -0.5f, 0));
            if (bottom.texcoords) {
                Matrix transformtc = MatrixMultiply(MatrixTranslate(0.01f, 0.0f, 0.0f), MatrixScale(0.48f, 8.0f/5.0f, 1.0f));
                MeshTransformTexcoords(&bottom, transformtc);
            }
            trunk = MeshMerge(trunk, bottom);
        }
    }
    
    // Add foliage layers
    Vector3 top = { 0, 5.0f, 0 };
    Matrix transformtc = MatrixMultiply(MatrixTranslate(0.51f, 0.02f, 0.0f), MatrixScale(0.48f, 0.96f, 1.0f));
    
    for (int i = 0; i < 4; i++) {
        float scale = 0.75f + 0.25f * powf(1.5f, (float)i);
        float height = 1.6f + 0.2f * (float)i;
        Mesh part = GenMeshCone(scale, height, detail);
        
        if (!part.vertices) {
            TraceLog(LOG_WARNING, "PLANTS: Failed to generate foliage layer %d, skipping", i);
            continue;
        }
        
        Matrix mat = MatrixRotateY((PI/16.0f) * (float)i);
        mat = MatrixMultiply(mat, MatrixTranslate(top.x, top.y - height/2.0f, top.z));
        
        MeshTransform(&part, mat);
        if (part.texcoords) {
            MeshTransformTexcoords(&part, transformtc);
        }
        trunk = MeshMerge(trunk, part);
        top.y -= scale * 0.6f;
    }
    
    // Upload and create model
    return LoadModelFromMesh(trunk);
}

Model create_tree_model(unsigned int detail) {
    TraceLog(LOG_INFO, "PLANTS: Creating L-system tree with detail %d", detail);
    
    // Scale up 20x and make much thicker to be visible
    const float ANGLE = 30.0f * DEG2RAD;
    const float LENGTH = 12.0f;  // Even longer segments
    const float THICKNESS = 3.0f;  // Much thicker trunk/branches
    const float DECREASE = 0.3f;  // Slower taper
    const uint32_t ITERATIONS = 2;
    const char *RULE = "F[&&>F]F[--F][&&&&-F][&&&&&&&&-F]";
    
    char *str = lsystem(ITERATIONS, "F", RULE);
    if (!str) {
        TraceLog(LOG_ERROR, "PLANTS: L-system generation failed");
        return (Model){ 0 };
    }
    TraceLog(LOG_INFO, "PLANTS: L-system string length: %d", strlen(str));

    Mesh plantMesh = { 0 };
    BranchStack stack = { 0 };
    BranchProperties branch = { MatrixIdentity(), Vector3Zero(), 0 };
    
    int segmentCount = 0;
    int endCount = 0;
    
    size_t len = strlen(str);
    for (size_t i = 0; i < len; i++) {
        char ch = str[i];
        if (ch == '+') branch.transform = MatrixMultiply(MatrixRotateZ(ANGLE), branch.transform);
        else if (ch == '-') branch.transform = MatrixMultiply(MatrixRotateZ(-ANGLE), branch.transform);
        else if (ch == '&') branch.transform = MatrixMultiply(MatrixRotateY(ANGLE), branch.transform);
        else if (ch == '<') branch.transform = MatrixMultiply(MatrixRotateX(ANGLE), branch.transform);
        else if (ch == '>') branch.transform = MatrixMultiply(MatrixRotateX(-ANGLE), branch.transform);
        else if (ch == 'F') {
            Mesh treepart;
            if (i == len - 1 || str[i+1] == ']') {
                treepart = createBranchEnd(branch, THICKNESS, DECREASE, LENGTH, detail);
                endCount++;
            } else {
                treepart = createBranchSegment(branch, THICKNESS, DECREASE, LENGTH, detail);
                segmentCount++;
            }
            
            plantMesh = MeshMerge(plantMesh, treepart);
            
            Vector3 forward = { 0, LENGTH, 0 };
            forward = Vector3Transform(forward, branch.transform);
            branch.position = Vector3Add(branch.position, forward);
            branch.depth++;
        }
        else if (ch == '[') stack_push(&stack, branch);
        else if (ch == ']') branch = stack_pop(&stack);
    }
    
    TraceLog(LOG_INFO, "PLANTS: Created %d segments and %d ends", segmentCount, endCount);
    MemFree(str);
    if (stack.items) MemFree(stack.items);
    
    if (detail > 4) {
        Mesh bottom = GenMeshCylinder(THICKNESS, 15.0f, detail);  // Thick trunk base
        MeshTransform(&bottom, MatrixTranslate(0, -7.5f, 0));  // Position it
        Matrix transformtc = MatrixMultiply(MatrixTranslate(0.01f, 0.0f, 0.0f), MatrixScale(0.48f, 8.0f/5.0f, 1.0f));
        MeshTransformTexcoords(&bottom, transformtc);
        
        plantMesh = MeshMerge(plantMesh, bottom);
        TraceLog(LOG_INFO, "PLANTS: Added trunk base cylinder");
    }
    
    TraceLog(LOG_INFO, "PLANTS: Tree mesh created - vertices: %d, triangles: %d", 
             plantMesh.vertexCount, plantMesh.triangleCount);
    
    // CRITICAL: Upload the merged mesh to GPU before creating the model
    if (!plantMesh.vertices || !plantMesh.indices) {
        TraceLog(LOG_ERROR, "PLANTS: Merged mesh has NULL data!");
        return (Model){0};
    }
    
    UploadMesh(&plantMesh, false);
    TraceLog(LOG_INFO, "PLANTS: Mesh uploaded to GPU - vaoId: %d", plantMesh.vaoId);
    
    Model model = LoadModelFromMesh(plantMesh);
    TraceLog(LOG_INFO, "PLANTS: Tree model created - meshCount: %d, materialCount: %d", 
             model.meshCount, model.materialCount);
    return model;
}
