#include "ItkStitchImages.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ItkStitchImages::name() const
{
  return FilterTraits<ItkStitchImages>::name.str();
}

//------------------------------------------------------------------------------
std::string ItkStitchImages::className() const
{
  return FilterTraits<ItkStitchImages>::className;
}

//------------------------------------------------------------------------------
Uuid ItkStitchImages::uuid() const
{
  return FilterTraits<ItkStitchImages>::uuid;
}

//------------------------------------------------------------------------------
std::string ItkStitchImages::humanName() const
{
  return "Stitch Images (ImageProcessing)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ItkStitchImages::defaultTags() const
{
  return {"#Unsupported", "#Misc"};
}

//------------------------------------------------------------------------------
Parameters ItkStitchImages::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_AttributeMatrixName_Key, "Image Tile Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_StitchedCoordinatesArrayPath_Key, "Image Tile Origins", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_AttributeArrayNamesPath_Key, "Stitched Coordinates Names", "", DataPath{}));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_StitchedVolumeDataContainerName_Key, "Stitched Image Data Container", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_StitchedAttributeMatrixName_Key, "Montage Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_StitchedImagesArrayName_Key, "Montage", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ItkStitchImages::clone() const
{
  return std::make_unique<ItkStitchImages>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ItkStitchImages::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pAttributeMatrixNameValue = filterArgs.value<DataPath>(k_AttributeMatrixName_Key);
  auto pStitchedCoordinatesArrayPathValue = filterArgs.value<DataPath>(k_StitchedCoordinatesArrayPath_Key);
  auto pAttributeArrayNamesPathValue = filterArgs.value<DataPath>(k_AttributeArrayNamesPath_Key);
  auto pStitchedVolumeDataContainerNameValue = filterArgs.value<DataPath>(k_StitchedVolumeDataContainerName_Key);
  auto pStitchedAttributeMatrixNameValue = filterArgs.value<DataPath>(k_StitchedAttributeMatrixName_Key);
  auto pStitchedImagesArrayNameValue = filterArgs.value<DataPath>(k_StitchedImagesArrayName_Key);

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

#if 0
  // Define the OutputActions Object that will hold the actions that would take
  // place if the filter were to execute. This is mainly what would happen to the
  // DataStructure during this filter, i.e., what modificationst to the DataStructure
  // would take place.
  OutputActions actions;
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ItkStitchImagesAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> ItkStitchImages::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pAttributeMatrixNameValue = filterArgs.value<DataPath>(k_AttributeMatrixName_Key);
  auto pStitchedCoordinatesArrayPathValue = filterArgs.value<DataPath>(k_StitchedCoordinatesArrayPath_Key);
  auto pAttributeArrayNamesPathValue = filterArgs.value<DataPath>(k_AttributeArrayNamesPath_Key);
  auto pStitchedVolumeDataContainerNameValue = filterArgs.value<DataPath>(k_StitchedVolumeDataContainerName_Key);
  auto pStitchedAttributeMatrixNameValue = filterArgs.value<DataPath>(k_StitchedAttributeMatrixName_Key);
  auto pStitchedImagesArrayNameValue = filterArgs.value<DataPath>(k_StitchedImagesArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
