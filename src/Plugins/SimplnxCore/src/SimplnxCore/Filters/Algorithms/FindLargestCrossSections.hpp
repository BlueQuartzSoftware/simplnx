#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT FindLargestCrossSectionsInputValues
{
  ChoicesParameter::ValueType Plane;
  DataPath ImageGeometryPath;
  DataPath FeatureIdsArrayPath;
  DataPath LargestCrossSectionsArrayPath;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT FindLargestCrossSections
{
public:
  FindLargestCrossSections(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindLargestCrossSectionsInputValues* inputValues);
  ~FindLargestCrossSections() noexcept;

  FindLargestCrossSections(const FindLargestCrossSections&) = delete;
  FindLargestCrossSections(FindLargestCrossSections&&) noexcept = delete;
  FindLargestCrossSections& operator=(const FindLargestCrossSections&) = delete;
  FindLargestCrossSections& operator=(FindLargestCrossSections&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindLargestCrossSectionsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
