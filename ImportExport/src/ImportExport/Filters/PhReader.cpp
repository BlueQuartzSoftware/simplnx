#include "PhReader.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string PhReader::name() const
{
  return FilterTraits<PhReader>::name.str();
}

//------------------------------------------------------------------------------
std::string PhReader::className() const
{
  return FilterTraits<PhReader>::className;
}

//------------------------------------------------------------------------------
Uuid PhReader::uuid() const
{
  return FilterTraits<PhReader>::uuid;
}

//------------------------------------------------------------------------------
std::string PhReader::humanName() const
{
  return "Import Ph File (Feature Ids)";
}

//------------------------------------------------------------------------------
std::vector<std::string> PhReader::defaultTags() const
{
  return {"#IO", "#Input", "#Read", "#Import"};
}

//------------------------------------------------------------------------------
Parameters PhReader::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFile_Key, "Input File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Origin_Key, "Origin", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Spacing_Key, "Spacing", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_VolumeDataContainerName_Key, "Data Container", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_CellAttributeMatrixName_Key, "Cell Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_FeatureIdsArrayName_Key, "Feature Ids", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer PhReader::clone() const
{
  return std::make_unique<PhReader>();
}

//------------------------------------------------------------------------------
Result<OutputActions> PhReader::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto pOriginValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  auto pSpacingValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  auto pVolumeDataContainerNameValue = filterArgs.value<DataPath>(k_VolumeDataContainerName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellAttributeMatrixName_Key);
  auto pFeatureIdsArrayNameValue = filterArgs.value<DataPath>(k_FeatureIdsArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<PhReaderAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> PhReader::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto pOriginValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  auto pSpacingValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  auto pVolumeDataContainerNameValue = filterArgs.value<DataPath>(k_VolumeDataContainerName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellAttributeMatrixName_Key);
  auto pFeatureIdsArrayNameValue = filterArgs.value<DataPath>(k_FeatureIdsArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
