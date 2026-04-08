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
class SIMPLNXCORE_EXPORT ComputeBoundaryCells
{
public:
  ComputeBoundaryCells(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeBoundaryCellsInputValues* inputValues);
  ~ComputeBoundaryCells() noexcept;

  ComputeBoundaryCells(const ComputeBoundaryCells&) = delete;
  ComputeBoundaryCells(ComputeBoundaryCells&&) noexcept = delete;
  ComputeBoundaryCells& operator=(const ComputeBoundaryCells&) = delete;
  ComputeBoundaryCells& operator=(ComputeBoundaryCells&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeBoundaryCellsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
