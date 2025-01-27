#pragma once

#include "stanford_dragon_data.h"

struct StanfordDragonMesh
{
    StanfordDragonMesh();

    std::vector<glm::vec3> positions{};
    std::vector<glm::vec3> normals{};
    std::vector<glm::vec2> uvs{};
    std::vector<glm::ivec3> triangles{};
};
