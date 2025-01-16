#include "ComputeTriangleGeomShapes.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/GeometryHelpers.hpp"

#include "EbsdLib/Core/Orientation.hpp"
#include "EbsdLib/Core/OrientationTransformation.hpp"

#include <unordered_set>

using namespace nx::core;

namespace
{
using TriStore = AbstractDataStore<INodeGeometry2D::SharedFaceList::value_type>;
using VertsStore = AbstractDataStore<INodeGeometry0D::SharedVertexList::value_type>;

constexpr double k_Multiplier = 1.0 / (4.0 * Constants::k_PiD);
constexpr float64 k_ScaleFactor = 1.0;

usize FindEulerCharacteristic(usize numVertices, usize numFaces, usize numRegions)
{
  return numVertices + numFaces - (2 * numRegions);
}

template <typename T>
bool ValidateMesh(const AbstractDataStore<T>& faceStore, usize numVertices, usize numRegions)
{
  // Expensive call
  usize numEdges = GeometryHelpers::Connectivity::FindNumEdges(faceStore, numVertices);

  return numEdges == FindEulerCharacteristic(numVertices, faceStore.getNumberOfTuples(), numRegions);
}

struct AxialLengths
{
  IGeometry::SharedVertexList::value_type xLength = 0.0;
  IGeometry::SharedVertexList::value_type yLength = 0.0;
  IGeometry::SharedVertexList::value_type zLength = 0.0;
};

// These are values that are not associated with the direction of the ray
// Precalculating is only useful when calculating multiple rays (same origin/different directions) on the same triangle
template<typename T>
struct MTPointsCache
{
  using PointT = Eigen::Vector3<T>;

  // Ray origin
  PointT origin;

  // Triangle Vertices
  PointT pointA;
  PointT pointB;
  PointT pointC;

  // Specific edges
  // edge1 = pointB - pointA;
  PointT edge1;
  // edge2 = pointC - pointA;
  PointT edge2;

  // Pre-calculations
  // s = origin - pointA;
  PointT s;
  // sCrossE1 = s.cross(edge1);
  PointT sCrossE1;
};

// Eigen implementation of Moller-Trumbore intersection algorithm adapted to account for distance
template<typename T>
bool MTIntersection(const Eigen::Vector3<T>& dirVec, const MTPointsCache<T>& cache)
{
  using PointT = Eigen::Vector3<T>;
  constexpr T epsilon = std::numeric_limits<T>::epsilon();

  PointT crossE2 = dirVec.cross(cache.edge2);
  T det = cache.edge1.dot(crossE2);

  if(det > -epsilon && det < epsilon)
  {
    // Ray is parallel to given triangle
    return false;
  }

  T invDet = 1.0 / det;
  T u = invDet * cache.s.dot(crossE2);

  // Allow ADL for efficient absolute value function
  using std::abs;
  if((u < 0 && abs(u) > epsilon) || (u > 1 && abs(u - 1.0) > epsilon))
  {
    // Ray is parallel to given triangle
    return false;
  }

  T v = invDet * dirVec.dot(cache.sCrossE1);

  if((v < 0 && abs(v) > epsilon) || (u + v > 1 && abs(u + v - 1.0) > epsilon))
  {
    // Ray is parallel to given triangle
    return false;
  }

  T t = invDet * cache.edge2.dot(cache.sCrossE1);

  if(t > epsilon)
  {
    // Ray intersection
    return true;
  }

  // line intersection not ray intersection
  return false;
}

// Eigen implementation of Moller-Trumbore intersection algorithm adapted to account for distance
template <typename T = IGeometry::SharedVertexList::value_type>
AxialLengths FindIntersections(const Eigen::Matrix<T, 3, 3, Eigen::RowMajor>& orientationMatrix, const AbstractDataStore<int32>& faceLabelsStore,
                               const AbstractDataStore<IGeometry::MeshIndexType>& triStore, const AbstractDataStore<IGeometry::SharedVertexList::value_type>& vertexStore,
                               const AbstractDataStore<float32>& centroidsStore, IGeometry::MeshIndexType featureId, bool& valid, const std::atomic_bool& shouldCancel)
{
  constexpr T epsilon = std::numeric_limits<T>::epsilon();

  AxialLengths lengths;

  using PointT = Eigen::Vector3<T>;

  // derive the direction vector for each corresponding axis (using unit vectors)
  PointT xDirVec = PointT{1.0, 0.0, 0.0}.transpose() * orientationMatrix;
  PointT yDirVec = PointT{0.0, 1.0, 0.0}.transpose() * orientationMatrix;
  PointT zDirVec = PointT{0.0, 0.0, 1.0}.transpose() * orientationMatrix;

  MTPointsCache<T> cache;

  // Feature Centroid
  cache.origin = PointT{centroidsStore[3 * featureId], centroidsStore[3 * featureId + 1], centroidsStore[3 * featureId + 2]};

  for(usize i = 0; i < faceLabelsStore.getNumberOfTuples(); i++)
  {
    if(shouldCancel)
    {
      valid = false;
      return {};
    }

    if(faceLabelsStore[2 * i] != featureId && faceLabelsStore[2 * i + 1] != featureId)
    {
      // Triangle not in feature continue
      continue;
    }

    usize threeCompIndex = 3 * i;
    usize vertAIndex = triStore[threeCompIndex];
    cache.pointA = PointT{vertexStore[vertAIndex], vertexStore[vertAIndex + 1], vertexStore[vertAIndex + 2]};
    usize vertBIndex = triStore[threeCompIndex + 1];
    cache.pointB = PointT{vertexStore[vertBIndex], vertexStore[vertBIndex + 1], vertexStore[vertBIndex + 2]};
    usize vertCIndex = triStore[threeCompIndex + 2];
    cache.pointC = PointT{vertexStore[vertCIndex], vertexStore[vertCIndex + 1], vertexStore[vertCIndex + 2]};

    PointT triCentroid = (cache.pointA + cache.pointB + cache.pointC) / 3;

    cache.edge1 = cache.pointB - cache.pointA;
    cache.edge2 = cache.pointC - cache.pointA;

    cache.s = cache.origin - cache.pointA;
    cache.sCrossE1 = cache.s.cross(cache.edge1);

    if(MTIntersection(xDirVec, cache))
    {
      // Ray intersection
      using std::sqrt; // Allow ADL
      using std::abs;
      T distance = sqrt((triCentroid - cache.origin).array().square().sum());
      if(abs(distance) > abs(lengths.xLength))
      {
        lengths.xLength = distance;
      }
    }

    if(MTIntersection(yDirVec, cache))
    {
      // Ray intersection
      using std::sqrt; // Allow ADL
      using std::abs;
      T distance = sqrt((triCentroid - cache.origin).array().square().sum());
      if(abs(distance) > abs(lengths.yLength))
      {
        lengths.yLength = distance;
      }
    }

    if(MTIntersection(zDirVec, cache))
    {
      // Ray intersection
      using std::sqrt; // Allow ADL
      using std::abs;
      T distance = sqrt((triCentroid - cache.origin).array().square().sum());
      if(abs(distance) > abs(lengths.zLength))
      {
        lengths.zLength = distance;
      }
    }
  }

  // Check for zeroes (zeroes = probably invalid)
  valid = lengths.xLength && lengths.yLength && lengths.zLength;

  return lengths;
}

/**
 * @brief This will extract the 3 vertices from a given triangle face of a triangle geometry. This is MUCH faster
 * than calling the function in the Triangle Geometry because of the dynamic_cast<> that goes on in that function.
 */
inline std::array<nx::core::Point3Df, 3> GetFaceCoordinates(usize triangleId, const VertsStore& verts, const TriStore& triangleList)
{
  usize v0Idx = triangleList[triangleId * 3];
  usize v1Idx = triangleList[triangleId * 3 + 1];
  usize v2Idx = triangleList[triangleId * 3 + 2];
  return {Point3Df{verts[v0Idx * 3], verts[v0Idx * 3 + 1], verts[v0Idx * 3 + 2]}, Point3Df{verts[v1Idx * 3], verts[v1Idx * 3 + 1], verts[v1Idx * 3 + 2]},
          Point3Df{verts[v2Idx * 3], verts[v2Idx * 3 + 1], verts[v2Idx * 3 + 2]}};
}

/**
 * @brief Sorts the 3 values
 * @param a First Value
 * @param b Second Value
 * @param c Third Value
 * @return The indices in their sorted order
 */
template <typename T>
std::array<size_t, 3> TripletSort(T a, T b, T c, bool lowToHigh)
{
  constexpr size_t A = 0;
  constexpr size_t B = 1;
  constexpr size_t C = 2;
  std::array<size_t, 3> idx = {0, 1, 2};
  if(a > b && a > c)
  {
    // sorted[2] = a;
    if(b > c)
    {
      // sorted[1] = b;
      // sorted[0] = c;
      idx = {C, B, A};
    }
    else
    {
      // sorted[1] = c;
      // sorted[0] = b;
      idx = {B, C, A};
    }
  }
  else if(b > a && b > c)
  {
    // sorted[2] = b;
    if(a > c)
    {
      // sorted[1] = a;
      // sorted[0] = c;
      idx = {C, A, B};
    }
    else
    {
      // sorted[1] = c;
      // sorted[0] = a;
      idx = {A, C, B};
    }
  }
  else if(a > b)
  {
    // sorted[1] = a;
    // sorted[0] = b;
    // sorted[2] = c;
    idx = {B, A, C};
  }
  else if(a >= c && b >= c)
  {
    // sorted[0] = c;
    // sorted[1] = a;
    // sorted[2] = b;
    idx = {C, A, B};
  }
  else
  {
    // sorted[0] = a;
    // sorted[1] = b;
    // sorted[2] = c;
    idx = {A, B, C};
  }

  if(!lowToHigh)
  {
    std::swap(idx[0], idx[2]);
  }
  return idx;
}

} // namespace

// -----------------------------------------------------------------------------
ComputeTriangleGeomShapes::ComputeTriangleGeomShapes(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                     ComputeTriangleGeomShapesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeTriangleGeomShapes::~ComputeTriangleGeomShapes() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeTriangleGeomShapes::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ComputeTriangleGeomShapes::operator()()
{
  using MeshIndexType = IGeometry::MeshIndexType;
  const auto& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TriangleGeometryPath);
  const TriStore& triangleList = triangleGeom.getFacesRef().getDataStoreRef();
  const VertsStore& verts = triangleGeom.getVerticesRef().getDataStoreRef();

  const auto& faceLabels = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FaceLabelsArrayPath).getDataStoreRef();
  const auto& centroids = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CentroidsArrayPath).getDataStoreRef();

  // the assumption here is face labels contains information on region ids, that it is contiguous in the values, and that 0 is an invalid id
  // (ie the max function means that if the values in array are [1,2,4,5] it will assume there are 5 regions)
  usize numRegions = *std::max_element(faceLabels.begin(), faceLabels.end());

  if(!ValidateMesh(triangleList, verts.getNumberOfTuples(), numRegions))
  {
    return MakeErrorResult(-64720, fmt::format("The Euler Characteristic of the shape was found to be unequal to 2, this implies the shape may not be watertight or is malformed."));
  }

  // Calculated Arrays
  auto& omega3S = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->Omega3sArrayPath);
  auto& axisEulerAngles = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->AxisEulerAnglesArrayPath);
  auto& axisLengths = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->AxisLengthsArrayPath);
  auto& aspectRatios = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->AspectRatiosArrayPath);

  using Matrix3x3 = Eigen::Matrix<double, 3, 3, Eigen::RowMajor>;
  Matrix3x3 Cinertia;

  usize numFaces = faceLabels.getNumberOfTuples();
  usize numFeatures = centroids.getNumberOfTuples();

  nx::core::Point3Df centroid = {0.0F, 0.0F, 0.0F};

  // Theoretical perfect Sphere value of Omega-3. Each calculated Omega-3
  // will be normalized using this value;
  const float64 k_Sphere = (2000.0 * M_PI * M_PI) / 9.0;

  // define the canonical C matrix
  double aa = 1.0 / 60.0;
  double bb = aa / 2.0;
  // clang-format off
  Matrix3x3 C;
  C << aa, bb, bb, bb, aa, bb, bb, bb, aa;

  // and the identity matrix
  aa = 1.0;
  bb = 0.0;
  Matrix3x3 ID;
  ID << aa, bb, bb, bb, aa, bb, bb, bb, aa;

  // The C-Prime matrix
  Matrix3x3 CC;
  CC << -0.50000000, 0.50000000, 0.50000000,
        0.50000000, -0.50000000, 0.50000000,
        0.50000000, 0.50000000, -0.50000000;
  // clang-format on

  // Loop over each "Feature" which is the number of tuples in the "Centroids" array
  // We could parallelize over the features?
  for(usize featureId = 1; featureId < numFeatures; featureId++)
  {
    /**
     * The following section calculates moment of inertia tensor (Cinertia) and omega3s
     */
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      double Vol = 0.0;
      // define the accumulator arrays
      Matrix3x3 Cacc;
      Cacc << 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0;

      // Get the centroid for the feature
      centroid[0] = centroids[3 * featureId + 0];
      centroid[1] = centroids[3 * featureId + 1];
      centroid[2] = centroids[3 * featureId + 2];

      // for each triangle we need the transformation matrix A defined by the three points as columns
      // Loop over all triangle faces
      int32_t tCount = 0;
      for(usize i = 0; i < numFaces; i++)
      {
        if(faceLabels[2 * i] != featureId && faceLabels[2 * i + 1] != featureId)
        {
          continue;
        }
        tCount++;
        usize compIndex = (faceLabels[2 * i] == featureId ? 0 : 1);
        std::array<nx::core::Point3Df, 3> vertCoords = GetFaceCoordinates(i, verts, triangleList);

        const nx::core::Point3Df& a = vertCoords[0] - centroid;
        const nx::core::Point3Df& b = (compIndex == 0 ? vertCoords[1] : vertCoords[2]) - centroid;
        const nx::core::Point3Df& c = (compIndex == 0 ? vertCoords[2] : vertCoords[1]) - centroid;

        Matrix3x3 A;
        A << a[0], b[0], c[0], a[1], b[1], c[1], a[2], b[2], c[2];

        float64 dA = A.determinant();

        Cacc = (Cacc + dA * (A * (C * (A.transpose())))).eval();
        Vol += (dA / 6.0f); // accumulate the volumes
      }

      Cacc = (Cacc / Vol).eval();
      Cinertia = ID * Cacc.trace() - Cacc;
      // extract the moments from the inertia tensor
      Eigen::Vector3d e(Cinertia(0, 0), Cinertia(1, 1), Cinertia(2, 2));
      auto sols = CC * e;
      omega3S[featureId] = static_cast<float32>(((Vol * Vol) / sols.prod()) / k_Sphere);
    }

    /**
     * This next section finds the principle axis via eigenvalues.
     * Paper/Lecture Notes (Page 5): https://ocw.mit.edu/courses/16-07-dynamics-fall-2009/dd277ec654440f4c2b5b07d6c286c3fd_MIT16_07F09_Lec26.pdf
     * Video Walkthrough [0:00-10:45]: https://www.youtube.com/watch?v=IEDniK9kmaw
     *
     * The main goal is to derive the eigenvalues from the moment of inertia tensor therein finding the eigenvectors,
     * which are the angular velocity vectors.
     */
    Eigen::EigenSolver<Matrix3x3> eigenSolver(Cinertia);

    // The primary axis is the largest eigenvalue
    Eigen::EigenSolver<Matrix3x3>::EigenvalueType eigenvalues = eigenSolver.eigenvalues();

    // This is the angular velocity vector, each row represents an axial alignment (principle axis)
    Eigen::EigenSolver<Matrix3x3>::EigenvectorsType eigenvectors = eigenSolver.eigenvectors();

    /**
     * Following section for debugging
     */
    //    std::cout << "Eigenvalues:\n" << eigenvalues << std::endl;
    //    std::cout << "\n Eigenvectors:\n" << eigenvectors << std::endl;
    //
    //    constexpr char k_BaselineAxisLabel = 'x'; // x
    //    char axisLabel = 'x';
    //    double primaryAxis = eigenvalues[0].real();
    //    for(usize i = 1; i < eigenvalues.size(); i++)
    //    {
    //      if(primaryAxis < eigenvalues[i].real())
    //      {
    //        axisLabel = k_BaselineAxisLabel + static_cast<char>(i);
    //        primaryAxis = eigenvalues[i].real();
    //      }
    //    }
    //    std::cout << "\nPrimary Axis: " << axisLabel << " | Associated Eigenvalue: " << primaryAxis << std::endl;

    // Presort eigen ordering for following sections
    // Returns the argument order sorted high to low
    std::array<size_t, 3> idxs = ::TripletSort(eigenvalues[0].real(), eigenvalues[1].real(), eigenvalues[2].real(), false);

    Matrix3x3 orientationMatrix = {};

    /**
     * The following section calculates the axis eulers
     */
    {
      if(m_ShouldCancel)
      {
        return {};
      }

      // EigenVector associated with the largest EigenValue goes in the 3rd column
      auto col3 = eigenvectors.col(idxs[0]);

      // Then the next largest into the 2nd column
      auto col2 = eigenvectors.col(idxs[1]);

      // The smallest into the 1rst column
      auto col1 = eigenvectors.col(idxs[2]);

      // insert principal unit vectors into rotation matrix representing Feature reference frame within the sample reference frame
      //(Note that the 3 direction is actually the long axis and the 1 direction is actually the short axis)
      // clang-format off
      double g[3][3] = {{col1(0).real(), col1(1).real(), col1(2).real()},
                        {col2(0).real(), col2(1).real(), col2(2).real()},
                        {col3(0).real(), col3(1).real(), col3(2).real()}};
      // clang-format on

      orientationMatrix.col(0) = col1.real();
      orientationMatrix.col(1) = col2.real();
      orientationMatrix.col(2) = col3.real();

      // check for right-handedness
      OrientationTransformation::ResultType result = OrientationTransformation::om_check(OrientationD(g));
      if(result.result == 0)
      {
        g[2][0] *= -1.0f;
        g[2][1] *= -1.0f;
        g[2][2] *= -1.0f;
      }

      auto euler = OrientationTransformation::om2eu<OrientationD, OrientationD>(OrientationD(g));

      axisEulerAngles[3 * featureId] = euler[0];
      axisEulerAngles[3 * featureId + 1] = euler[1];
      axisEulerAngles[3 * featureId + 2] = euler[2];
    }

    /**
     * The following section finds axes
     */
    {
      if(m_ShouldCancel)
      {
        return {};
      }

      bool isValid = false;
      auto lengths = FindIntersections(orientationMatrix, faceLabels, triangleList, verts, centroids, featureId, isValid, m_ShouldCancel);

      if(!isValid)
      {
        return MakeErrorResult(-64721, fmt::format("{}({}): Error. The feature at id ({}) failed to have its lengths calculated.", __FILE__, __LINE__, featureId));
      }

      axisLengths[3 * featureId] = static_cast<float32>(lengths.xLength);
      axisLengths[3 * featureId + 1] = static_cast<float32>(lengths.yLength);
      axisLengths[3 * featureId + 2] = static_cast<float32>(lengths.zLength);
      auto bOverA = static_cast<float32>(lengths.yLength / lengths.xLength);
      auto cOverA = static_cast<float32>(lengths.zLength / lengths.xLength);
      aspectRatios[2 * featureId] = bOverA;
      aspectRatios[2 * featureId + 1] = cOverA;
    }
  }

  return {};
}
