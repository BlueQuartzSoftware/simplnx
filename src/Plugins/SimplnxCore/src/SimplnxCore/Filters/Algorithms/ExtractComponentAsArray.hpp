#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ExtractComponentAsArrayInputValues
{
  bool MoveComponentsToNewArray;
  bool RemoveComponentsFromArray;
  int32 CompNumber;
  DataPath TempArrayPath;
  DataPath BaseArrayPath;
  DataPath NewArrayPath;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT ExtractComponentAsArray
{
public:
  ExtractComponentAsArray(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ExtractComponentAsArrayInputValues* inputValues);
  ~ExtractComponentAsArray() noexcept;

  ExtractComponentAsArray(const ExtractComponentAsArray&) = delete;
  ExtractComponentAsArray(ExtractComponentAsArray&&) noexcept = delete;
  ExtractComponentAsArray& operator=(const ExtractComponentAsArray&) = delete;
  ExtractComponentAsArray& operator=(ExtractComponentAsArray&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ExtractComponentAsArrayInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
