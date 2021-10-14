#include "ItkStitchImages.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"

using namespace complex;

namespace complex
{
std::string ItkStitchImages::name() const
{
  return FilterTraits<ItkStitchImages>::name.str();
}

std::string ItkStitchImages::className() const
{
  return FilterTraits<ItkStitchImages>::className;
}

Uuid ItkStitchImages::uuid() const
{
  return FilterTraits<ItkStitchImages>::uuid;
}

std::string ItkStitchImages::humanName() const
{
  return "Stitch Images (ImageProcessing)";
}

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

IFilter::UniquePointer ItkStitchImages::clone() const
{
  return std::make_unique<ItkStitchImages>();
}

Result<OutputActions> ItkStitchImages::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pAttributeMatrixNameValue = filterArgs.value<DataPath>(k_AttributeMatrixName_Key);
  auto pStitchedCoordinatesArrayPathValue = filterArgs.value<DataPath>(k_StitchedCoordinatesArrayPath_Key);
  auto pAttributeArrayNamesPathValue = filterArgs.value<DataPath>(k_AttributeArrayNamesPath_Key);
  auto pStitchedVolumeDataContainerNameValue = filterArgs.value<DataPath>(k_StitchedVolumeDataContainerName_Key);
  auto pStitchedAttributeMatrixNameValue = filterArgs.value<DataPath>(k_StitchedAttributeMatrixName_Key);
  auto pStitchedImagesArrayNameValue = filterArgs.value<DataPath>(k_StitchedImagesArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ItkStitchImagesAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> ItkStitchImages::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
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
