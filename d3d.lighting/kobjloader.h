#pragma once

#include <cstdint>

#pragma pack(push, 1)
struct VertexData
{
    float pos[3];
    float uv[2];
    float norm[3];
};
#pragma pack(pop)

struct KOBJBlob
{
    uint32_t numVertices;
    uint32_t numIndices;
    VertexData *vertexBuffer;
    uint16_t *indexBuffer;
};

// ASSUMPTION: Missing vertex data causes an assertion
// failure. Otherwise, if UVs and normals are missing, they are
// silently filled in with zeros.
//
// USAGE:
//
// KOBJBlob objb = load_obj("teapot.obj")
// Send objb.vertex_buffer to the GPU
// Send objb.index_buffer to the GPU.
// free_obj(objb);

KOBJBlob load_obj(const char *filename);
void free_obj(KOBJBlob obj_blob);
