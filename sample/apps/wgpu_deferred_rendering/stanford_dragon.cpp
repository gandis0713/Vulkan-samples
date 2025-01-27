#include "stanford_dragon.h"
#include "utils.h"

StanfordDragonMesh::StanfordDragonMesh()
{
    GenerateNormalsResult result = generateNormals(
        glm::pi<float>(),
        getPositions(), getCells());

    positions = std::move(result.positions);
    normals = std::move(result.normals);
    triangles = std::move(result.triangles);

    uvs = computeProjectedPlaneUVs(positions, ProjectedPlane::XY);

    // Push indices for an additional ground plane
    triangles.insert(triangles.end(),
                     { { positions.size(), positions.size() + 2, positions.size() + 1 },
                       { positions.size(), positions.size() + 1, positions.size() + 3 } });

    // Push vertex attributes for an additional ground plane
    // prettier-ignore
    positions.insert(positions.end(),
                     { { -100, 0, -100 },
                       { 100, 0, 100 },
                       { -100, 0, 100 },
                       { 100, 0, -100 } });
    normals.insert(normals.end(),
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