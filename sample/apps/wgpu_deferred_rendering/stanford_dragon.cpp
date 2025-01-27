#include "stanford_dragon.h"
#include "utils.h"

StanfordDragonMesh::StanfordDragonMesh()
{
    GenerateNormalsResult result = generateNormals(
        glm::pi<float>(),
        getPositions(), getCells());

    std::vector<glm::vec2> uvs = computeProjectedPlaneUVs(result.positions, ProjectedPlane::XY);

    // Push indices for an additional ground plane
    result.triangles.insert(result.triangles.end(),
                            { { result.positions.size(), result.positions.size() + 2, result.positions.size() + 1 },
                              { result.positions.size(), result.positions.size() + 1, result.positions.size() + 3 } });

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