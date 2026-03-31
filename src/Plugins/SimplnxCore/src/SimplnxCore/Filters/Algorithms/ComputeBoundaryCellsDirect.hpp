#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ComputeBoundaryCellsInputValues
{
  bool IgnoreFeatureZero;
  bool IncludeVolumeBoundary;
  DataPath ImageGeometryPath;
  DataPath FeatureIdsArrayPath;
  DataPath BoundaryCellsArrayName;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT ComputeBoundaryCellsDirect
{
public:
  ComputeBoundaryCellsDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeBoundaryCellsInputValues* inputValues);
  ~ComputeBoundaryCellsDirect() noexcept;

  ComputeBoundaryCellsDirect(const ComputeBoundaryCellsDirect&) = delete;
  ComputeBoundaryCellsDirect(ComputeBoundaryCellsDirect&&) noexcept = delete;
  ComputeBoundaryCellsDirect& operator=(const ComputeBoundaryCellsDirect&) = delete;
  ComputeBoundaryCellsDirect& operator=(ComputeBoundaryCellsDirect&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeBoundaryCellsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
