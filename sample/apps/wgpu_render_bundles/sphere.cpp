#include "sphere.h"

#include <cmath>
#include <cstdlib>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

SphereMesh createSphereMesh(float radius, int widthSegments, int heightSegments, float randomness)
{
    SphereMesh mesh;

    widthSegments = std::max(3, widthSegments);
    heightSegments = std::max(2, heightSegments);

    std::vector<std::vector<int>> grid;
    int index = 0;

    // Generate vertices, normals, and uvs
    for (int iy = 0; iy <= heightSegments; ++iy)
    {
        std::vector<int> verticesRow;
        float v = static_cast<float>(iy) / heightSegments;

        // Special case for poles
        float uOffset = 0.0f;
        if (iy == 0)
        {
            uOffset = 0.5f / widthSegments;
        }
        else if (iy == heightSegments)
        {
            uOffset = -0.5f / widthSegments;
        }

        for (int ix = 0; ix <= widthSegments; ++ix)
        {
            float u = static_cast<float>(ix) / widthSegments;

            glm::vec3 vertex;
            if (ix == 0 || ix == widthSegments || (iy != 0 && iy != heightSegments))
            {
                float rr = radius + (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.0f * randomness * radius;

                vertex.x = -rr * std::cos(u * glm::two_pi<float>()) * std::sin(v * glm::pi<float>());
                vertex.y = rr * std::cos(v * glm::pi<float>());
                vertex.z = rr * std::sin(u * glm::two_pi<float>()) * std::sin(v * glm::pi<float>());
            }

            // Position
            mesh.vertices.push_back(vertex.x);
            mesh.vertices.push_back(vertex.y);
            mesh.vertices.push_back(vertex.z);

            // Normal
            glm::vec3 normal = glm::normalize(vertex);
            mesh.vertices.push_back(normal.x);
            mesh.vertices.push_back(normal.y);
            mesh.vertices.push_back(normal.z);

            // UV
            mesh.vertices.push_back(u + uOffset);
            mesh.vertices.push_back(1.0f - v);

            verticesRow.push_back(index++);
        }
        grid.push_back(verticesRow);
    }

    // Generate indices
    for (int iy = 0; iy < heightSegments; ++iy)
    {
        for (int ix = 0; ix < widthSegments; ++ix)
        {
            int a = grid[iy][ix + 1];
            int b = grid[iy][ix];
            int c = grid[iy + 1][ix];
            int d = grid[iy + 1][ix + 1];

            if (iy != 0)
            {
                mesh.indices.push_back(a);
                mesh.indices.push_back(b);
                mesh.indices.push_back(d);
            }
            if (iy != heightSegments - 1)
            {
                mesh.indices.push_back(b);
                mesh.indices.push_back(c);
                mesh.indices.push_back(d);
            }
        }
    }

    return mesh;
}