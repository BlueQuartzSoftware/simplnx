#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT WriteSPParksSitesInputValues
{
  FileSystemPathParameter::ValueType OutputFile;
  DataPath FeatureIdsArrayPath;
  DataPath CellEulerAnglesArrayPath;
  DataPath CellPhasesArrayPath;
  DataPath ImageGeomPath;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT WriteSPParksSites
{
public:
  WriteSPParksSites(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WriteSPParksSitesInputValues* inputValues);
  ~WriteSPParksSites() noexcept;

  WriteSPParksSites(const WriteSPParksSites&) = delete;
  WriteSPParksSites(WriteSPParksSites&&) noexcept = delete;
  WriteSPParksSites& operator=(const WriteSPParksSites&) = delete;
  WriteSPParksSites& operator=(WriteSPParksSites&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const WriteSPParksSitesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
