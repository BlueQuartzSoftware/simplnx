#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  CreateFeatureArrayFromElementArrayInputValues inputValues;
  inputValues.CellFeatureAttributeMatrixPath = filterArgs.value<AttributeMatrixSelectionParameter::ValueType>(cell_feature_attribute_matrix_path);
  inputValues.CreatedArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(created_array_name);
  inputValues.FeatureIdsPath = filterArgs.value<ArraySelectionParameter::ValueType>(feature_ids_path);
  inputValues.SelectedCellArrayPath = filterArgs.value<ArraySelectionParameter::ValueType>(selected_cell_array_path);
  return CreateFeatureArrayFromElementArray(dataStructure, messageHandler, shouldCancel, &inputValues)();

*/

namespace nx::core
{

struct SIMPLNXCORE_EXPORT CreateFeatureArrayFromElementArrayInputValues
{
  AttributeMatrixSelectionParameter::ValueType CellFeatureAttributeMatrixPath;
  DataObjectNameParameter::ValueType CreatedArrayName;
  ArraySelectionParameter::ValueType FeatureIdsPath;
  ArraySelectionParameter::ValueType SelectedCellArrayPath;
};

/**
 * @class CreateFeatureArrayFromElementArray
 * @brief This algorithm implements support code for the CreateFeatureArrayFromElementArrayFilter
 */

class SIMPLNXCORE_EXPORT CreateFeatureArrayFromElementArray
{
public:
  CreateFeatureArrayFromElementArray(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                     CreateFeatureArrayFromElementArrayInputValues* inputValues);
  ~CreateFeatureArrayFromElementArray() noexcept;

  CreateFeatureArrayFromElementArray(const CreateFeatureArrayFromElementArray&) = delete;
  CreateFeatureArrayFromElementArray(CreateFeatureArrayFromElementArray&&) noexcept = delete;
  CreateFeatureArrayFromElementArray& operator=(const CreateFeatureArrayFromElementArray&) = delete;
  CreateFeatureArrayFromElementArray& operator=(CreateFeatureArrayFromElementArray&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const CreateFeatureArrayFromElementArrayInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
