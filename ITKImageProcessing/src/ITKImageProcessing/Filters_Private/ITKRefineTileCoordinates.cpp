#include "ITKRefineTileCoordinates.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/MultiDataContainerSelectionFilterParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKRefineTileCoordinates::name() const
{
  return FilterTraits<ITKRefineTileCoordinates>::name.str();
}

//------------------------------------------------------------------------------
std::string ITKRefineTileCoordinates::className() const
{
  return FilterTraits<ITKRefineTileCoordinates>::className;
}

//------------------------------------------------------------------------------
Uuid ITKRefineTileCoordinates::uuid() const
{
  return FilterTraits<ITKRefineTileCoordinates>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKRefineTileCoordinates::humanName() const
{
  return "ITK Refine Tile Coordinates";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKRefineTileCoordinates::defaultTags() const
{
  return {"#Unsupported", "#Misc"};
}

//------------------------------------------------------------------------------
Parameters ITKRefineTileCoordinates::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Mosaic Layout"});
  params.insert(std::make_unique<VectorInt32Parameter>(k_MontageSize_Key, "Montage Size (Cols, Rows)", "", std::vector<int32>(3), std::vector<std::string>(3)));
  params.insertLinkableParameter(
      std::make_unique<ChoicesParameter>(k_ImportMode_Key, "Import Mode", "", 0, ChoicesParameter::Choices{"Row-By-Row (Comb Order)", "Column-By-Column", "Snake-By-Row", "Snake-By-Column"}));
  params.insert(std::make_unique<Float32Parameter>(k_TileOverlap_Key, "Tile Overlap (Percent)", "", 1.23345f));
  params.insert(std::make_unique<BoolParameter>(k_ApplyRefinedOrigin_Key, "Apply Refined Origin to Geometries", "", false));
  params.insertSeparator(Parameters::Separator{"Input Image Setup"});
  /*[x]*/ params.insert(std::make_unique<MultiDataContainerSelectionFilterParameter>(k_DataContainers_Key, "Select Image Data Containers", "", {}));
  params.insert(std::make_unique<StringParameter>(k_CommonAttributeMatrixName_Key, "Common Attribute Matrix", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_CommonDataArrayName_Key, "Common Data Array", "", "SomeString"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKRefineTileCoordinates::clone() const
{
  return std::make_unique<ITKRefineTileCoordinates>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKRefineTileCoordinates::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                 const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pMontageSizeValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_MontageSize_Key);
  auto pImportModeValue = filterArgs.value<ChoicesParameter::ValueType>(k_ImportMode_Key);
  auto pTileOverlapValue = filterArgs.value<float32>(k_TileOverlap_Key);
  auto pApplyRefinedOriginValue = filterArgs.value<bool>(k_ApplyRefinedOrigin_Key);
  auto pDataContainersValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_DataContainers_Key);
  auto pCommonAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_CommonAttributeMatrixName_Key);
  auto pCommonDataArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_CommonDataArrayName_Key);

  PreflightResult preflightResult;

  complex::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  // If the filter needs to pass back some updated values via a key:value string:string set of values
  // you can declare and update that string here.
  // None found in this filter based on the filter parameters

  // If this filter makes changes to the DataStructure in the form of
  // creating/deleting/moving/renaming DataGroups, Geometries, DataArrays then you
  // will need to use one of the `*Actions` classes located in complex/Filter/Actions
  // to relay that information to the preflight and execute methods. This is done by
  // creating an instance of the Action class and then storing it in the resultOutputActions variable.
  // This is done through a `push_back()` method combined with a `std::move()`. For the
  // newly initiated to `std::move` once that code is executed what was once inside the Action class
  // instance variable is *no longer there*. The memory has been moved. If you try to access that
  // variable after this line you will probably get a crash or have subtle bugs. To ensure that this
  // does not happen we suggest using braces `{}` to scope each of the action's declaration and store
  // so that the programmer is not tempted to use the action instance past where it should be used.
  // You have to create your own Actions class if there isn't something specific for your filter's needs

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // None found based on the filter parameters

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ITKRefineTileCoordinates::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                               const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pMontageSizeValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_MontageSize_Key);
  auto pImportModeValue = filterArgs.value<ChoicesParameter::ValueType>(k_ImportMode_Key);
  auto pTileOverlapValue = filterArgs.value<float32>(k_TileOverlap_Key);
  auto pApplyRefinedOriginValue = filterArgs.value<bool>(k_ApplyRefinedOrigin_Key);
  auto pDataContainersValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_DataContainers_Key);
  auto pCommonAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_CommonAttributeMatrixName_Key);
  auto pCommonDataArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_CommonDataArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
