#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT CropImageGeometryInputValues
{
  GeometrySelectionParameter::ValueType InputImageGeometryPath;
  DataGroupCreationParameter::ValueType OutputImageGeometryPath;
  ArraySelectionParameter::ValueType FeatureIdsPath;
  VectorUInt64Parameter::ValueType MinVoxel;
  VectorUInt64Parameter::ValueType MaxVoxel;
  BoolParameter::ValueType RenumberFeatures;
  AttributeMatrixSelectionParameter::ValueType CellFeatureAttributeMatrixPath;
  BoolParameter::ValueType RemoveOriginalGeometry;
  BoolParameter::ValueType CropXDim;
  BoolParameter::ValueType CropYDim;
  BoolParameter::ValueType CropZDim;

  // Precomputed bounds from preflight
  uint64 XMin;
  uint64 XMax;
  uint64 YMin;
  uint64 YMax;
  uint64 ZMin;
  uint64 ZMax;
};

/**
 * @class CropImageGeometry
 * @brief This algorithm implements support code for the CropImageGeometryFilter
 */

class SIMPLNXCORE_EXPORT CropImageGeometry
{
public:
  CropImageGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CropImageGeometryInputValues* inputValues);
  ~CropImageGeometry() noexcept;

  CropImageGeometry(const CropImageGeometry&) = delete;
  CropImageGeometry(CropImageGeometry&&) noexcept = delete;
  CropImageGeometry& operator=(const CropImageGeometry&) = delete;
  CropImageGeometry& operator=(CropImageGeometry&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const CropImageGeometryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
