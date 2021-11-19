#include "ITKRefineTileCoordinates.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
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
  return "ITK::Refine Tile Coordinates";
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
IFilter::PreflightResult ITKRefineTileCoordinates::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
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

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
  complex::Result<OutputActions> resultOutputActions;

  // If your filter is going to pass back some `preflight updated values` then this is where you
  // would create the code to store those values in the appropriate object. Note that we
  // in line creating the pair (NOT a std::pair<>) of Key:Value that will get stored in
  // the std::vector<PreflightValue> object.
  std::vector<PreflightValue> preflightUpdatedValues;

  // If the filter needs to pass back some updated values via a key:value string:string set of values
  // you can declare and update that string here.

  // Assuming this filter did make some structural changes to the DataStructure then store
  // the outputAction into the resultOutputActions object via a std::move().
  // NOTE: That using std::move() means that you can *NOT* use the outputAction variable
  // past this point so let us scope this part which will stop stupid subtle bugs
  // from being introduced. If you have multiple `Actions` classes that you are
  // using such as a CreateDataGroupAction followed by a CreateArrayAction you might
  // want to consider scoping each of those bits of code into their own section of code
  {
    // Replace the "EmptyAction" with one of the prebuilt actions that apply changes
    // to the DataStructure. If none are available then create a new custom Action subclass.
    // If your filter does not make any structural modifications to the DataStructure then
    // you can skip this code.

    auto outputAction = std::make_unique<EmptyAction>();
    resultOutputActions.value().actions.push_back(std::move(outputAction));
  }

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ITKRefineTileCoordinates::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
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
