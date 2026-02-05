#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/MultiPathSelectionParameter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  MoveDataInputValues inputValues;
  inputValues.DestinationParentPath = filterArgs.value<DataGroupSelectionParameter::ValueType>(destination_parent_path);
  inputValues.SourceDataPaths = filterArgs.value<MultiPathSelectionParameter::ValueType>(source_data_paths);
  return MoveData(dataStructure, messageHandler, shouldCancel, &inputValues)();

*/

namespace nx::core
{

struct SIMPLNXCORE_EXPORT MoveDataInputValues
{
  DataGroupSelectionParameter::ValueType DestinationParentPath;
  MultiPathSelectionParameter::ValueType SourceDataPaths;
};

/**
 * @class MoveData
 * @brief This algorithm implements support code for the MoveDataFilter
 */

class SIMPLNXCORE_EXPORT MoveData
{
public:
  MoveData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, MoveDataInputValues* inputValues);
  ~MoveData() noexcept;

  MoveData(const MoveData&) = delete;
  MoveData(MoveData&&) noexcept = delete;
  MoveData& operator=(const MoveData&) = delete;
  MoveData& operator=(MoveData&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const MoveDataInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
