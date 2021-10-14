#include "ReadH5Ebsd.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/ReadH5EbsdFilterParameter.hpp"

using namespace complex;

namespace complex
{
std::string ReadH5Ebsd::name() const
{
  return FilterTraits<ReadH5Ebsd>::name.str();
}

std::string ReadH5Ebsd::className() const
{
  return FilterTraits<ReadH5Ebsd>::className;
}

Uuid ReadH5Ebsd::uuid() const
{
  return FilterTraits<ReadH5Ebsd>::uuid;
}

std::string ReadH5Ebsd::humanName() const
{
  return "Import H5EBSD File";
}

Parameters ReadH5Ebsd::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ReadH5EbsdFilterParameter>(k_ReadH5Ebsd_Key, "Import H5Ebsd File", "", {}));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_DataContainerName_Key, "Data Container", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_CellAttributeMatrixName_Key, "Cell Attribute Matrix", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Ensemble Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_CellEnsembleAttributeMatrixName_Key, "Cell Ensemble Attribute Matrix", "", DataPath{}));

  return params;
}

IFilter::UniquePointer ReadH5Ebsd::clone() const
{
  return std::make_unique<ReadH5Ebsd>();
}

Result<OutputActions> ReadH5Ebsd::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pReadH5EbsdValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_ReadH5Ebsd_Key);
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_DataContainerName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellAttributeMatrixName_Key);
  auto pCellEnsembleAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellEnsembleAttributeMatrixName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ReadH5EbsdAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> ReadH5Ebsd::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pReadH5EbsdValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_ReadH5Ebsd_Key);
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_DataContainerName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellAttributeMatrixName_Key);
  auto pCellEnsembleAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellEnsembleAttributeMatrixName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
