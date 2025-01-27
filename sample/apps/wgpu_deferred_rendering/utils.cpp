#include "utils.h"

// --------------------------------------------------------
// 1) 표면 노멀을 계산하는 함수 (computeSurfaceNormals)
//    JS 코드의 computeSurfaceNormals와 동일한 로직입니다.
// --------------------------------------------------------
std::vector<glm::vec3> computeSurfaceNormals(
    const std::vector<glm::vec3>& positions,
    const std::vector<glm::ivec3>& triangles)
{
    std::vector<glm::vec3> normals(positions.size(), glm::vec3(0.0f));

    // 각 삼각형을 돌면서 노멀을 누적
    for (const auto& tri : triangles)
    {
        int i0 = tri.x;
        int i1 = tri.y;
        int i2 = tri.z;

        glm::vec3 p0 = positions[i0];
        glm::vec3 p1 = positions[i1];
        glm::vec3 p2 = positions[i2];

        // 두 변을 구해서 정규화
        glm::vec3 v0 = glm::normalize(p1 - p0);
        glm::vec3 v1 = glm::normalize(p2 - p0);

        // 외적을 사용해 노멀 구하기
        glm::vec3 norm = glm::cross(v0, v1);

        // 노멀 누적
        normals[i0] += norm;
        normals[i1] += norm;
        normals[i2] += norm;
    }

    // 누적한 노멀을 최종적으로 정규화
    for (auto& n : normals)
    {
        n = glm::normalize(n);
    }

    return normals;
}

// --------------------------------------------------------
// 2) generateNormals
//    - 각 면의 노멀(faceNormals)을 구하고
//    - 정점이 공유되는 면들 중 최대 각도 이하인 노멀만 누적
//    - 새롭게 재구성된 positions, normals, triangles를 반환
// --------------------------------------------------------
GenerateNormalsResult generateNormals(
    float maxAngle,
    const std::vector<glm::vec3>& positions,
    const std::vector<glm::ivec3>& triangles)
{
    // 삼각형(면)의 개수
    const int numFaces = static_cast<int>(triangles.size());

    // ----------------------------------------------------
    // 2-1) 각 면의 노멀을 먼저 구하기(faceNormals)
    // ----------------------------------------------------
    std::vector<glm::vec3> faceNormals(numFaces);
    for (int i = 0; i < numFaces; ++i)
    {
        const glm::ivec3& tri = triangles[i];
        glm::vec3 v1 = positions[tri.x];
        glm::vec3 v2 = positions[tri.y];
        glm::vec3 v3 = positions[tri.z];

        glm::vec3 edge1 = v2 - v1;
        glm::vec3 edge2 = v3 - v1;
        glm::vec3 fn = glm::cross(edge1, edge2);
        fn = glm::normalize(fn);
        faceNormals[i] = fn;
    }

    // ----------------------------------------------------
    // 2-2) 정점마다 공유되는 면(삼각형)들의 인덱스를 기록
    //      vertFaces[정점공유인덱스] = { ...해당되는 face 인덱스들... }
    // ----------------------------------------------------
    // positions와 완전히 동일한 좌표를 갖는 정점끼리는 같은 인덱스로 취급
    // JavaScript 코드에서의 vertIndices 역할
    // ----------------------------------------------------
    // 먼저 (x,y,z) 좌표를 문자열로 만들어서 중복 좌표를 식별
    auto vec3ToString = [](const glm::vec3& v) {
        std::ostringstream oss;
        oss << v.x << "," << v.y << "," << v.z;
        return oss.str();
    };

    std::unordered_map<std::string, int> sharedIndexMap; // 좌표->공유인덱스
    sharedIndexMap.reserve(positions.size());

    std::vector<int> vertIndices(positions.size());
    int nextSharedIndex = 0;
    for (size_t i = 0; i < positions.size(); ++i)
    {
        std::string key = vec3ToString(positions[i]);
        auto it = sharedIndexMap.find(key);
        if (it == sharedIndexMap.end())
        {
            sharedIndexMap[key] = nextSharedIndex;
            vertIndices[i] = nextSharedIndex;
            nextSharedIndex++;
        }
        else
        {
            vertIndices[i] = it->second;
        }
    }

    // 정점 공유 인덱스 -> 어떤 face(삼각형)들에 속해 있는지
    std::vector<std::vector<int>> vertFaces(nextSharedIndex);
    for (int faceNdx = 0; faceNdx < numFaces; ++faceNdx)
    {
        const glm::ivec3& tri = triangles[faceNdx];
        for (int j = 0; j < 3; ++j)
        {
            int originalVertIndex = tri[j];
            int sharedNdx = vertIndices[originalVertIndex];
            vertFaces[sharedNdx].push_back(faceNdx);
        }
    }

    // ----------------------------------------------------
    // 2-3) 각 면을 순회하면서 최대 각도 이하인 면들만 노멀을 더해 새 정점 구성
    //      - JS 코드에서 tempVerts( (pos,normal) -> 새인덱스 ) 로 관리
    // ----------------------------------------------------
    struct PosNorm
    {
        glm::vec3 position;
        glm::vec3 normal;
    };

    // (position, normal) 쌍을 문자열로 만들어 인덱스 관리
    auto posNormToString = [](const glm::vec3& p, const glm::vec3& n) {
        std::ostringstream oss;
        oss << p.x << "," << p.y << "," << p.z << ";"
            << n.x << "," << n.y << "," << n.z;
        return oss.str();
    };

    std::unordered_map<std::string, int> newVertMap;
    std::vector<glm::vec3> newPositions;
    std::vector<glm::vec3> newNormals;
    std::vector<glm::i16vec3> newTriangles;
    newPositions.reserve(positions.size());
    newNormals.reserve(positions.size());
    newVertMap.reserve(positions.size());

    float maxAngleCos = std::cos(maxAngle);
    int nextNewIndex = 0;

    // 각 면을 순회
    for (int faceNdx = 0; faceNdx < numFaces; ++faceNdx)
    {
        const glm::ivec3& tri = triangles[faceNdx];
        glm::ivec3 newTri(0);

        // 면의 노멀
        glm::vec3 thisFaceNormal = faceNormals[faceNdx];

        // 삼각형의 각 꼭짓점 처리
        for (int j = 0; j < 3; ++j)
        {
            int oldVertIndex = tri[j];
            int sharedNdx = vertIndices[oldVertIndex];
            const glm::vec3& pos = positions[oldVertIndex];

            // 공유된 모든 면 중, 이 면(thisFaceNormal)과의 각도가 maxAngle 이하인 것만 노멀을 누적
            glm::vec3 accNormal(0.0f);
            for (int fNdx : vertFaces[sharedNdx])
            {
                glm::vec3 otherFaceNormal = faceNormals[fNdx];
                float dotVal = glm::dot(thisFaceNormal, otherFaceNormal);
                if (dotVal > maxAngleCos)
                {
                    accNormal += otherFaceNormal;
                }
            }
            accNormal = glm::normalize(accNormal);

            // (pos, accNormal)을 문자열로 변환하여 중복 체크
            std::string key = posNormToString(pos, accNormal);
            auto it = newVertMap.find(key);
            if (it == newVertMap.end())
            {
                newVertMap[key] = nextNewIndex;
                newPositions.push_back(pos);
                newNormals.push_back(accNormal);
                newTri[j] = nextNewIndex;
                nextNewIndex++;
            }
            else
            {
                newTri[j] = it->second;
            }
        }
        newTriangles.push_back(newTri);
    }

    GenerateNormalsResult result;
    result.positions = std::move(newPositions);
    result.normals = std::move(newNormals);
    result.triangles = std::move(newTriangles);
    return result;
}

// plane에 따라 (u, v)가 어떤 좌표축에 맵핑될지 결정
std::pair<int, int> planeToIdx(ProjectedPlane plane)
{
    switch (plane)
    {
    case ProjectedPlane::XY:
        return { 0, 1 };
    case ProjectedPlane::XZ:
        return { 0, 2 };
    case ProjectedPlane::YZ:
        return { 1, 2 };
    }
    // 기본값
    return { 0, 1 };
}

// positions를 특정 평면(XY, XZ, YZ)에 투영하여 [0,1] 범위로 매핑
std::vector<glm::vec2> computeProjectedPlaneUVs(
    const std::vector<glm::vec3>& positions,
    ProjectedPlane plane)
{
    std::vector<glm::vec2> uvs(positions.size(), glm::vec2(0.0f));

    auto idx = planeToIdx(plane);
    float minU = std::numeric_limits<float>::infinity();
    float minV = std::numeric_limits<float>::infinity();
    float maxU = -std::numeric_limits<float>::infinity();
    float maxV = -std::numeric_limits<float>::infinity();

    // 1) 우선 평면에 투영된 좌표(u, v)를 구하고 최소/최대값을 찾음
    for (size_t i = 0; i < positions.size(); ++i)
    {
        float u = positions[i][idx.first];
        float v = positions[i][idx.second];

        uvs[i] = glm::vec2(u, v);

        if (u < minU)
            minU = u;
        if (v < minV)
            minV = v;
        if (u > maxU)
            maxU = u;
        if (v > maxV)
            maxV = v;
    }

    // 2) 0~1 범위로 정규화
    float rangeU = maxU - minU;
    float rangeV = maxV - minV;
    for (auto& uv : uvs)
    {
        uv.x = (uv.x - minU) / rangeU;
        uv.y = (uv.y - minV) / rangeV;
    }

    return uvs;
}

// --------------------------------------------------------
// 예시 사용
// --------------------------------------------------------
/*
int main() {
    // (예시) positions와 triangles를 미리 구성했다고 가정
    std::vector<glm::vec3> positions = {
        {0.f, 0.f, 0.f},
        {1.f, 0.f, 0.f},
        {1.f, 1.f, 0.f},
        {0.f, 1.f, 0.f}
    };
    std::vector<glm::ivec3> triangles = {
        {0, 1, 2},
        {0, 2, 3}
    };

    // 표면 노멀 계산
    auto normals = computeSurfaceNormals(positions, triangles);

    // 특정 각도 기준으로 새로운 노멀 생성
    float maxAngleRadians = glm::radians(30.0f);
    GenerateNormalsResult genResult = generateNormals(maxAngleRadians, positions, triangles);

    // UV 계산
    auto uvCoords = computeProjectedPlaneUVs(positions, ProjectedPlane::XY);

    return 0;
}
*/
