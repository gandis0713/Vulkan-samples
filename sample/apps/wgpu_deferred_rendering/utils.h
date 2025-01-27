#include <algorithm>
#include <array>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <limits>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

// --------------------------------------------------------
// 1) 표면 노멀을 계산하는 함수 (computeSurfaceNormals)
//    JS 코드의 computeSurfaceNormals와 동일한 로직입니다.
// --------------------------------------------------------
std::vector<glm::vec3> computeSurfaceNormals(
    const std::vector<glm::vec3>& positions,
    const std::vector<glm::ivec3>& triangles);

// --------------------------------------------------------
// generateNormals 결과를 담기 위한 구조체
// (JS 코드에서는 {positions, normals, triangles}를 반환)
// --------------------------------------------------------
struct GenerateNormalsResult
{
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::ivec3> triangles;
};

// --------------------------------------------------------
// 2) generateNormals
//    - 각 면의 노멀(faceNormals)을 구하고
//    - 정점이 공유되는 면들 중 최대 각도 이하인 노멀만 누적
//    - 새롭게 재구성된 positions, normals, triangles를 반환
// --------------------------------------------------------
GenerateNormalsResult generateNormals(
    float maxAngle,
    const std::vector<glm::vec3>& positions,
    const std::vector<glm::ivec3>& triangles);

// --------------------------------------------------------
// 3) 투영 평면 UV를 계산하는 함수 (computeProjectedPlaneUVs)
// --------------------------------------------------------
enum class ProjectedPlane
{
    XY,
    XZ,
    YZ
};

// plane에 따라 (u, v)가 어떤 좌표축에 맵핑될지 결정
std::pair<int, int> planeToIdx(ProjectedPlane plane);

// positions를 특정 평면(XY, XZ, YZ)에 투영하여 [0,1] 범위로 매핑
std::vector<glm::vec2> computeProjectedPlaneUVs(
    const std::vector<glm::vec3>& positions,
    ProjectedPlane plane = ProjectedPlane::XY);