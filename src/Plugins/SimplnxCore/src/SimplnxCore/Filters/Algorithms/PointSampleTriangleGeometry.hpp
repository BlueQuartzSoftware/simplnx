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

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"

#include <array>
#include <filesystem>
#include <memory>
#include <random>
#include <string>

namespace nx::core
{

struct SIMPLNXCORE_EXPORT PointSampleTriangleGeometryInputs
{
  int32 pNumberOfSamples;
  bool pUseMask;
  DataPath pTriangleGeometry;
  DataPath pTriangleAreasArrayPath;
  DataPath pMaskArrayPath;
  MultiArraySelectionParameter::ValueType pSelectedDataArrayPaths;
  DataPath pVertexGeometryPath;
  DataPath pVertexGroupDataPath;
  MultiArraySelectionParameter::ValueType pCreatedDataArrayPaths;
  uint64 Seed;
};

/**
 * @brief The PointSampleTriangleGeometry class. See [Filter documentation](@ref pointsampletrianglegeometry) for details.
 */
class SIMPLNXCORE_EXPORT PointSampleTriangleGeometry
{

public:
  PointSampleTriangleGeometry(DataStructure& dataStructure, PointSampleTriangleGeometryInputs* inputValues, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler);
  ~PointSampleTriangleGeometry() noexcept;

  PointSampleTriangleGeometry(const PointSampleTriangleGeometry&) = delete;
  PointSampleTriangleGeometry(PointSampleTriangleGeometry&&) noexcept = delete;
  PointSampleTriangleGeometry& operator=(const PointSampleTriangleGeometry&) = delete;
  PointSampleTriangleGeometry& operator=(PointSampleTriangleGeometry&&) noexcept = delete;

  using MeshIndexType = IGeometry::MeshIndexType;

  Result<> operator()();

private:
#if 0
  double* m_TriangleAreas = nullptr;
  bool* m_Mask = nullptr;

  int m_SamplesNumberType = {0};
  DataPath m_TriangleGeometry = {SIMPL::Defaults::TriangleDataContainerName, "", ""};
  std::string m_VertexGeometry = {SIMPL::Defaults::VertexDataContainerName};
  std::string m_VertexAttributeMatrixName = {SIMPL::Defaults::VertexAttributeMatrixName};
  int m_NumberOfSamples = {100000};
  DataPath m_ParentGeometry = {"", "", ""};
  DataPath m_TriangleAreasArrayPath = {SIMPL::Defaults::TriangleDataContainerName, SIMPL::Defaults::FaceAttributeMatrixName, SIMPL::FaceData::SurfaceMeshFaceAreas};
  bool m_UseMask = {false};
  DataPath m_MaskArrayPath = {"", "", ""};
  std::vector<DataPath> m_SelectedDataPaths = {};

  std::vector<IDataArray::WeakPointer> m_SelectedWeakPtrVector;
  std::vector<IDataArray::WeakPointer> m_CreatedWeakPtrVector;

  int32_t m_NumSamples = 0;
#endif

  DataStructure& m_DataStructure;
  const PointSampleTriangleGeometryInputs* m_Inputs = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // end namespace nx::core
