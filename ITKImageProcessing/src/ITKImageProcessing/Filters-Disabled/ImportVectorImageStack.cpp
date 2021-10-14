#include "ImportVectorImageStack.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/ImportVectorImageStackFilterParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

using namespace complex;

namespace complex
{
std::string ImportVectorImageStack::name() const
{
  return FilterTraits<ImportVectorImageStack>::name.str();
}

Uuid ImportVectorImageStack::uuid() const
{
  return FilterTraits<ImportVectorImageStack>::uuid;
}

std::string ImportVectorImageStack::humanName() const
{
  return "Import Images (3D Vector Stack)";
}

Parameters ImportVectorImageStack::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ImportVectorImageStackFilterParameter>(k_InputFileListInfo_Key, "Input File List", "", {}));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Origin_Key, "Origin", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Spacing_Key, "Spacing", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<BoolParameter>(k_ConvertToGrayscale_Key, "Convert Color To Grayscale", "", false));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_DataContainerName_Key, "Data Container Name", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_CellAttributeMatrixName_Key, "Cell Attribute Matrix Name", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_VectorDataArrayName_Key, "Vector Data Array Name", "", DataPath{}));

  return params;
}

IFilter::UniquePointer ImportVectorImageStack::clone() const
{
  return std::make_unique<ImportVectorImageStack>();
}

Result<OutputActions> ImportVectorImageStack::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pInputFileListInfoValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_InputFileListInfo_Key);
  auto pOriginValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  auto pSpacingValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  auto pConvertToGrayscaleValue = filterArgs.value<bool>(k_ConvertToGrayscale_Key);
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_DataContainerName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellAttributeMatrixName_Key);
  auto pVectorDataArrayNameValue = filterArgs.value<DataPath>(k_VectorDataArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ImportVectorImageStackAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> ImportVectorImageStack::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pInputFileListInfoValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_InputFileListInfo_Key);
  auto pOriginValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  auto pSpacingValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  auto pConvertToGrayscaleValue = filterArgs.value<bool>(k_ConvertToGrayscale_Key);
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_DataContainerName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellAttributeMatrixName_Key);
  auto pVectorDataArrayNameValue = filterArgs.value<DataPath>(k_VectorDataArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
