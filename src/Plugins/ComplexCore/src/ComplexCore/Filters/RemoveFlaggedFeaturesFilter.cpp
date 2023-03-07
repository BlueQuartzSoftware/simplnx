#include "RemoveFlaggedFeaturesFilter.hpp"
#include "Algorithms/RemoveFlaggedFeatures.hpp"

#include "complex/Common/TypeTraits.hpp"
#include "complex/DataStructure/AttributeMatrix.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/INeighborList.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string RemoveFlaggedFeaturesFilter::name() const
{
  return FilterTraits<RemoveFlaggedFeaturesFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string RemoveFlaggedFeaturesFilter::className() const
{
  return FilterTraits<RemoveFlaggedFeaturesFilter>::className;
}

//------------------------------------------------------------------------------
Uuid RemoveFlaggedFeaturesFilter::uuid() const
{
  return FilterTraits<RemoveFlaggedFeaturesFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string RemoveFlaggedFeaturesFilter::humanName() const
{
  return "Extract/Remove Flagged Features";
}

//------------------------------------------------------------------------------
std::vector<std::string> RemoveFlaggedFeaturesFilter::defaultTags() const
{
  return {"Processing", "Cleanup"};
}

//------------------------------------------------------------------------------
Parameters RemoveFlaggedFeaturesFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_Functionality_Key, "Selected Operation", "Whether to extract features into new geometry or remove or extract then remove",
                                                                    to_underlying(Functionality::Remove),
                                                                    ChoicesParameter::Choices{"Remove", "Extract", "Extract then Remove"})); // sequence dependent DO NOT REORDER
  params.insert(std::make_unique<BoolParameter>(k_FillRemovedFeatures_Key, "Fill-in Removed Features", "Whether or not to fill in the gaps left by the removed Features", false));

  params.insertSeparator(Parameters::Separator{"Geometry"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ImageGeometry_Key, "Selected Image Geometry", "The target geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Cell Feature Ids", "Specifies to which Feature each cell belongs", DataPath({"CellData", "FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<StringParameter>(k_CreatedImageGeometryPrefix_Key, "Created Image Geometry Prefix",
                                                  "The prefix name for each of new cropped (extracted) geometry \n\nNOTE: a '-' will automatically be added between the prefix and number",
                                                  "Extracted_Feature"));

  params.insertSeparator(Parameters::Separator{"Required Input Cell Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FlaggedFeaturesArrayPath_Key, "Flagged Features", "Specifies whether the Feature will remain in the structure or not", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean, DataType::uint8}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_IgnoredDataArrayPaths_Key, "Attribute Arrays to Ignore", "The list of arrays to ignore when removing flagged features",
                                                               MultiArraySelectionParameter::ValueType{DataPath(), DataPath(), DataPath()},
                                                               MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray}, complex::GetAllDataTypes()));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_Functionality_Key, k_CreatedImageGeometryPrefix_Key, std::make_any<uint64>(to_underlying(Functionality::Extract)));
  params.linkParameters(k_Functionality_Key, k_CreatedImageGeometryPrefix_Key, std::make_any<uint64>(to_underlying(Functionality::ExtractThenRemove)));

  params.linkParameters(k_Functionality_Key, k_FillRemovedFeatures_Key, std::make_any<uint64>(to_underlying(Functionality::Remove)));
  params.linkParameters(k_Functionality_Key, k_FillRemovedFeatures_Key, std::make_any<uint64>(to_underlying(Functionality::ExtractThenRemove)));

  params.linkParameters(k_Functionality_Key, k_IgnoredDataArrayPaths_Key, std::make_any<uint64>(to_underlying(Functionality::Remove)));
  params.linkParameters(k_Functionality_Key, k_IgnoredDataArrayPaths_Key, std::make_any<uint64>(to_underlying(Functionality::ExtractThenRemove)));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer RemoveFlaggedFeaturesFilter::clone() const
{
  return std::make_unique<RemoveFlaggedFeaturesFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult RemoveFlaggedFeaturesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                    const std::atomic_bool& shouldCancel) const
{
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  auto pFlaggedFeaturesArrayPathValue = filterArgs.value<DataPath>(k_FlaggedFeaturesArrayPath_Key);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  if(dataStructure.getDataAs<Int32Array>(pFeatureIdsArrayPathValue) == nullptr)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{-9890, fmt::format("Could not find selected Feature Ids Data Array at path '{}'", pFeatureIdsArrayPathValue.toString())}})};
  }

  if(dataStructure.getDataAs<BoolArray>(pFlaggedFeaturesArrayPathValue) == nullptr && dataStructure.getDataAs<UInt8Array>(pFlaggedFeaturesArrayPathValue) == nullptr)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{-9891, fmt::format("Could not find selected Flagged Features Data Array at path '{}'", pFlaggedFeaturesArrayPathValue.toString())}})};
  }

  DataPath cellFeatureAttributeMatrixPath = pFlaggedFeaturesArrayPathValue.getParent();
  const auto* cellFeatureAM = dataStructure.getDataAs<AttributeMatrix>(cellFeatureAttributeMatrixPath);
  if(cellFeatureAM == nullptr)
  {
    return {nonstd::make_unexpected(std::vector<Error>{
        Error{-9892, fmt::format("Could not find the parent Attribute Matrix for the selected Flagged Features Data Array at path '{}'", pFlaggedFeaturesArrayPathValue.toString())}})};
  }

  std::string warningMsg = "";
  for(const auto& [identifier, object] : *cellFeatureAM)
  {
    if(const auto* srcNeighborListArray = dynamic_cast<const INeighborList*>(object.get()); srcNeighborListArray != nullptr)
    {
      warningMsg += "\n" + cellFeatureAttributeMatrixPath.toString() + "/" + srcNeighborListArray->getName();
    }
  }
  if(!warningMsg.empty())
  {
    resultOutputActions.m_Warnings.push_back(Warning({-11505, fmt::format("This filter modifies the Cell Level Array '{}', the following arrays are of type NeighborList and will not be kept:{}",
                                                                          pFeatureIdsArrayPathValue.toString(), warningMsg)}));
  }

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> RemoveFlaggedFeaturesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel) const
{
  RemoveFlaggedFeaturesInputValues inputValues;

  inputValues.ExtractFeatures = filterArgs.value<ChoicesParameter::ValueType>(k_Functionality_Key);
  inputValues.FillRemovedFeatures = filterArgs.value<bool>(k_FillRemovedFeatures_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputValues.FlaggedFeaturesArrayPath = filterArgs.value<DataPath>(k_FlaggedFeaturesArrayPath_Key);
  inputValues.IgnoredDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IgnoredDataArrayPaths_Key);
  inputValues.ImageGeometryPath = filterArgs.value<DataPath>(k_ImageGeometry_Key);
  inputValues.CreatedImageGeometryPrefix = filterArgs.value<std::string>(k_CreatedImageGeometryPrefix_Key);

  return RemoveFlaggedFeatures(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
