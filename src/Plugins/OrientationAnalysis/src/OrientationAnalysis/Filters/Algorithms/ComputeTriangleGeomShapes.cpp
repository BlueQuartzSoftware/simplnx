#include "ComputeTriangleGeomShapes.hpp"

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
  findMoments();
  findAxes();
  findAxisEulers();

  return {};
}

// -----------------------------------------------------------------------------
void ComputeTriangleGeomShapes::findMoments()
{
  using MeshIndexType = IGeometry::MeshIndexType;
  const auto& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TriangleGeometryPath);
  const TriStore& triangleList = triangleGeom.getFacesRef().getDataStoreRef();
  const VertsStore& verts = triangleGeom.getVerticesRef().getDataStoreRef();

  const auto& faceLabels = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FaceLabelsArrayPath);
  const auto& centroids = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CentroidsArrayPath);
  // Calculated Arrays
  auto& omega3S = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->Omega3sArrayPath);

  usize numFaces = faceLabels.getNumberOfTuples();
  usize numFeatures = centroids.getNumberOfTuples();
  m_FeatureMoments.resize(numFeatures * 6, 0.0);

  nx::core::Point3Df centroid = {0.0F, 0.0F, 0.0F};

  using Matrix3x3 = Eigen::Matrix<double, 3, 3, Eigen::RowMajor>;
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
    if(m_ShouldCancel)
    {
      return;
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
    Matrix3x3 Cinertia = ID * Cacc.trace() - Cacc;
    // extract the moments from the inertia tensor
    Eigen::Vector3d e(Cinertia(0, 0), Cinertia(1, 1), Cinertia(2, 2));
    auto sols = CC * e;
    omega3S[featureId] = static_cast<float32>(((Vol * Vol) / sols.prod()) / k_Sphere);

    m_FeatureMoments[featureId * 6 + 0] = sols[0];
    m_FeatureMoments[featureId * 6 + 1] = sols[1];
    m_FeatureMoments[featureId * 6 + 2] = sols[2];
    m_FeatureMoments[featureId * 6 + 3] = -Cinertia(0, 1);
    m_FeatureMoments[featureId * 6 + 4] = -Cinertia(0, 2);
    m_FeatureMoments[featureId * 6 + 5] = -Cinertia(1, 2);

    /**
     * This next section finds the principle axis via eigenvalues.
     * Paper/Lecture Notes (Page 5): https://ocw.mit.edu/courses/16-07-dynamics-fall-2009/dd277ec654440f4c2b5b07d6c286c3fd_MIT16_07F09_Lec26.pdf
     * Video Walkthrough [0:00-10:45]: https://www.youtube.com/watch?v=IEDniK9kmaw
     *
     * The main goal is to derive the eigenvalues from the moment of inertia tensor therein finding the eigenvectors,
     * which are the angular velocity vectors.
     *
     * Code Keynotes for reviewers:
     *  - Hessenburg Decomposition is pre-processing to get an upper/right triangular matrix.
     *  - This significantly reduces the iterations needed for QR Decomposition
     *  - QR Factorization is different than QR Decomposition.
     *  - QR Decomposition expresses the product of an Orthogonal Matrix (Q) and an right/upper triangular matrix (R) as a singular matrix (A).
     */
    // TODO:
    //  - Remove Copying between processing (inline integration)
    //  - Extract real part of complexes stored in eigenvalues/vectors
    Eigen::HessenbergDecomposition<Matrix3x3> hessDecomp(Cinertia);
    Matrix3x3 hessenMatrixUpper = hessDecomp.matrixH();

    Eigen::HouseholderQR<Matrix3x3> hQR(hessenMatrixUpper);
    Matrix3x3 hqrMatrix = hQR.matrixQR();

    // Extract eigenvalues and eigenvectors
    Eigen::EigenSolver<Matrix3x3> eigenSolver(hqrMatrix);

    // The primary axis is the largest eigenvector
    Eigen::EigenSolver<Matrix3x3>::EigenvalueType eigenvalues = eigenSolver.eigenvalues();

    // This is the angular velocity vector, each row represents an axial alignment (principle axis)
    Eigen::EigenSolver<Matrix3x3>::EigenvectorsType eigenvectors = eigenSolver.eigenvectors();

    std::cout << "Eigenvalues:\n" << eigenvalues << std::endl;
    std::cout << "\n Eigenvectors:\n" << eigenvectors << std::endl;

    constexpr char k_BaselineAxisLabel = 'x'; // x
    char axisLabel = 'x';
    double primaryAxis = eigenvalues[0].real();
    for(usize i = 1; i < eigenvalues.size(); i++)
    {
      if(primaryAxis < eigenvalues[i].real())
      {
        axisLabel = k_BaselineAxisLabel + static_cast<char>(i);
        primaryAxis = eigenvalues[i].real();
      }
    }
    std::cout << "\nPrimary Axis: " << axisLabel << " | Associated Eigenvalue: " << primaryAxis << std::endl;
  }
}

// -----------------------------------------------------------------------------
void ComputeTriangleGeomShapes::findAxes()
{
  constexpr float64 k_ScaleFactor = 1.0;
  const Float32Array& centroids = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CentroidsArrayPath);

  auto& axisLengths = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->AxisLengthsArrayPath);
  auto& aspectRatios = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->AspectRatiosArrayPath);

  usize numFeatures = centroids.getNumberOfTuples();

  m_FeatureEigenVals.resize(numFeatures * 3);

  for(usize i = 1; i < numFeatures; i++)
  {
    if(m_ShouldCancel)
    {
      return;
    }
    float64 ixx = m_FeatureMoments[i * 6 + 0];
    float64 iyy = m_FeatureMoments[i * 6 + 1];
    float64 izz = m_FeatureMoments[i * 6 + 2];

    float64 ixy = m_FeatureMoments[i * 6 + 3];
    float64 iyz = m_FeatureMoments[i * 6 + 4];
    float64 ixz = m_FeatureMoments[i * 6 + 5];

    float64 a = 1.0;
    float64 b = (-ixx - iyy - izz);
    float64 c = ((ixx * izz) + (ixx * iyy) + (iyy * izz) - (ixz * ixz) - (ixy * ixy) - (iyz * iyz));
    float64 d = ((ixz * iyy * ixz) + (ixy * izz * ixy) + (iyz * ixx * iyz) - (ixx * iyy * izz) - (ixy * iyz * ixz) - (ixy * iyz * ixz));

    // f and g are the p and q values when reducing the cubic equation to t^3 + pt + q = 0
    float64 f = ((3.0 * c / a) - ((b / a) * (b / a))) / 3.0;
    float64 g = ((2.0 * (b / a) * (b / a) * (b / a)) - (9.0 * b * c / (a * a)) + (27.0 * (d / a))) / 27.0;
    float64 h = (g * g / 4.0) + (f * f * f / 27.0);
    float64 rSquare = (g * g / 4.0) - h;
    float64 r = sqrt(rSquare);
    if(rSquare < 0.0)
    {
      r = 0.0;
    }
    float64 theta = 0;
    if(r == 0)
    {
      theta = 0;
    }
    if(r != 0)
    {
      float64 value = -g / (2.0 * r);
      if(value > 1)
      {
        value = 1.0;
      }
      if(value < -1)
      {
        value = -1.0;
      }
      theta = acos(value);
    }
    float64 const1 = pow(r, 0.33333333333);
    float64 const2 = cos(theta / 3.0);
    float64 const3 = b / (3.0 * a);
    float64 const4 = 1.7320508 * sin(theta / 3.0);

    float64 r1 = 2 * const1 * const2 - (const3);
    float64 r2 = -const1 * (const2 - (const4)) - const3;
    float64 r3 = -const1 * (const2 + (const4)) - const3;
    m_FeatureEigenVals[3 * i] = r1;
    m_FeatureEigenVals[3 * i + 1] = r2;
    m_FeatureEigenVals[3 * i + 2] = r3;

    float64 i1 = (15.0 * r1) / (4.0 * M_PI);
    float64 i2 = (15.0 * r2) / (4.0 * M_PI);
    float64 i3 = (15.0 * r3) / (4.0 * M_PI);
    float64 aRatio = (i1 + i2 - i3) / 2.0f;
    float64 bRatio = (i1 + i3 - i2) / 2.0f;
    float64 cRatio = (i2 + i3 - i1) / 2.0f;
    a = (aRatio * aRatio * aRatio * aRatio) / (bRatio * cRatio);
    a = pow(a, 0.1);
    b = bRatio / aRatio;
    b = sqrt(b) * a;
    c = aRatio / (a * a * a * b);

    axisLengths[3 * i] = static_cast<float32>(a / k_ScaleFactor);
    axisLengths[3 * i + 1] = static_cast<float32>(b / k_ScaleFactor);
    axisLengths[3 * i + 2] = static_cast<float32>(c / k_ScaleFactor);
    auto bOverA = static_cast<float32>(b / a);
    auto cOverA = static_cast<float32>(c / a);
    if(aRatio == 0 || bRatio == 0 || cRatio == 0)
    {
      bOverA = 0.0f, cOverA = 0.0f;
    }
    aspectRatios[2 * i] = bOverA;
    aspectRatios[2 * i + 1] = cOverA;
  }
}

// -----------------------------------------------------------------------------
void ComputeTriangleGeomShapes::findAxisEulers()
{
  const Float32Array& centroids = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CentroidsArrayPath);
  usize numFeatures = centroids.getNumberOfTuples();

  auto& axisEulerAngles = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->AxisEulerAnglesArrayPath);

  for(usize i = 1; i < numFeatures; i++)
  {
    if(m_ShouldCancel)
    {
      return;
    }
    float64 ixx = m_FeatureMoments[i * 6 + 0];
    float64 iyy = m_FeatureMoments[i * 6 + 1];
    float64 izz = m_FeatureMoments[i * 6 + 2];
    float64 ixy = m_FeatureMoments[i * 6 + 3];
    float64 iyz = m_FeatureMoments[i * 6 + 4];
    float64 ixz = m_FeatureMoments[i * 6 + 5];
    float64 radius1 = m_FeatureEigenVals[3 * i];
    float64 radius2 = m_FeatureEigenVals[3 * i + 1];
    float64 radius3 = m_FeatureEigenVals[3 * i + 2];

    float64 e[3][1];
    float64 vect[3][3] = {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};
    e[0][0] = radius1;
    e[1][0] = radius2;
    e[2][0] = radius3;
    float64 uber[3][3] = {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};
    float64 bMatrix[3];
    bMatrix[0] = 0.0000001;
    bMatrix[1] = 0.0000001;
    bMatrix[2] = 0.0000001;

    for(int32 j = 0; j < 3; j++)
    {
      uber[0][0] = ixx - e[j][0];
      uber[0][1] = ixy;
      uber[0][2] = ixz;
      uber[1][0] = ixy;
      uber[1][1] = iyy - e[j][0];
      uber[1][2] = iyz;
      uber[2][0] = ixz;
      uber[2][1] = iyz;
      uber[2][2] = izz - e[j][0];
      std::array<std::array<float64, 3>, 3> uberelim{};
      std::array<std::array<float64, 1>, 3> uberbelim{};
      int32 elimCount = 0;

      for(int32 a = 0; a < 3; a++)
      {
        for(int32 b = 0; b < 3; b++)
        {
          uberelim[elimCount][b] = uber[a][b];
        }
        uberbelim[elimCount][0] = bMatrix[a];
        elimCount++;
      }
      for(int32 k = 0; k < elimCount - 1; k++)
      {
        for(int32 l = k + 1; l < elimCount; l++)
        {
          float64 c = uberelim[l][k] / uberelim[k][k];
          for(int32 r = k + 1; r < elimCount; r++)
          {
            uberelim[l][r] = uberelim[l][r] - c * uberelim[k][r];
          }
          uberbelim[l][0] = uberbelim[l][0] - c * uberbelim[k][0];
        }
      }
      uberbelim[elimCount - 1][0] = uberbelim[elimCount - 1][0] / uberelim[elimCount - 1][elimCount - 1];
      for(int32 l = 1; l < elimCount; l++)
      {
        int32 r = (elimCount - 1) - l;
        float64 sum = 0.0;
        for(int32 n = r + 1; n < elimCount; n++)
        {
          sum = sum + (uberelim[r][n] * uberbelim[n][0]);
        }
        uberbelim[r][0] = (uberbelim[r][0] - sum) / uberelim[r][r];
      }
      for(int32 p = 0; p < elimCount; p++)
      {
        vect[j][p] = uberbelim[p][0];
      }
    }

    float64 n1X = vect[0][0];
    float64 n1Y = vect[0][1];
    float64 n1Z = vect[0][2];
    float64 n2X = vect[1][0];
    float64 n2Y = vect[1][1];
    float64 n2Z = vect[1][2];
    float64 n3X = vect[2][0];
    float64 n3Y = vect[2][1];
    float64 n3Z = vect[2][2];
    float64 norm1 = sqrt(((n1X * n1X) + (n1Y * n1Y) + (n1Z * n1Z)));
    float64 norm2 = sqrt(((n2X * n2X) + (n2Y * n2Y) + (n2Z * n2Z)));
    float64 norm3 = sqrt(((n3X * n3X) + (n3Y * n3Y) + (n3Z * n3Z)));
    n1X = n1X / norm1;
    n1Y = n1Y / norm1;
    n1Z = n1Z / norm1;
    n2X = n2X / norm2;
    n2Y = n2Y / norm2;
    n2Z = n2Z / norm2;
    n3X = n3X / norm3;
    n3Y = n3Y / norm3;
    n3Z = n3Z / norm3;

    // insert principal unit vectors into rotation matrix representing Feature reference frame within the sample reference frame
    //(Note that the 3 direction is actually the long axis and the 1 direction is actually the short axis)
    float32 g[3][3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
    g[0][0] = static_cast<float32>(n3X);
    g[0][1] = static_cast<float32>(n3Y);
    g[0][2] = static_cast<float32>(n3Z);
    g[1][0] = static_cast<float32>(n2X);
    g[1][1] = static_cast<float32>(n2Y);
    g[1][2] = static_cast<float32>(n2Z);
    g[2][0] = static_cast<float32>(n1X);
    g[2][1] = static_cast<float32>(n1Y);
    g[2][2] = static_cast<float32>(n1Z);

    // check for right-handedness
    OrientationTransformation::ResultType result = OrientationTransformation::om_check(OrientationF(g));
    if(result.result == 0)
    {
      g[2][0] *= -1.0f;
      g[2][1] *= -1.0f;
      g[2][2] *= -1.0f;
    }

    auto euler = OrientationTransformation::om2eu<OrientationF, OrientationF>(OrientationF(g));

    axisEulerAngles[3 * i] = euler[0];
    axisEulerAngles[3 * i + 1] = euler[1];
    axisEulerAngles[3 * i + 2] = euler[2];
  }
}
