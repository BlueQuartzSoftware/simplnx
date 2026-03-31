#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ComputeSurfaceFeaturesInputValues
{
  AttributeMatrixSelectionParameter::ValueType FeatureAttributeMatrixPath;
  ArraySelectionParameter::ValueType FeatureIdsPath;
  GeometrySelectionParameter::ValueType InputImageGeometryPath;
  BoolParameter::ValueType MarkFeature0Neighbors;
  DataObjectNameParameter::ValueType SurfaceFeaturesArrayName;
};

/**
 * @class ComputeSurfaceFeaturesDirect
 * @brief This algorithm implements support code for the ComputeSurfaceFeaturesFilter
 */

class SIMPLNXCORE_EXPORT ComputeSurfaceFeaturesDirect
{
public:
  ComputeSurfaceFeaturesDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeSurfaceFeaturesInputValues* inputValues);
  ~ComputeSurfaceFeaturesDirect() noexcept;

  ComputeSurfaceFeaturesDirect(const ComputeSurfaceFeaturesDirect&) = delete;
  ComputeSurfaceFeaturesDirect(ComputeSurfaceFeaturesDirect&&) noexcept = delete;
  ComputeSurfaceFeaturesDirect& operator=(const ComputeSurfaceFeaturesDirect&) = delete;
  ComputeSurfaceFeaturesDirect& operator=(ComputeSurfaceFeaturesDirect&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeSurfaceFeaturesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
