#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT FindBoundaryCellsInputValues
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
class SIMPLNXCORE_EXPORT FindBoundaryCells
{
public:
  FindBoundaryCells(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindBoundaryCellsInputValues* inputValues);
  ~FindBoundaryCells() noexcept;

  FindBoundaryCells(const FindBoundaryCells&) = delete;
  FindBoundaryCells(FindBoundaryCells&&) noexcept = delete;
  FindBoundaryCells& operator=(const FindBoundaryCells&) = delete;
  FindBoundaryCells& operator=(FindBoundaryCells&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindBoundaryCellsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
