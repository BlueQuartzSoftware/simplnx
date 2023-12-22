#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ConvertDataInputValues
{
  DataType ScalarType;
  DataPath SelectedArrayPath;
  DataPath OutputArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SIMPLNXCORE_EXPORT ConvertData
{
public:
  ConvertData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ConvertDataInputValues* inputValues);
  ~ConvertData() noexcept;

  ConvertData(const ConvertData&) = delete;
  ConvertData(ConvertData&&) noexcept = delete;
  ConvertData& operator=(const ConvertData&) = delete;
  ConvertData& operator=(ConvertData&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ConvertDataInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
