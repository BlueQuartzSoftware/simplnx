/* ============================================================================
 * Copyright (c) 2009-2016 BlueQuartz Software, LLC
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

#pragma once

#include <set>

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Filter/IFilter.hpp"

#include "ComplexCore/ComplexCore_export.hpp"

class FeatureFaceCurvatureFilter;

namespace complex
{
/**
 * @brief The CalculateTriangleGroupCurvatures class calculates the curvature values for a group of triangles
 * where each triangle in the group will have the 2 Principal Curvature values computed and optionally
 * the 2 Principal Directions and optionally the Mean and Gaussian Curvature computed.
 */
class COMPLEXCORE_EXPORT CalculateTriangleGroupCurvatures
{
public:
  CalculateTriangleGroupCurvatures(int64_t nring, std::vector<int64_t> triangleIds, bool useNormalsForCurveFitting, Float64Array* principleCurvature1, Float64Array* principleCurvature2,
                                   Float64Array* principleDirection1, Float64Array* principleDirection2, Float64Array* gaussianCurvature, Float64Array* meanCurvature, Float64Array* weingartenMatrix,
                                   TriangleGeom* trianglesGeom, Int32Array* surfaceMeshFaceLabels, Float64Array* surfaceMeshFaceNormals, Float64Array* surfaceMeshTriangleCentroids,
                                   const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel);

  virtual ~CalculateTriangleGroupCurvatures();

  void operator()() const;

  using UniqueFaceIds_t = std::set<int64_t>;

protected:
  /**
   * @brief extractPatchData Extracts out the needed data values from the global arrays
   * @param triId The seed triangle Id
   * @param triPatch The group of triangles being used
   * @param data The data to extract from
   * @return Shared pointer to the extracted data
   */
  std::shared_ptr<Float64DataStore> extractPatchData(int64_t triId, UniqueFaceIds_t& triPatch, Float64AbstractDataStore& data) const;

private:
  int64_t m_NRing;
  std::vector<int64_t> m_TriangleIds;
  bool m_UseNormalsForCurveFitting;
  Float64Array* m_PrincipleCurvature1;
  Float64Array* m_PrincipleCurvature2;
  Float64Array* m_PrincipleDirection1;
  Float64Array* m_PrincipleDirection2;
  Float64Array* m_GaussianCurvature;
  Float64Array* m_MeanCurvature;
  Float64Array* m_WeingartenMatrix;
  TriangleGeom* m_TrianglesPtr;
  Int32Array* m_SurfaceMeshFaceLabels;
  Float64Array* m_SurfaceMeshFaceNormals;
  Float64Array* m_SurfaceMeshTriangleCentroids;
  const IFilter::MessageHandler& m_MessageHandler;
  const std::atomic_bool& m_ShouldCancel;
};
} // namespace complex
