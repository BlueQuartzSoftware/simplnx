#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"

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
