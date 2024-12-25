#include <vector>

struct SphereMesh
{
    std::vector<float> vertices;
    std::vector<uint16_t> indices;
};

struct SphereLayout
{
    static constexpr size_t vertexStride = 8 * sizeof(float);
    static constexpr size_t positionsOffset = 0;
    static constexpr size_t normalOffset = 3 * sizeof(float);
    static constexpr size_t uvOffset = 6 * sizeof(float);
};

SphereMesh createSphereMesh(float radius, int widthSegments = 32, int heightSegments = 16, float randomness = 0.0f);