#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/Dream3dImportParameter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ReadDREAM3DInputValues inputValues;
  inputValues.ImportDataObject = filterArgs.value<Dream3dImportParameter::ValueType>(import_data_object);
  return ReadDREAM3D(dataStructure, messageHandler, shouldCancel, &inputValues)();

*/

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ReadDREAM3DInputValues
{
  Dream3dImportParameter::ValueType ImportDataObject;
};

/**
 * @class ReadDREAM3D
 * @brief This algorithm implements support code for the ReadDREAM3DFilter
 */

class SIMPLNXCORE_EXPORT ReadDREAM3D
{
public:
  ReadDREAM3D(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadDREAM3DInputValues* inputValues);
  ~ReadDREAM3D() noexcept;

  ReadDREAM3D(const ReadDREAM3D&) = delete;
  ReadDREAM3D(ReadDREAM3D&&) noexcept = delete;
  ReadDREAM3D& operator=(const ReadDREAM3D&) = delete;
  ReadDREAM3D& operator=(ReadDREAM3D&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ReadDREAM3DInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
