#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT SliceTriangleGeometryInputValues
{
  // VectorFloat32Parameter::ValueType SliceDirection;
  ChoicesParameter::ValueType SliceRange;
  float32 Zstart;
  float32 Zend;
  float32 SliceResolution;
  bool HaveRegionIds;
  DataPath CADDataContainerName;
  DataPath RegionIdArrayPath;
  DataGroupCreationParameter::ValueType SliceDataContainerName;
  DataObjectNameParameter::ValueType EdgeAttributeMatrixName;
  DataObjectNameParameter::ValueType SliceIdArrayName;
  DataObjectNameParameter::ValueType SliceAttributeMatrixName;
};

/**
 * @class SliceTriangleGeometry
 * @brief This filter slices an input Triangle Geometry, producing an Edge Geometry
 */

class SIMPLNXCORE_EXPORT SliceTriangleGeometry
{
public:
  SliceTriangleGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, SliceTriangleGeometryInputValues* inputValues);
  ~SliceTriangleGeometry() noexcept;

  SliceTriangleGeometry(const SliceTriangleGeometry&) = delete;
  SliceTriangleGeometry(SliceTriangleGeometry&&) noexcept = delete;
  SliceTriangleGeometry& operator=(const SliceTriangleGeometry&) = delete;
  SliceTriangleGeometry& operator=(SliceTriangleGeometry&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const SliceTriangleGeometryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
