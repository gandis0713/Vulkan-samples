#include "stanfordDragon.h"
#include "utils.h"

StanfordDragonMesh::StanfordDragonMesh()
{
    GenerateNormalsResult result = generateNormals(
        glm::pi<float>(),
        positions, cells);

    std::vector<glm::vec2> uvs = computeProjectedPlaneUVs(positions, ProjectedPlane::XY);

    // Push indices for an additional ground plane
    result.triangles.insert(result.triangles.end(),
                            { { positions.size(), positions.size() + 2, positions.size() + 1 },
                              { positions.size(), positions.size() + 1, positions.size() + 3 } });

    // Push vertex attributes for an additional ground plane
    // prettier-ignore
    result.positions.insert(result.positions.end(),
                            { { -100, 0, -100 },
                              { 100, 0, 100 },
                              { -100, 0, 100 },
                              { 100, 0, -100 } });
    result.normals.insert(result.normals.end(),
                          { { 0, 1, 0 },
                            { 0, 1, 0 },
                            { 0, 1, 0 },
                            { 0, 1, 0 } });
    uvs.insert(uvs.end(),
               { { 0, 0 },
                 { 1, 1 },
                 { 0, 1 },
                 { 1, 0 } });
}