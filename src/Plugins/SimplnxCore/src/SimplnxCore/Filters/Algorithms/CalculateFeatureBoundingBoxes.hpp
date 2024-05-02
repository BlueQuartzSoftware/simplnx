#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT CalculateFeatureBoundingBoxesInputValues
{
  DataPath FeatureIdsArrayPath;
  DataPath FeatureDataAttributeMatrixPath;
  DataPath FeatureRectArrayPath;
};

class SIMPLNXCORE_EXPORT CalculateFeatureBoundingBoxes
{
public:
  CalculateFeatureBoundingBoxes(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CalculateFeatureBoundingBoxesInputValues* inputValues);
  ~CalculateFeatureBoundingBoxes() noexcept;

  CalculateFeatureBoundingBoxes(const CalculateFeatureBoundingBoxes&) = delete;
  CalculateFeatureBoundingBoxes(CalculateFeatureBoundingBoxes&&) noexcept = delete;
  CalculateFeatureBoundingBoxes& operator=(const CalculateFeatureBoundingBoxes&) = delete;
  CalculateFeatureBoundingBoxes& operator=(CalculateFeatureBoundingBoxes&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const CalculateFeatureBoundingBoxesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
