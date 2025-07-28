#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  CreateAttributeMatrixInputValues inputValues;
  inputValues.DataObjectPath = filterArgs.value<DataGroupCreationParameter::ValueType>(data_object_path);
  inputValues.TupleDimensions = filterArgs.value<DynamicTableParameter::ValueType>(tuple_dimensions);
  return CreateAttributeMatrix(dataStructure, messageHandler, shouldCancel, &inputValues)();

*/

namespace nx::core
{

struct SIMPLNXCORE_EXPORT CreateAttributeMatrixInputValues
{
  DataGroupCreationParameter::ValueType DataObjectPath;
  DynamicTableParameter::ValueType TupleDimensions;
};

/**
 * @class CreateAttributeMatrix
 * @brief This algorithm implements support code for the CreateAttributeMatrixFilter
 */

class SIMPLNXCORE_EXPORT CreateAttributeMatrix
{
public:
  CreateAttributeMatrix(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CreateAttributeMatrixInputValues* inputValues);
  ~CreateAttributeMatrix() noexcept;

  CreateAttributeMatrix(const CreateAttributeMatrix&) = delete;
  CreateAttributeMatrix(CreateAttributeMatrix&&) noexcept = delete;
  CreateAttributeMatrix& operator=(const CreateAttributeMatrix&) = delete;
  CreateAttributeMatrix& operator=(CreateAttributeMatrix&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const CreateAttributeMatrixInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
