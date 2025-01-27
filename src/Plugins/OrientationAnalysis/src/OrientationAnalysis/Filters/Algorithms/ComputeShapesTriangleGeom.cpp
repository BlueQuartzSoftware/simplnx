#include "ComputeShapesTriangleGeom.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/GeometryHelpers.hpp"
#include "simplnx/Utilities/IntersectionUtilities.hpp"

#include "EbsdLib/Core/Orientation.hpp"
#include "EbsdLib/Core/OrientationTransformation.hpp"

#include <unordered_set>

using namespace nx::core;

namespace
{
using TriStore = AbstractDataStore<INodeGeometry2D::SharedFaceList::value_type>;
using VertsStore = AbstractDataStore<INodeGeometry0D::SharedVertexList::value_type>;

usize FindEulerCharacteristic(usize numVertices, usize numFaces, usize numRegions)
{
  return numVertices + numFaces - (2 * numRegions);
}

template <typename T>
bool ValidateMesh(const AbstractDataStore<T>& faceStore, usize numVertices, usize numRegions)
{
  // Expensive call
  const usize numEdges = GeometryHelpers::Connectivity::FindNumEdges(faceStore, numVertices);

  return numEdges == FindEulerCharacteristic(numVertices, faceStore.getNumberOfTuples(), numRegions);
}

struct AxialLengths
{
  IGeometry::SharedVertexList::value_type xLength = 0.0;
  IGeometry::SharedVertexList::value_type yLength = 0.0;
  IGeometry::SharedVertexList::value_type zLength = 0.0;
};

// Eigen implementation of Moller-Trumbore intersection algorithm adapted to account for distance
template <typename T = IGeometry::SharedVertexList::value_type>
AxialLengths FindIntersections(const Eigen::Matrix<T, 3, 3, Eigen::RowMajor>& orientationMatrix, const AbstractDataStore<int32>& faceLabelsStore,
                               const AbstractDataStore<IGeometry::MeshIndexType>& triStore, const AbstractDataStore<IGeometry::SharedVertexList::value_type>& vertexStore,
                               const AbstractDataStore<float32>& centroidsStore, IGeometry::MeshIndexType featureId, const std::atomic_bool& shouldCancel)
{
  constexpr T epsilon = std::numeric_limits<T>::epsilon();

  AxialLengths lengths;

  using PointT = Eigen::Vector3<T>;

  // derive the direction vector for each corresponding axis (using unit vectors)
  const PointT xDirVec = PointT{1.0, 0.0, 0.0}.transpose() * orientationMatrix;
  const PointT yDirVec = PointT{0.0, 1.0, 0.0}.transpose() * orientationMatrix;
  const PointT zDirVec = PointT{0.0, 0.0, 1.0}.transpose() * orientationMatrix;

  IntersectionUtilities::MTPointsCache<T> cache;

  // Feature Centroid
  cache.origin = PointT{centroidsStore[3 * featureId], centroidsStore[(3 * featureId) + 1], centroidsStore[(3 * featureId) + 2]};

  for(usize i = 0; i < faceLabelsStore.getNumberOfTuples(); i++)
  {
    if(shouldCancel)
    {
      return lengths;
    }

    if(faceLabelsStore[2 * i] != featureId && faceLabelsStore[(2 * i) + 1] != featureId)
    {
      // Triangle not in feature continue
      continue;
    }

    // Here we are manually extracting the vertex points from the SharedVertexList
    const usize threeCompIndex = 3 * i;

    // Extract tuple index of the first vertex and compute the index to the first x-value in the SharedVertexList
    const usize vertAIndex = triStore[threeCompIndex] * 3;
    cache.pointA = PointT{vertexStore[vertAIndex], vertexStore[vertAIndex + 1], vertexStore[vertAIndex + 2]};

    const usize vertBIndex = triStore[threeCompIndex + 1] * 3;
    cache.pointB = PointT{vertexStore[vertBIndex], vertexStore[vertBIndex + 1], vertexStore[vertBIndex + 2]};

    const usize vertCIndex = triStore[threeCompIndex + 2] * 3;
    cache.pointC = PointT{vertexStore[vertCIndex], vertexStore[vertCIndex + 1], vertexStore[vertCIndex + 2]};

    cache.edge1 = cache.pointB - cache.pointA;
    cache.edge2 = cache.pointC - cache.pointA;

    cache.sDist = cache.origin - cache.pointA;
    cache.sCrossE1 = cache.sDist.cross(cache.edge1);

    const std::optional<PointT> xIntersect = IntersectionUtilities::MTIntersection(xDirVec, cache);
    if(xIntersect.has_value())
    {
      // Ray intersection
      T distance = std::sqrt((xIntersect.value() - cache.origin).array().square().sum());
      if(abs(distance) > std::abs(lengths.xLength))
      {
        lengths.xLength = distance;
      }
    }

    const std::optional<PointT> yIntersect = IntersectionUtilities::MTIntersection(yDirVec, cache);
    if(yIntersect.has_value())
    {
      // Ray intersection
      T distance = std::sqrt((yIntersect.value() - cache.origin).array().square().sum());
      if(abs(distance) > std::abs(lengths.yLength))
      {
        lengths.yLength = distance;
      }
    }

    const std::optional<PointT> zIntersect = IntersectionUtilities::MTIntersection(zDirVec, cache);
    if(zIntersect.has_value())
    {
      // Ray intersection
      T distance = std::sqrt((zIntersect.value() - cache.origin).array().square().sum());
      if(abs(distance) > std::abs(lengths.zLength))
      {
        lengths.zLength = distance;
      }
    }
  }

  return lengths;
}

/**
 * @brief This will extract the 3 vertices from aVal given triangle face of aVal triangle geometry. This is MUCH faster
 * than calling the function in the Triangle Geometry because of the dynamic_cast<> that goes on in that function.
 */
inline std::array<nx::core::Point3Df, 3> GetFaceCoordinates(usize triangleId, const VertsStore& verts, const TriStore& triangleList)
{
  const usize v0Idx = triangleList[triangleId * 3];
  const usize v1Idx = triangleList[(triangleId * 3) + 1];
  const usize v2Idx = triangleList[(triangleId * 3) + 2];
  return {Point3Df{verts[v0Idx * 3], verts[(v0Idx * 3) + 1], verts[(v0Idx * 3) + 2]}, Point3Df{verts[v1Idx * 3], verts[(v1Idx * 3) + 1], verts[(v1Idx * 3) + 2]},
          Point3Df{verts[v2Idx * 3], verts[(v2Idx * 3) + 1], verts[(v2Idx * 3) + 2]}};
}

/**
 * @brief Sorts the 3 values
 * @param aVal First Value
 * @param bVal Second Value
 * @param cVal Third Value
 * @return The indices in their sorted order
 */
template <typename T>
std::array<size_t, 3> TripletSort(T aVal, T bVal, T cVal, bool lowToHigh)
{
  constexpr size_t k_AIdx = 0;
  constexpr size_t k_BIdx = 1;
  constexpr size_t k_CIdx = 2;
  std::array<size_t, 3> idx = {0, 1, 2};
  if(aVal > bVal && aVal > cVal)
  {
    // sorted[2] = aVal;
    if(bVal > cVal)
    {
      // sorted[1] = bVal;
      // sorted[0] = cVal;
      idx = {k_CIdx, k_BIdx, k_AIdx};
    }
    else
    {
      // sorted[1] = cVal;
      // sorted[0] = bVal;
      idx = {k_BIdx, k_CIdx, k_AIdx};
    }
  }
  else if(bVal > aVal && bVal > cVal)
  {
    // sorted[2] = bVal;
    if(aVal > cVal)
    {
      // sorted[1] = aVal;
      // sorted[0] = cVal;
      idx = {k_CIdx, k_AIdx, k_BIdx};
    }
    else
    {
      // sorted[1] = cVal;
      // sorted[0] = aVal;
      idx = {k_AIdx, k_CIdx, k_BIdx};
    }
  }
  else if(aVal > bVal)
  {
    // sorted[1] = aVal;
    // sorted[0] = bVal;
    // sorted[2] = cVal;
    idx = {k_BIdx, k_AIdx, k_CIdx};
  }
  else if(aVal >= cVal && bVal >= cVal)
  {
    // sorted[0] = cVal;
    // sorted[1] = aVal;
    // sorted[2] = bVal;
    idx = {k_CIdx, k_AIdx, k_BIdx};
  }
  else
  {
    // sorted[0] = aVal;
    // sorted[1] = bVal;
    // sorted[2] = cVal;
    idx = {k_AIdx, k_BIdx, k_CIdx};
  }

  if(!lowToHigh)
  {
    std::swap(idx[0], idx[2]);
  }
  return idx;
}

} // namespace

// -----------------------------------------------------------------------------
ComputeShapesTriangleGeom::ComputeShapesTriangleGeom(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                     ComputeShapesTriangleGeomInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeShapesTriangleGeom::~ComputeShapesTriangleGeom() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeShapesTriangleGeom::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ComputeShapesTriangleGeom::operator()()
{
  using MeshIndexType = IGeometry::MeshIndexType;
  const auto& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TriangleGeometryPath);
  const TriStore& triangleList = triangleGeom.getFacesRef().getDataStoreRef();
  const VertsStore& verts = triangleGeom.getVerticesRef().getDataStoreRef();

  const auto& faceLabels = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FaceLabelsArrayPath).getDataStoreRef();
  const auto& centroids = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CentroidsArrayPath).getDataStoreRef();

  // the assumption here is face labels contains information on region ids, that it is contiguous in the values, and that 0 is an invalid id
  // (ie the max function means that if the values in array are [1,2,4,5] it will assume there are 5 regions)
  const usize numRegions = *std::max_element(faceLabels.begin(), faceLabels.end());

  if(!ValidateMesh(triangleList, verts.getNumberOfTuples(), numRegions))
  {
    return MakeErrorResult(-64720, fmt::format("The Euler Characteristic of the shape was found to be unequal to 2, this implies the shape may not be watertight or is malformed."));
  }

  // Calculated Arrays
  auto& omega3S = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->Omega3sArrayPath);
  auto& axisEulerAngles = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->AxisEulerAnglesArrayPath);
  auto& axisLengths = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->AxisLengthsArrayPath);
  auto& aspectRatios = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->AspectRatiosArrayPath);

  using Matrix3x3 = Eigen::Matrix<float64, 3, 3, Eigen::RowMajor>;
  Matrix3x3 Cinertia;

  const usize numFaces = faceLabels.getNumberOfTuples();
  const usize numFeatures = centroids.getNumberOfTuples();

  nx::core::Point3Df centroid = {0.0F, 0.0F, 0.0F};

  // Theoretical perfect Sphere value of Omega-3. Each calculated Omega-3
  // will be normalized using this value;
  constexpr float64 k_Sphere = (2000.0 * M_PI * M_PI) / 9.0;

  // define the canonical cMatrix matrix
  constexpr float64 aVal = 1.0 / 60.0;
  constexpr float64 bVal = aVal / 2.0;
  // clang-format off
  Matrix3x3 cMatrix;
  cMatrix << aVal, bVal, bVal, bVal, aVal, bVal, bVal, bVal, aVal;

  // and the identity matrix
  Matrix3x3 identityMat;
  identityMat << 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0;

  // The cMatrix-Prime matrix
  Matrix3x3 cPrime;
  cPrime << -0.50000000, 0.50000000, 0.50000000,
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
      float64 Vol = 0.0;
      // define the accumulator arrays
      Matrix3x3 Cacc;
      Cacc << 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0;

      // Get the centroid for the feature
      centroid[0] = centroids[(3 * featureId) + 0];
      centroid[1] = centroids[(3 * featureId) + 1];
      centroid[2] = centroids[(3 * featureId) + 2];

      // for each triangle we need the transformation matrix A defined by the three points as columns
      // Loop over all triangle faces
      int32_t tCount = 0;
      for(usize i = 0; i < numFaces; i++)
      {
        if(faceLabels[2 * i] != featureId && faceLabels[(2 * i) + 1] != featureId)
        {
          continue;
        }
        tCount++;
        const usize compIndex = (faceLabels[2 * i] == featureId ? 0 : 1);
        std::array<nx::core::Point3Df, 3> vertCoords = GetFaceCoordinates(i, verts, triangleList);

        const nx::core::Point3Df& aVert = vertCoords[0] - centroid;
        const nx::core::Point3Df& bVert = (compIndex == 0 ? vertCoords[1] : vertCoords[2]) - centroid;
        const nx::core::Point3Df& cVert = (compIndex == 0 ? vertCoords[2] : vertCoords[1]) - centroid;

        Matrix3x3 aMat;
        aMat << aVert[0], bVert[0], cVert[0], aVert[1], bVert[1], cVert[1], aVert[2], bVert[2], cVert[2];

        const float64 detA = aMat.determinant();

        Cacc = (Cacc + detA * (aMat * (cMatrix * (aMat.transpose())))).eval();
        Vol += (detA / 6.0f); // accumulate the volumes
      }

      Cacc = (Cacc / Vol).eval();
      Cinertia = identityMat * Cacc.trace() - Cacc;
      // extract the moments from the inertia tensor
      const Eigen::Vector3d eVec(Cinertia(0, 0), Cinertia(1, 1), Cinertia(2, 2));
      auto sols = cPrime * eVec;
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
    const Eigen::EigenSolver<Matrix3x3> eigenSolver(Cinertia);

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
    //    float64 primaryAxis = eigenvalues[0].real();
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
      orientationMatrix.row(0) = col1.real();
      orientationMatrix.row(1) = col2.real();
      orientationMatrix.row(2) = col3.real();

      auto euler = OrientationTransformation::om2eu<OrientationD, OrientationD>(OrientationD(orientationMatrix.data(), 9));

      axisEulerAngles[3 * featureId] = static_cast<float32>(euler[0]);
      axisEulerAngles[(3 * featureId) + 1] = static_cast<float32>(euler[1]);
      axisEulerAngles[(3 * featureId) + 2] = static_cast<float32>(euler[2]);
    }

    /**
     * The following section finds axes
     */
    {
      if(m_ShouldCancel)
      {
        return {};
      }

      const ::AxialLengths lengths = FindIntersections(orientationMatrix, faceLabels, triangleList, verts, centroids, featureId, m_ShouldCancel);

      // Check for zeroes (zeroes = probably invalid)
      if(lengths.xLength == 0.0 || lengths.yLength == 0.0 || lengths.zLength == 0.0)
      {
        return MakeErrorResult(-64721, fmt::format("{}({}): One or more of the axis lengths for feature {} was unable to be found. This indicates the geometry was malformed.\nFeature Centroid(XYZ): "
                                                   "[{},{},{}]\nX Length: {}\nY Length: {}\nZ Length: {}",
                                                   __FILE__, __LINE__, featureId, centroids[(3 * featureId) + 0], centroids[(3 * featureId) + 1], centroids[(3 * featureId) + 2], lengths.xLength,
                                                   lengths.yLength, lengths.zLength));
      }

      axisLengths[3 * featureId] = static_cast<float32>(lengths.xLength);
      axisLengths[(3 * featureId) + 1] = static_cast<float32>(lengths.yLength);
      axisLengths[(3 * featureId) + 2] = static_cast<float32>(lengths.zLength);
      auto bOverA = static_cast<float32>(lengths.yLength / lengths.xLength);
      auto cOverA = static_cast<float32>(lengths.zLength / lengths.xLength);
      aspectRatios[2 * featureId] = bOverA;
      aspectRatios[(2 * featureId) + 1] = cOverA;
    }
  }

  return {};
}
