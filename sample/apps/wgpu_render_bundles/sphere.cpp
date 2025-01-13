#include "sphere.h"

#include <cmath>
#include <cstdlib>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <vector>

// GLM 라이브러리 헤더
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

SphereMesh createSphereMesh(
    float radius,
    int widthSegments,
    int heightSegments,
    float randomness)
{
    widthSegments = std::max(3, widthSegments);
    heightSegments = std::max(2, heightSegments);

    std::random_device rd;
    std::mt19937 gen(rd());
    // -1.0 ~ 1.0 범위의 난수 분포
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

    SphereMesh mesh;
    auto& vertices = mesh.vertices;
    auto& indices = mesh.indices;

    // 임시 glm::vec3
    glm::vec3 firstVertex(0.0f);
    glm::vec3 vertex(0.0f);
    glm::vec3 normal(0.0f);

    // grid[iy][ix] => 생성된 버텍스 인덱스
    std::vector<std::vector<uint16_t>> grid(heightSegments + 1);
    uint16_t index = 0;

    for (int iy = 0; iy <= heightSegments; ++iy)
    {
        std::vector<uint16_t> verticesRow;
        verticesRow.reserve(widthSegments + 1);

        float v = static_cast<float>(iy) / static_cast<float>(heightSegments);

        // 위/아래 극점에서의 U 오프셋 (Three.js 방식)
        float uOffset = 0.0f;
        if (iy == 0)
        {
            uOffset = 0.5f / static_cast<float>(widthSegments);
        }
        else if (iy == heightSegments)
        {
            uOffset = -0.5f / static_cast<float>(widthSegments);
        }

        for (int ix = 0; ix <= widthSegments; ++ix)
        {
            float u = static_cast<float>(ix) / static_cast<float>(widthSegments);

            // 경도(둘레) 마지막 부분은 x=0(첫 정점)의 정보를 재활용
            if (ix == widthSegments)
            {
                vertex = firstVertex;
            }
            // 극점(iy=0 또는 iy=heightSegments)을 제외한 일반 영역
            else if (ix == 0 || (iy != 0 && iy != heightSegments))
            {
                float randomScale = 1.0f + (dist(gen) * 0.5f * randomness);
                float rr = radius * randomScale;

                vertex.x = -rr * std::cos(u * glm::pi<float>() * 2.0f) * std::sin(v * glm::pi<float>());
                vertex.y = rr * std::cos(v * glm::pi<float>());
                vertex.z = rr * std::sin(u * glm::pi<float>() * 2.0f) * std::sin(v * glm::pi<float>());

                // 처음 ix=0인 경우, 이후 ix==widthSegments에서 활용하도록 저장
                if (ix == 0)
                {
                    firstVertex = vertex;
                }
            }

            // 1) 위치
            vertices.push_back(vertex.x);
            vertices.push_back(vertex.y);
            vertices.push_back(vertex.z);

            // 2) 노멀 (정규화)
            normal = glm::normalize(vertex);
            vertices.push_back(normal.x);
            vertices.push_back(normal.y);
            vertices.push_back(normal.z);

            // 3) UV
            vertices.push_back(u + uOffset);
            vertices.push_back(1.0f - v);

            verticesRow.push_back(index++);
        }

        grid[iy] = verticesRow;
    }

    // 인덱스 버퍼 생성
    for (int iy = 0; iy < heightSegments; ++iy)
    {
        for (int ix = 0; ix < widthSegments; ++ix)
        {
            uint16_t a = grid[iy][ix + 1];
            uint16_t b = grid[iy][ix];
            uint16_t c = grid[iy + 1][ix];
            uint16_t d = grid[iy + 1][ix + 1];

            if (iy != 0)
            {
                indices.push_back(a);
                indices.push_back(b);
                indices.push_back(d);
            }
            if (iy != (heightSegments - 1))
            {
                indices.push_back(b);
                indices.push_back(c);
                indices.push_back(d);
            }
        }
    }

    return mesh;
}