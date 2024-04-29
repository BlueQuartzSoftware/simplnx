#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct SIMPLNXCORE_EXPORT SplitAttributeArrayInputValues
{
  DataPath InputArrayPath;
  std::string SplitArraysSuffix;
  std::vector<usize> ExtractComponents;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT SplitAttributeArray
{
public:
  SplitAttributeArray(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, SplitAttributeArrayInputValues* inputValues);
  ~SplitAttributeArray() noexcept;

  SplitAttributeArray(const SplitAttributeArray&) = delete;
  SplitAttributeArray(SplitAttributeArray&&) noexcept = delete;
  SplitAttributeArray& operator=(const SplitAttributeArray&) = delete;
  SplitAttributeArray& operator=(SplitAttributeArray&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const SplitAttributeArrayInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
