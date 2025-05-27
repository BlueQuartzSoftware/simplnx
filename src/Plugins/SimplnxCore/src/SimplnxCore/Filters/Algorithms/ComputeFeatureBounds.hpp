#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"

namespace nx::core
{
struct SIMPLNXCORE_EXPORT ComputeFeatureBoundsInputValues
{
  ChoicesParameter::ValueType OutputType;
  DataPath GeometryPath;
  DataPath FeatureAMPath;
  DataPath FeatureIdsArrayPath;
  DataPath MinArrayPath;
  DataPath MaxArrayPath;
  DataPath UnifiedArrayPath;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT ComputeFeatureBounds
{
public:
  ComputeFeatureBounds(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeFeatureBoundsInputValues* inputValues);
  ~ComputeFeatureBounds() noexcept;

  ComputeFeatureBounds(const ComputeFeatureBounds&) = delete;
  ComputeFeatureBounds(ComputeFeatureBounds&&) noexcept = delete;
  ComputeFeatureBounds& operator=(const ComputeFeatureBounds&) = delete;
  ComputeFeatureBounds& operator=(ComputeFeatureBounds&&) noexcept = delete;

  enum OutputDataType : uint8
  {
    Split = 0,
    Unified = 1
  };

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeFeatureBoundsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
