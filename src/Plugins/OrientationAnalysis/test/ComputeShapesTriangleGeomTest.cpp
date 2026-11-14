#include "OrientationAnalysis/Filters/Algorithms/ComputeShapesTriangleGeom.hpp"
#include "OrientationAnalysis/Filters/ComputeShapesTriangleGeomFilter.hpp"
#include "OrientationAnalysis/utilities/OrientationUtilities.hpp"

#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <EbsdLib/Orientation/Euler.hpp>
#include <Eigen/Geometry>
#include <catch2/catch.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

using namespace nx::core;

namespace
{
using Frame = OrientationUtilities::Matrix3dR;
using Point = Eigen::Vector3d;
using Face = std::array<IGeometry::MeshIndexType, 3>;

struct ShapeMesh
{
  ShapeMesh() = default;
  ShapeMesh(std::vector<Point> meshVertices, std::vector<Face> meshFaces)
  : vertices(std::move(meshVertices))
  , faces(std::move(meshFaces))
  {
  }
  ~ShapeMesh() noexcept = default;

  ShapeMesh(const ShapeMesh&) = default;
  ShapeMesh(ShapeMesh&&) noexcept = default;
  ShapeMesh& operator=(const ShapeMesh&) = default;
  ShapeMesh& operator=(ShapeMesh&&) noexcept = default;

  std::vector<Point> vertices;
  std::vector<Face> faces;
};

ShapeMesh Frustum()
{
  ShapeMesh mesh;
  mesh.vertices = {{-3, -2, 0}, {3, -2, 0}, {3, 2, 0}, {-3, 2, 0}, {-6, -4, 1}, {6, -4, 1}, {6, 4, 1}, {-6, 4, 1}, {0.6, 0.3, 0}, {1.2, 0.6, 1}};
  for(usize i = 0; i < 4; i++)
  {
    const usize j = (i + 1) % 4;
    mesh.faces.push_back({8, j, i});
    mesh.faces.push_back({9, i + 4, j + 4});
    mesh.faces.push_back({i, j, j + 4});
    mesh.faces.push_back({i, j + 4, i + 4});
  }
  // At z=17/28, side-diagonal coordinates are +/-(6*17/28-2)=+/-23/14 in Y
  // and +/-(9*17/28-3)=+/-69/28 in X,
  // while axial hits have Y=0 or X=0. Thus each side hit is inside one triangle.
  // Cap fan centers are (0.6,0.3)*(1+z); neither has the corner slope +/-2/3.
  // Hence (0,0,z) is on no fan edge, and both cap hits are strictly interior.
  return mesh;
}

void AppendBox(ShapeMesh& mesh, const Point& minimum, const Point& maximum)
{
  const usize offset = mesh.vertices.size();
  mesh.vertices.insert(mesh.vertices.end(), {{minimum.x(), minimum.y(), minimum.z()},
                                             {maximum.x(), minimum.y(), minimum.z()},
                                             {maximum.x(), maximum.y(), minimum.z()},
                                             {minimum.x(), maximum.y(), minimum.z()},
                                             {minimum.x(), minimum.y(), maximum.z()},
                                             {maximum.x(), minimum.y(), maximum.z()},
                                             {maximum.x(), maximum.y(), maximum.z()},
                                             {minimum.x(), maximum.y(), maximum.z()}});
  const std::array<Face, 12> faces = {{{0, 2, 1}, {0, 3, 2}, {4, 5, 6}, {4, 6, 7}, {0, 1, 5}, {0, 5, 4}, {1, 2, 6}, {1, 6, 5}, {2, 3, 7}, {2, 7, 6}, {3, 0, 4}, {3, 4, 7}}};
  for(auto face : faces)
  {
    for(auto& index : face)
      index += offset;
    mesh.faces.push_back(face);
  }
}

DataStructure MeshData(const ShapeMesh& mesh, const Point& centroid, const Frame& rotation = Frame::Identity())
{
  DataStructure data;
  auto* geometry = TriangleGeom::Create(data, "Triangles");
  REQUIRE(geometry != nullptr);
  auto* vertices = UnitTest::CreateTestDataArray<float32>(data, "Vertices", {mesh.vertices.size()}, {3}, geometry->getId());
  auto* faces = UnitTest::CreateTestDataArray<IGeometry::MeshIndexType>(data, "Faces", {mesh.faces.size()}, {3}, geometry->getId());
  REQUIRE(vertices != nullptr);
  REQUIRE(faces != nullptr);
  geometry->setVertices(*vertices);
  geometry->setFaceList(*faces);
  for(usize i = 0; i < mesh.vertices.size(); i++)
  {
    const Point point = centroid + rotation * (mesh.vertices[i] - centroid);
    for(usize axis = 0; axis < 3; axis++)
      (*vertices)[3 * i + axis] = static_cast<float32>(point[axis]);
  }
  for(usize i = 0; i < mesh.faces.size(); i++)
    for(usize axis = 0; axis < 3; axis++)
      (*faces)[3 * i + axis] = mesh.faces[i][axis];
  auto* faceData = AttributeMatrix::Create(data, "Face Data", {mesh.faces.size()}, geometry->getId());
  auto* featureData = AttributeMatrix::Create(data, "Feature Data", {2}, geometry->getId());
  REQUIRE(faceData != nullptr);
  REQUIRE(featureData != nullptr);
  geometry->setFaceAttributeMatrix(*faceData);
  auto* labels = UnitTest::CreateTestDataArray<int32>(data, "Face Labels", {mesh.faces.size()}, {2}, faceData->getId());
  auto* centers = UnitTest::CreateTestDataArray<float32>(data, "Centroids", {2}, {3}, featureData->getId());
  REQUIRE(labels != nullptr);
  REQUIRE(centers != nullptr);
  centers->fill(0.0F);
  for(usize axis = 0; axis < 3; axis++)
    (*centers)[3 + axis] = static_cast<float32>(centroid[axis]);
  for(usize i = 0; i < mesh.faces.size(); i++)
  {
    (*labels)[2 * i] = -1;
    (*labels)[2 * i + 1] = 1;
  }
  return data;
}

void ExecuteShapes(DataStructure& data)
{
  ComputeShapesTriangleGeomFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeShapesTriangleGeomFilter::k_TriGeometryDataPath_Key, std::make_any<DataPath>(DataPath({"Triangles"})));
  args.insertOrAssign(ComputeShapesTriangleGeomFilter::k_FaceLabelsArrayPath_Key, std::make_any<DataPath>(DataPath({"Triangles", "Face Data", "Face Labels"})));
  args.insertOrAssign(ComputeShapesTriangleGeomFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Triangles", "Feature Data"})));
  args.insertOrAssign(ComputeShapesTriangleGeomFilter::k_CentroidsArrayPath_Key, std::make_any<DataPath>(DataPath({"Triangles", "Feature Data", "Centroids"})));
  const auto preflight = filter.preflight(data, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  const auto result = filter.execute(data, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  UnitTest::CheckArraysInheritTupleDims(data);
}

const Float32Array& Output(const DataStructure& data, const std::string& name)
{
  const DataPath path({"Triangles", "Feature Data", name});
  REQUIRE_NOTHROW(data.getDataRefAs<Float32Array>(path));
  return data.getDataRefAs<Float32Array>(path);
}

std::array<float32, 5> TetrahedronMeasurements(float64 degrees)
{
  // #1802 issue-body fixture: volume=2*3*4/6=4, centroid=(0.5,0.75,1).
  const ShapeMesh mesh = {{{0, 0, 0}, {2, 0, 0}, {0, 3, 0}, {0, 0, 4}}, {{0, 2, 1}, {0, 1, 3}, {0, 3, 2}, {1, 2, 3}}};
  const Frame rotation = Eigen::AngleAxisd(degrees * std::acos(-1.0) / 180.0, Point::UnitZ()).toRotationMatrix();
  auto data = MeshData(mesh, {0.5, 0.75, 1}, rotation);
  ExecuteShapes(data);
  const auto& lengths = Output(data, "AxisLengths");
  const auto& ratios = Output(data, "AspectRatios");
  return {lengths[3], lengths[4], lengths[5], ratios[2], ratios[3]};
}

void CheckOrderedAxes(const std::array<float64, 3>& euler, const Frame& expected)
{
  const auto matrix = ebsdlib::EulerDType(euler[0], euler[1], euler[2]).toOrientationMatrix();
  const Eigen::Map<const Frame> actual(matrix.data());
  CHECK(actual.determinant() == Approx(1.0).margin(1.0e-6));
  for(usize row = 0; row < 3; row++)
  {
    CAPTURE(row);
    CHECK(std::abs(actual.row(row).dot(expected.row(row))) == Approx(1.0).margin(1.0e-5));
  }
}

std::array<float64, 3> RayLengths(const DataStructure& data, const Frame& frame)
{
  REQUIRE_NOTHROW(data.getDataRefAs<TriangleGeom>(DataPath({"Triangles"})));
  const auto& geometry = data.getDataRefAs<TriangleGeom>(DataPath({"Triangles"}));
  std::vector<usize> faces(geometry.getNumberOfFaces());
  std::iota(faces.begin(), faces.end(), 0);
  const auto& centers = Output(data, "Centroids");
  const auto lengths = ComputeShapesTriangleGeomDetail::FindIntersections(frame, faces, geometry.getFacesRef().getDataStoreRef(), geometry.getVerticesRef().getDataStoreRef(),
                                                                          centers.getDataStoreRef(), 1, std::atomic_bool{false});
  return {lengths.xLength, lengths.yLength, lengths.zLength};
}
} // namespace

TEST_CASE("OrientationAnalysis::ComputeShapesTriangleGeom: Rigid rotation preserves lengths and ratios", "[OrientationAnalysis][ComputeShapesTriangleGeom]")
{
  const auto reference = TetrahedronMeasurements(0.0);
  for(const float64 angle : {30.0, 75.0, 135.0, 210.0})
  {
    const auto rotated = TetrahedronMeasurements(angle);
    for(usize i = 0; i < reference.size(); i++)
    {
      CAPTURE(angle, i);
      REQUIRE(std::isfinite(reference[i]));
      REQUIRE(reference[i] > 0.0F);
      REQUIRE(std::isfinite(rotated[i]));
      REQUIRE(rotated[i] > 0.0F);
      CHECK(rotated[i] == Approx(reference[i]).epsilon(1.0e-4).margin(1.0e-5));
    }
  }
}

TEST_CASE("OrientationAnalysis::ComputeShapesTriangleGeom: Analytic frustum half-spans preserve ordered axes", "[OrientationAnalysis][ComputeShapesTriangleGeom]")
{
  const usize rotationIndex = GENERATE(usize{0}, usize{1}, usize{2});
  CAPTURE(rotationIndex);
  const std::array<Frame, 3> rotations = {Frame::Identity(), Eigen::AngleAxisd(std::acos(-1.0) / 6, Point::UnitZ()).toRotationMatrix(),
                                          Eigen::AngleAxisd(0.8, Point{1, 2, 2}.normalized()).toRotationMatrix()};
  const auto& rotation = rotations[rotationIndex];
  auto data = MeshData(Frustum(), {0, 0, 17.0 / 28.0}, rotation);
  ExecuteShapes(data);
  const auto& lengths = Output(data, "AxisLengths");
  const auto& ratios = Output(data, "AspectRatios");
  const auto& eulers = Output(data, "AxisEulerAngles");
  // Integral A(z) dz=integral 24*(1+z)^2 dz=56; E[z]=17/28.
  // Covariance=diag(279/35,124/35,291/3920), so ascending inertia orders X,Y,Z.
  // At z=17/28: x=+/-135/28, y=+/-90/28; z rays hit 11/28 and 17/28.
  // Half-spans=(135/28,90/28,1/2); b/a=2/3 and c/a=14/135.
  CHECK(lengths[3] == Approx(135.0 / 28.0).margin(1.0e-5));
  CHECK(lengths[4] == Approx(90.0 / 28.0).margin(1.0e-5));
  CHECK(lengths[5] == Approx(0.5).margin(1.0e-5));
  CHECK(ratios[2] == Approx(2.0 / 3.0).margin(1.0e-5));
  CHECK(ratios[3] == Approx(14.0 / 135.0).margin(1.0e-5));
  if(rotationIndex == 0)
  {
    // Volume=56 and covariance=diag(279/35,124/35,291/3920).
    // Omega3=56^2/((279/35)*(124/35)*(291/3920))*9/(2000*pi^2)
    //       =1882384/(279651*pi^2)=0.6820121327990901.
    const auto& omega3Ref = Output(data, "Omega3s");
    CHECK(omega3Ref[1] == Approx(0.6820121327990901).margin(1.0e-5));
  }
  CheckOrderedAxes({eulers[3], eulers[4], eulers[5]}, rotation.transpose());
}

TEST_CASE("OrientationAnalysis::ComputeShapesTriangleGeom: Principal Euler frame preserves ordered axes", "[OrientationAnalysis][ComputeShapesTriangleGeom]")
{
  Frame frame;
  SECTION("Symmetric improper H frame")
  {
    // H=I-(2/3)*ones, H*H^T=I, determinant=-1.
    frame << 1, -2, -2, -2, 1, -2, -2, -2, 1;
    frame /= 3.0;
  }
  SECTION("Nonsymmetric proper frame")
  {
    // Rodrigues rotation about (1,2,2)/3, cos=3/5 and sin=4/5.
    frame << 29, -20, 28, 28, 35, -4, -20, 20, 35;
    frame /= 45.0;
  }
  SECTION("Nonsymmetric improper frame")
  {
    frame << 29, -20, 28, -28, -35, 4, -20, 20, 35;
    frame /= 45.0;
  }
  const auto euler = OrientationUtilities::PrincipalAxisFrameToEuler(frame);
  CheckOrderedAxes(euler, frame);
}

TEST_CASE("OrientationAnalysis::ComputeShapesTriangleGeom: Explicit axis signs preserve half-spans and ratios", "[OrientationAnalysis][ComputeShapesTriangleGeom]")
{
  auto data = MeshData(Frustum(), {0, 0, 17.0 / 28.0});
  const auto positive = RayLengths(data, Frame::Identity());
  const auto negative = RayLengths(data, -Frame::Identity());
  const std::array<float64, 3> expected = {135.0 / 28.0, 90.0 / 28.0, 0.5};
  for(usize axis = 0; axis < 3; axis++)
  {
    CAPTURE(axis);
    CHECK(positive[axis] == Approx(expected[axis]).margin(1.0e-6));
    CHECK(negative[axis] == Approx(expected[axis]).margin(1.0e-6));
    CHECK(positive[axis] == Approx(negative[axis]).margin(1.0e-6));
  }
  CHECK(positive[1] / positive[0] == Approx(2.0 / 3.0).margin(1.0e-6));
  CHECK(negative[1] / negative[0] == Approx(2.0 / 3.0).margin(1.0e-6));
  CHECK(positive[2] / positive[0] == Approx(14.0 / 135.0).margin(1.0e-6));
  CHECK(negative[2] / negative[0] == Approx(14.0 / 135.0).margin(1.0e-6));
  CHECK(std::abs(positive[2] - 11.0 / 28.0) > 0.1);
  CHECK(std::abs(positive[2] - 17.0 / 28.0) > 0.1);
}

TEST_CASE("OrientationAnalysis::ComputeShapesTriangleGeom: Missing hits and nonconvex gaps follow half-span policy", "[OrientationAnalysis][ComputeShapesTriangleGeom]")
{
  SECTION("One side has no hit from an external centroid")
  {
    auto data = MeshData(Frustum(), {0, 0, -1});
    const auto lengths = RayLengths(data, Frame::Identity());
    // +Z hits at t=1 and 2; -Z has none. Half-span=(2+0)/2=1.
    CHECK(lengths[2] == Approx(1.0));
    CHECK(RayLengths(data, -Frame::Identity())[2] == Approx(1.0));
  }
  SECTION("All axes miss the feature and produce the sentinel")
  {
    ShapeMesh mesh;
    for(const float64 x : {-10.0, 10.0})
      for(const float64 y : {-20.0, 20.0})
        for(const float64 z : {-30.0, 30.0})
          AppendBox(mesh, Point{x - 1, y - 2, z - 3}, Point{x + 1, y + 2, z + 3});
    // Eight disjoint lobes have centroid zero and diagonal covariance
    // (100+1/3,400+4/3,900+3). All three coordinate rays miss every lobe.
    auto data = MeshData(mesh, Point::Zero());
    const auto lengths = RayLengths(data, Frame::Identity());
    CHECK(lengths == std::array<float64, 3>{0, 0, 0});
    ExecuteShapes(data);
    const auto& output = Output(data, "AxisLengths");
    const auto& ratios = Output(data, "AspectRatios");
    for(usize i = 3; i < 6; i++)
      CHECK(output[i] == -1.0F);
    CHECK(ratios[2] == -1.0F);
    CHECK(ratios[3] == -1.0F);
  }
  SECTION("A missing axis invalidates every length and ratio")
  {
    ShapeMesh mesh;
    AppendBox(mesh, {-5, -2, -0.5}, {-3, 2, 0.5});
    AppendBox(mesh, {3, -2, -0.5}, {5, 2, 0.5});
    // The two lobes have centroid zero and diagonal covariance
    // (16+1/3,4/3,1/12). X has half-span 5; Y and Z have no hits.
    auto data = MeshData(mesh, Point::Zero());
    const auto lengths = RayLengths(data, Frame::Identity());
    CHECK(lengths == std::array<float64, 3>{5, 0, 0});
    ExecuteShapes(data);
    const auto& output = Output(data, "AxisLengths");
    const auto& ratios = Output(data, "AspectRatios");
    for(usize i = 3; i < 6; i++)
      CHECK(output[i] == -1.0F);
    CHECK(ratios[2] == -1.0F);
    CHECK(ratios[3] == -1.0F);
  }
  SECTION("Nonconvex multiple hits include the gaps")
  {
    ShapeMesh mesh;
    AppendBox(mesh, {-5, -2, -1}, {-3, 2, 1});
    AppendBox(mesh, {1, -2, -1}, {2, 2, 1});
    AppendBox(mesh, {4, -2, -1}, {8, 2, 1});
    auto data = MeshData(mesh, Point::Zero());
    // One feature with three disjoint lobes: X hits -5,-3,1,2,4,8.
    // Farthest distances are 5 and 8, half-span=13/2, including all gaps.
    CHECK(RayLengths(data, Frame::Identity())[0] == Approx(6.5));
    CHECK(RayLengths(data, -Frame::Identity())[0] == Approx(6.5));
  }
}

TEST_CASE("OrientationAnalysis::ComputeShapesTriangleGeom: Face Feature Centroids requires 3 components", "[OrientationAnalysis][ComputeShapesTriangleGeom][ComponentShape]")
{
  DataStructure dataStructure;
  auto* triangleGeom = TriangleGeom::Create(dataStructure, "Triangles");
  REQUIRE(triangleGeom != nullptr);
  auto* vertices = UnitTest::CreateTestDataArray<float32>(dataStructure, "Vertices", {3}, {3}, triangleGeom->getId());
  triangleGeom->setVertices(*vertices);
  triangleGeom->setVertexCoordinate(0, {0.0F, 0.0F, 0.0F});
  triangleGeom->setVertexCoordinate(1, {1.0F, 0.0F, 0.0F});
  triangleGeom->setVertexCoordinate(2, {0.0F, 1.0F, 0.0F});
  auto* faces = UnitTest::CreateTestDataArray<IGeometry::MeshIndexType>(dataStructure, "Faces", {1}, {3}, triangleGeom->getId());
  (*faces)[0] = 0;
  (*faces)[1] = 1;
  (*faces)[2] = 2;
  triangleGeom->setFaceList(*faces);
  auto* faceData = AttributeMatrix::Create(dataStructure, "Face Data", {1}, triangleGeom->getId());
  REQUIRE(faceData != nullptr);
  triangleGeom->setFaceAttributeMatrix(*faceData);
  auto* labels = UnitTest::CreateTestDataArray<int32>(dataStructure, "Face Labels", {1}, {2}, faceData->getId());
  (*labels)[0] = 1;
  (*labels)[1] = 0;
  auto* featureData = AttributeMatrix::Create(dataStructure, "Feature Data", {2}, triangleGeom->getId());
  REQUIRE(featureData != nullptr);
  UnitTest::CreateTestDataArray<float32>(dataStructure, "Centroids", {2}, {1}, featureData->getId());

  ComputeShapesTriangleGeomFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeShapesTriangleGeomFilter::k_TriGeometryDataPath_Key, std::make_any<DataPath>(DataPath({"Triangles"})));
  args.insertOrAssign(ComputeShapesTriangleGeomFilter::k_FaceLabelsArrayPath_Key, std::make_any<DataPath>(DataPath({"Triangles", "Face Data", "Face Labels"})));
  args.insertOrAssign(ComputeShapesTriangleGeomFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Triangles", "Feature Data"})));
  args.insertOrAssign(ComputeShapesTriangleGeomFilter::k_CentroidsArrayPath_Key, std::make_any<DataPath>(DataPath({"Triangles", "Feature Data", "Centroids"})));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(std::any_of(preflightResult.outputActions.errors().begin(), preflightResult.outputActions.errors().end(), [](const Error& error) { return error.code == -208; }));
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
