#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct SIMPLNXCORE_EXPORT SplitDataArrayByComponentInputValues
{
  DataPath InputArrayPath;
  std::string SplitArraysSuffix;
  std::vector<usize> ExtractComponents;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT SplitDataArrayByComponent
{
public:
  SplitDataArrayByComponent(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, SplitDataArrayByComponentInputValues* inputValues);
  ~SplitDataArrayByComponent() noexcept;

  SplitDataArrayByComponent(const SplitDataArrayByComponent&) = delete;
  SplitDataArrayByComponent(SplitDataArrayByComponent&&) noexcept = delete;
  SplitDataArrayByComponent& operator=(const SplitDataArrayByComponent&) = delete;
  SplitDataArrayByComponent& operator=(SplitDataArrayByComponent&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const SplitDataArrayByComponentInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
