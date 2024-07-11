/* ============================================================================
 * Copyright (c) 2009-2023 BlueQuartz Software, LLC
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * Neither the name of BlueQuartz Software, the US Air Force, nor the names of its
 * contributors may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The code contained herein was partially funded by the following contracts:
 *    United States Air Force Prime Contract FA8650-07-D-5800
 *    United States Air Force Prime Contract FA8650-10-D-5210
 *    United States Prime Contract Navy N00173-07-C-2068
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "CalculateTriangleGroupCurvatures.hpp"

#include "simplnx/Utilities/Math/MatrixMath.hpp"
#include "simplnx/Utilities/ParallelAlgorithmUtilities.hpp"

#include "SimplnxCore/Filters/Algorithms/FindNRingNeighbors.hpp"
#include "SimplnxCore/Filters/FeatureFaceCurvatureFilter.hpp"

#include <Eigen/Dense>

namespace nx::core
{
namespace
{
/**
 * @brief extractPatchData Extracts out the needed data values from the global arrays
 * @param triId The seed triangle Id
 * @param triPatch The group of triangles being used
 * @param data The data to extract from
 * @return Shared pointer to the extracted data
 */
std::shared_ptr<Float64DataStore> extractPatchData(int64_t triId, CalculateTriangleGroupCurvatures::UniqueFaceIds_t& triPatch, Float64AbstractDataStore& data)
{
  IDataStore::ShapeType cDims = {3ULL};

  for(auto iter = triPatch.begin(); iter != triPatch.end();)
  {
    int64_t t = *iter;

    if(std::isnan(data[t * 3]) || std::isnan(data[t * 3 + 1]) || std::isnan(data[t * 3 + 2]))
    {
      iter = triPatch.erase(iter);
      if(*iter == triId)
      {
        triId = *(triPatch.begin());
      }
    }
    else
    {
      ++iter;
    }
  }

  if(triPatch.empty())
  {
    return nullptr;
  }

  size_t totalTuples = triPatch.size();
  if(triPatch.count(triId) == 0)
  {
    totalTuples++;
  }

  IDataStore::ShapeType tupleShape = {totalTuples};
  std::optional<float64> initValue = {};
  std::shared_ptr<Float64DataStore> extractedData = std::make_shared<Float64DataStore>(tupleShape, cDims, initValue);
  // This little chunk makes sure the current seed triangles centroid and normal data appear
  // first in the returned arrays which makes the next steps a tad easier.
  int32_t i = 0;
  extractedData->setComponent(i, 0, data[triId * 3]);
  extractedData->setComponent(i, 1, data[triId * 3 + 1]);
  extractedData->setComponent(i, 2, data[triId * 3 + 2]);
  ++i;
  triPatch.erase(triId);

  for(int64_t t : triPatch)
  {
    extractedData->setComponent(i, 0, data[t * 3]);
    extractedData->setComponent(i, 1, data[t * 3 + 1]);
    extractedData->setComponent(i, 2, data[t * 3 + 2]);
    ++i;
  }
  triPatch.insert(triId);

  extractedData->resizeTuples({triPatch.size()}); // Resize the TriPatch DataArray
  return extractedData;
}
} // namespace

// -----------------------------------------------------------------------------
CalculateTriangleGroupCurvatures::CalculateTriangleGroupCurvatures(FeatureFaceCurvature* filter, int64_t nring, std::vector<int64_t> triangleIds, bool useNormalsForCurveFitting,
                                                                   Float64Array* principleCurvature1, Float64Array* principleCurvature2, Float64Array* principleDirection1,
                                                                   Float64Array* principleDirection2, Float64Array* gaussianCurvature, Float64Array* meanCurvature, Float64Array* weingartenMatrix,
                                                                   TriangleGeom* trianglesGeom, Int32Array* surfaceMeshFaceLabels, Float64Array* surfaceMeshFaceNormals,
                                                                   Float64Array* surfaceMeshTriangleCentroids, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel)
: m_Filter(filter)
, m_NRing(nring)
, m_TriangleIds(std::move(triangleIds))
, m_UseNormalsForCurveFitting(useNormalsForCurveFitting)
, m_PrincipleCurvature1(principleCurvature1)
, m_PrincipleCurvature2(principleCurvature2)
, m_PrincipleDirection1(principleDirection1)
, m_PrincipleDirection2(principleDirection2)
, m_GaussianCurvature(gaussianCurvature)
, m_MeanCurvature(meanCurvature)
, m_WeingartenMatrix(weingartenMatrix)
, m_TrianglesPtr(trianglesGeom)
, m_SurfaceMeshFaceLabels(surfaceMeshFaceLabels)
, m_SurfaceMeshFaceNormals(surfaceMeshFaceNormals)
, m_SurfaceMeshTriangleCentroids(surfaceMeshTriangleCentroids)
, m_MessageHandler(messageHandler)
, m_ShouldCancel(shouldCancel)
{
}

// -----------------------------------------------------------------------------
CalculateTriangleGroupCurvatures::~CalculateTriangleGroupCurvatures() = default;

// -----------------------------------------------------------------------------
void subtractVector3d(Float64AbstractDataStore& data, double* v)
{
  size_t count = data.getNumberOfTuples();
  // auto& ptr = data.getDataStoreRef();
  for(size_t i = 0; i < count; ++i)
  {
    const usize offset = i * 3;
    data.setValue(offset + 0, data.getValue(offset + 0) - v[0]);
    data.setValue(offset + 1, data.getValue(offset + 1) - v[1]);
    data.setValue(offset + 2, data.getValue(offset + 2) - v[2]);
  }
}

// -----------------------------------------------------------------------------
void CalculateTriangleGroupCurvatures::operator()() const
{
  if(m_TriangleIds.empty())
  {
    return;
  }

  // Instantiate a FindNRingNeighbors class to use during the loop
  auto& faceLabels = m_SurfaceMeshFaceLabels->getDataStoreRef();
  usize triangleIdOffset = m_TriangleIds[0] * 2;

  int32_t feature0 = 0;
  int32_t feature1 = 0;
  if(faceLabels[triangleIdOffset] < faceLabels[triangleIdOffset + 1])
  {
    feature0 = faceLabels[triangleIdOffset];
    feature1 = faceLabels[triangleIdOffset + 1];
  }
  else
  {
    feature0 = faceLabels[triangleIdOffset + 1];
    feature1 = faceLabels[triangleIdOffset];
  }

  bool computeGaussian = (m_GaussianCurvature != nullptr);
  bool computeMean = (m_MeanCurvature != nullptr);
  bool computeDirection = (m_PrincipleDirection1 != nullptr);
  bool computeWeingartenMatrix = (m_WeingartenMatrix != nullptr);

  std::vector<int64_t>::size_type tCount = m_TriangleIds.size();
  // For each triangle in the group
  for(std::vector<int64_t>::size_type i = 0; i < tCount; ++i)
  {
    if(m_ShouldCancel)
    {
      return;
    }

    int64_t triId = m_TriangleIds[i];
    FindNRingNeighborsInputValues inputs;
    inputs.TriangleGeomPtr = m_TrianglesPtr;
    inputs.TriangleId = triId;
    inputs.RegionId0 = feature0;
    inputs.RegionId1 = feature1;
    inputs.Ring = m_NRing;
    inputs.FaceLabelsArray = m_SurfaceMeshFaceLabels;

    FindNRingNeighbors nRingNeighborAlg(&inputs);
    auto result = nRingNeighborAlg(m_MessageHandler, m_ShouldCancel);

    // generate(m_TrianglesPtr, m_SurfaceMeshFaceLabels);
    if(result.invalid())
    {
      throw std::runtime_error("NRingNeighbor returned an error.");
    }

    UniqueFaceIds_t triPatch = nRingNeighborAlg.getNRingTriangles();
    if(triPatch.size() <= 1)
    {
      throw std::runtime_error("NRingNeighbor failed to find more than one triangles.");
    }

    size_t beforeSize = triPatch.size();
    std::shared_ptr<Float64DataStore> patchNormals = extractPatchData(triId, triPatch, m_SurfaceMeshFaceNormals->getDataStoreRef());
    // if every triangle got removed because of NaN values in the Normals, then bail from this iteration of the loop
    if(nullptr == patchNormals)
    {
      continue;
    }

    // If something got removed, redo this part again.
    if(triPatch.size() != beforeSize)
    {
      beforeSize = triPatch.size();
      patchNormals = extractPatchData(triId, triPatch, m_SurfaceMeshFaceNormals->getDataStoreRef());
    }

    beforeSize = triPatch.size();
    std::shared_ptr<Float64DataStore> patchCentroids = extractPatchData(triId, triPatch, m_SurfaceMeshTriangleCentroids->getDataStoreRef());
    // if every triangle got removed because of NaN values in the Normals, then bail from this iteration of the loop
    if(nullptr == patchCentroids)
    {
      continue;
    }

    // If something got removed, redo this part again.
    if(triPatch.size() != beforeSize)
    {
      beforeSize = triPatch.size();
      patchNormals = extractPatchData(triId, triPatch, m_SurfaceMeshFaceNormals->getDataStoreRef());
      patchCentroids = extractPatchData(triId, triPatch, m_SurfaceMeshTriangleCentroids->getDataStoreRef());
    }

    // Translate the patch to the 0,0,0 origin
    double sub[3] = {patchCentroids->getComponentValue(0, 0), patchCentroids->getComponentValue(0, 1), patchCentroids->getComponentValue(0, 2)};
    subtractVector3d(*patchCentroids, sub);

    double np[3] = {patchNormals->getComponentValue(0, 0), patchNormals->getComponentValue(0, 1), patchNormals->getComponentValue(0, 2)};

    double seedCentroid[3] = {patchCentroids->getComponentValue(0, 0), patchCentroids->getComponentValue(0, 1), patchCentroids->getComponentValue(0, 2)};
    double firstCentroid[3] = {patchCentroids->getComponentValue(1, 0), patchCentroids->getComponentValue(1, 1), patchCentroids->getComponentValue(1, 2)};

    double temp[3] = {firstCentroid[0] - seedCentroid[0], firstCentroid[1] - seedCentroid[1], firstCentroid[2] - seedCentroid[2]};
    double vp[3] = {0.0, 0.0, 0.0};

    // Cross Product of np and temp
    MatrixMath::Normalize3x1(np);
    MatrixMath::CrossProduct(np, temp, vp);
    MatrixMath::Normalize3x1(vp);

    // get the third orthogonal vector
    double up[3] = {0.0, 0.0, 0.0};
    MatrixMath::CrossProduct(vp, np, up);

    // this constitutes a rotation matrix to a local coordinate system
    double rot[3][3] = {{up[0], up[1], up[2]}, {vp[0], vp[1], vp[2]}, {np[0], np[1], np[2]}};
    double out[3] = {0.0, 0.0, 0.0};
    // Transform all centroids and normals to new coordinate system
    for(size_t m = 0; m < patchCentroids->getNumberOfTuples(); ++m)
    {
      ::memcpy(out, &patchCentroids->data()[m * 3], 3 * sizeof(double));
      MatrixMath::Multiply3x3with3x1(rot, &patchCentroids->data()[m * 3], out);
      if(std::isnan(out[0]) || std::isnan(out[1]) || std::isnan(out[2]))
      {
        break;
      }
      ::memcpy(&patchCentroids->data()[m * 3], out, 3 * sizeof(double));

      ::memcpy(out, &patchNormals->data()[m * 3], 3 * sizeof(double));
      MatrixMath::Multiply3x3with3x1(rot, &patchNormals->data()[m * 3], out);
      ::memcpy(&patchNormals->data()[m * 3], out, 3 * sizeof(double));

      // We rotate the normals now but we dont use them yet. If we start using part 3 of Goldfeathers paper then we
      // will need the normals.
    }

    {
      // Solve the Least Squares fit
      static const uint32_t NO_NORMALS = 3;
      static const uint32_t USE_NORMALS = 7;
      uint32_t cols = NO_NORMALS;
      if(m_UseNormalsForCurveFitting == true)
      {
        cols = USE_NORMALS;
      }
      size_t rows = patchCentroids->getNumberOfTuples();
      Eigen::MatrixXd A(rows, cols);
      Eigen::VectorXd b(rows);
      double x = 0.0, y = 0.0, z = 0.0;
      for(size_t m = 0; m < rows; ++m)
      {
        x = patchCentroids->getComponentValue(m, 0);
        y = patchCentroids->getComponentValue(m, 1);
        z = patchCentroids->getComponentValue(m, 2);

        A(m) = 0.5 * x * x;            // 1/2 x^2
        A(m + rows) = x * y;           // x*y
        A(m + rows * 2) = 0.5 * y * y; // 1/2 y^2
        if(m_UseNormalsForCurveFitting)
        {
          A(m + rows * 3) = x * x * x;
          A(m + rows * 4) = x * x * y;
          A(m + rows * 5) = x * y * y;
          A(m + rows * 6) = y * y * y;
        }
        b[m] = z; // The Z Values
      }

      Eigen::Matrix2d M;

      if(!m_UseNormalsForCurveFitting)
      {
        typedef Eigen::Matrix<double, NO_NORMALS, 1> Vector3d;
        Vector3d sln1 = A.colPivHouseholderQr().solve(b);
        // Now that we have the A, B, C constants we can solve the Eigen value/vector problem
        // to get the principal curvatures and principal directions.
        M << sln1(0), sln1(1), sln1(1), sln1(2);
      }
      else
      {
        typedef Eigen::Matrix<double, USE_NORMALS, 1> Vector7d;
        Vector7d sln1 = A.colPivHouseholderQr().solve(b);
        // Now that we have the A, B, C, D, E, F & G constants we can solve the Eigen value/vector problem
        // to get the principal curvatures and pricipal directions.
        M << sln1(0), sln1(1), sln1(1), sln1(2);
      }

      if(computeWeingartenMatrix)
      {
        m_WeingartenMatrix->setComponent(triId, 0, M.coeff(0, 0));
        m_WeingartenMatrix->setComponent(triId, 1, M.coeff(0, 1));
        m_WeingartenMatrix->setComponent(triId, 2, M.coeff(1, 0));
        m_WeingartenMatrix->setComponent(triId, 3, M.coeff(1, 1));
      }

      Eigen::SelfAdjointEigenSolver<Eigen::Matrix2d> eig(M);
      Eigen::SelfAdjointEigenSolver<Eigen::Matrix2d>::RealVectorType eValues = eig.eigenvalues();
      Eigen::SelfAdjointEigenSolver<Eigen::Matrix2d>::MatrixType eVectors = eig.eigenvectors();

      // Kappa1 >= Kappa2
      double kappa1 = eValues(0) * -1; // Kappa 1
      double kappa2 = eValues(1) * -1; // kappa 2
      if(kappa1 < kappa2)
      {
        throw std::runtime_error("Kappa1 should be >= Kappa2");
      }
      m_PrincipleCurvature1->setValue(triId, kappa1);
      m_PrincipleCurvature2->setValue(triId, kappa2);

      if(computeGaussian)
      {
        m_GaussianCurvature->setValue(triId, kappa1 * kappa2);
      }
      if(computeMean)
      {
        m_MeanCurvature->setValue(triId, (kappa1 + kappa2) / 2.0);
      }

      if(computeDirection)
      {
        Eigen::Matrix3d e_rot_T;
        e_rot_T.row(0) = Eigen::Vector3d(up[0], vp[0], np[0]);
        e_rot_T.row(1) = Eigen::Vector3d(up[1], vp[1], np[1]);
        e_rot_T.row(2) = Eigen::Vector3d(up[2], vp[2], np[2]);

        // Rotate our principal directions back into the original coordinate system
        Eigen::Vector3d dir1(eVectors.col(0)(0), eVectors.col(0)(1), 0.0);
        dir1 = e_rot_T * dir1;
        std::copy(dir1.data(), dir1.data() + 3, m_PrincipleDirection1->begin() + (triId * 3));

        Eigen::Vector3d dir2(eVectors.col(1)(0), eVectors.col(1)(1), 0.0);
        dir2 = e_rot_T * dir2;
        std::copy(dir2.data(), dir2.data() + 3, m_PrincipleDirection2->begin() + (triId * 3));
      }
    }

  } // End Loop over this triangle

  // Send some feedback
  m_Filter->sendThreadSafeProgressMessage(1);
}
} // namespace nx::core
