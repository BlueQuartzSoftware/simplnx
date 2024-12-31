#include "ComputeTriangleGeomShapes.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"

#include "EbsdLib/Core/Orientation.hpp"
#include "EbsdLib/Core/OrientationTransformation.hpp"

using namespace nx::core;

namespace
{
using TriStore = AbstractDataStore<INodeGeometry2D::SharedFaceList::value_type>;
using VertsStore = AbstractDataStore<INodeGeometry0D::SharedVertexList::value_type>;

constexpr double k_Multiplier = 1.0 / (4.0 * Constants::k_PiD);
constexpr float64 k_ScaleFactor = 1.0;

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

  const auto& faceLabels = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FaceLabelsArrayPath);
  const auto& centroids = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CentroidsArrayPath);

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

    // !!! DO NOT REMOVE ZEROING !!! It is integral to numerical stability in the following calculations
    // Zero out small numbers in Cinertia: https://stackoverflow.com/a/54505281
    Cinertia = (0.000000001 < Cinertia.array().abs()).select(Cinertia, 0.0);

    Eigen::EigenSolver<Matrix3x3> eigenSolver(Cinertia); // pass in HQR Matrix for implicit calculation

    // The primary axis is the largest eigenvalue
    Eigen::EigenSolver<Matrix3x3>::EigenvalueType eigenvalues = eigenSolver.eigenvalues();

    // This is the angular velocity vector, each row represents an axial alignment (principle axis)
    Eigen::EigenSolver<Matrix3x3>::EigenvectorsType eigenvectors = eigenSolver.eigenvectors();

    /**
     * Following section for debugging
     */
    //        std::cout << "Eigenvalues:\n" << eigenvalues << std::endl;
    //        std::cout << "\n Eigenvectors:\n" << eigenvectors << std::endl;
    //
    //        constexpr char k_BaselineAxisLabel = 'x'; // x
    //        char axisLabel = 'x';
    //        double primaryAxis = eigenvalues[0].real();
    //        for(usize i = 1; i < eigenvalues.size(); i++)
    //        {
    //          if(primaryAxis < eigenvalues[i].real())
    //          {
    //            axisLabel = k_BaselineAxisLabel + static_cast<char>(i);
    //            primaryAxis = eigenvalues[i].real();
    //          }
    //        }
    //        std::cout << "\nPrimary Axis: " << axisLabel << " | Associated Eigenvalue: " << primaryAxis << std::endl;

    // Presort eigen ordering for following sections
    // Returns the argument order sorted high to low
    std::array<size_t, 3> idxs = ::TripletSort(eigenvalues[0].real(), eigenvalues[1].real(), eigenvalues[2].real(), false);

    /**
     * The following section finds axes
     */
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      // Formula: I = (15.0 * eigenvalue) / (4.0 * Pi);
      // in the below implementation the original divisor has been put under one to avoid repeated division during execution
      double I1 = (15.0 * eigenvalues[idxs[0]].real()) * k_Multiplier;
      double I2 = (15.0 * eigenvalues[idxs[1]].real()) * k_Multiplier;
      double I3 = (15.0 * eigenvalues[idxs[2]].real()) * k_Multiplier;

      // Adjust to ABC of ellipsoid volume
      double aRatio = (I1 + I2 - I3) * 0.5;
      double bRatio = (I1 + I3 - I2) * 0.5;
      double cRatio = (I2 + I3 - I1) * 0.5;
      double a = (aRatio * aRatio * aRatio * aRatio) / (bRatio * cRatio);
      a = std::pow(a, 0.1);
      double b = bRatio / aRatio;
      b = std::sqrt(b) * a;
      double c = aRatio / (a * a * a * b);

      axisLengths[3 * featureId] = static_cast<float32>(a / k_ScaleFactor);
      axisLengths[3 * featureId + 1] = static_cast<float32>(b / k_ScaleFactor);
      axisLengths[3 * featureId + 2] = static_cast<float32>(c / k_ScaleFactor);
      auto bOverA = static_cast<float32>(b / a);
      auto cOverA = static_cast<float32>(c / a);
      if(aRatio == 0 || bRatio == 0 || cRatio == 0)
      {
        bOverA = 0.0f, cOverA = 0.0f;
      }
      aspectRatios[2 * featureId] = bOverA;
      aspectRatios[2 * featureId + 1] = cOverA;
    }

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
  }

  return {};
}
