#include "ResampleImageGeomFilter.hpp"

#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateDataGroupAction.hpp"
#include "complex/Filter/Actions/CreateImageGeometryAction.hpp"
#include "complex/Filter/Actions/DeleteDataAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include "Core/Filters/Algorithms/ResampleImageGeom.hpp"

#include <sstream>

using namespace complex;

namespace
{
std::string GenerateGeometryInfo(const complex::SizeVec3& dims, const complex::FloatVec3& spacing, const complex::FloatVec3& origin)
{
  std::stringstream description;

  description << "X Range: " << origin[0] << " to " << (origin[0] + (dims[0] * spacing[0])) << " (Delta: " << (dims[0] * spacing[0]) << ") " << 0 << "-" << dims[0] - 1 << " Voxels\n";
  description << "Y Range: " << origin[1] << " to " << (origin[1] + (dims[1] * spacing[1])) << " (Delta: " << (dims[1] * spacing[1]) << ") " << 0 << "-" << dims[1] - 1 << " Voxels\n";
  description << "Z Range: " << origin[2] << " to " << (origin[2] + (dims[2] * spacing[2])) << " (Delta: " << (dims[2] * spacing[2]) << ") " << 0 << "-" << dims[2] - 1 << " Voxels\n";
  return description.str();
}
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string ResampleImageGeomFilter::name() const
{
  return FilterTraits<ResampleImageGeomFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ResampleImageGeomFilter::className() const
{
  return FilterTraits<ResampleImageGeomFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ResampleImageGeomFilter::uuid() const
{
  return FilterTraits<ResampleImageGeomFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ResampleImageGeomFilter::humanName() const
{
  return "Resample Data (Image Geometry)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ResampleImageGeomFilter::defaultTags() const
{
  return {"#Sampling", "#Spacing"};
}

//------------------------------------------------------------------------------
Parameters ResampleImageGeomFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Spacing_Key, "New Spacing", "", std::vector<float32>{1.0F, 1.0F, 1.0F}, std::vector<std::string>(3)));
  //  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_RemoveOriginalGeometry_Key, "Remove Original Image Geometry Group", "", true));

  params.insertSeparator(Parameters::Separator{"Input Data"});
  params.insert(
      std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometry_Key, "Selected Image Geometry", "", DataPath{}, GeometrySelectionParameter::AllowedTypes{AbstractGeometry::Type::Image}));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_SelectedCellDataGroup_Key, "Cell Data Group", "Data Group that contains *only* cell data", DataPath{}));

#if 0
  params.insertSeparator(Parameters::Separator{"Renumber Features Input Parameters"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_RenumberFeatures_Key, "Renumber Features", "Renumber feature Ids to ensure continuous values starting at 0.", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Cell Feature Ids", "", DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::int32}));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_CellFeatureAttributeMatrixPath_Key, "Cell Feature Data Group", "", DataPath({"Cell Feature Data"})));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_RenumberFeatures_Key, k_CellFeatureAttributeMatrixPath_Key, true);
  params.linkParameters(k_RenumberFeatures_Key, k_FeatureIdsArrayPath_Key, true);
#endif

  params.insertSeparator(Parameters::Separator{"Created Image Geometry"});
  params.insert(
      std::make_unique<DataGroupCreationParameter>(k_NewDataContainerPath_Key, "Resampled Image Geometry", "Location to store the resampled image geometry", DataPath({"Resampled Image Geometry"})));
  // params.insert(std::make_unique<StringParameter>(k_NewFeaturesName_Key, "New Cell Features Group Name", "Name of the new DataGroup containing updated Voxel Arrays", "Cell Features"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ResampleImageGeomFilter::clone() const
{
  return std::make_unique<ResampleImageGeomFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ResampleImageGeomFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler&, const std::atomic_bool&) const
{
  auto pSpacingValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);

  auto pRemoveOriginalGeometry = filterArgs.value<bool>(k_RemoveOriginalGeometry_Key);
  auto pNewDataContainerPathValue = filterArgs.value<DataPath>(k_NewDataContainerPath_Key);

  auto pRenumberFeaturesValue = filterArgs.value<bool>(k_RenumberFeatures_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCellFeatureAttributeMatrixPathValue = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixPath_Key);

  auto selectedImageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeometry_Key);
  const auto& selectedImageGeom = dataStructure.getDataRefAs<ImageGeom>(selectedImageGeomPath);
  auto cellDataGroupPath = filterArgs.value<DataPath>(k_SelectedCellDataGroup_Key);

  if(pSpacingValue[0] < 0.0F || pSpacingValue[1] < 0.0F || pSpacingValue[2] < 0.0F)
  {
    return {MakeErrorResult<OutputActions>(-11500, fmt::format("Input Spacing has a negative value. {}, {}, {}", pSpacingValue[0], pSpacingValue[1], pSpacingValue[2]))};
  }
  if(pRenumberFeaturesValue)
  {
    const auto* featureIdsArray = dataStructure.getDataAs<Int32Array>(pFeatureIdsArrayPathValue);
    if(featureIdsArray == nullptr)
    {
      return {MakeErrorResult<OutputActions>(-11502, fmt::format("FeatureIds array is not of type Int32"))};
    }
  }

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
  complex::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;
  std::string currentGeometryInfo;
  std::string newGeometryInfo;

  complex::SizeVec3 oldDims = selectedImageGeom.getDimensions();
  complex::FloatVec3 oldSpacing = selectedImageGeom.getSpacing();
  complex::FloatVec3 oldOrigin = selectedImageGeom.getOrigin();

  auto m_XP = static_cast<size_t>(((oldSpacing[0] * static_cast<float>(oldDims[0])) / pSpacingValue[0]));
  auto m_YP = static_cast<size_t>(((oldSpacing[1] * static_cast<float>(oldDims[1])) / pSpacingValue[1]));
  auto m_ZP = static_cast<size_t>(((oldSpacing[2] * static_cast<float>(oldDims[2])) / pSpacingValue[2]));
  if(m_XP == 0)
  {
    m_XP = 1;
  }
  if(m_YP == 0)
  {
    m_YP = 1;
  }
  if(m_ZP == 0)
  {
    m_ZP = 1;
  }

  auto createdImageGeomPath = filterArgs.value<DataPath>(k_NewDataContainerPath_Key);
  resultOutputActions.value().actions.push_back(std::make_unique<CreateImageGeometryAction>(createdImageGeomPath, CreateImageGeometryAction::DimensionType{m_XP, m_YP, m_ZP},
                                                                                            CreateImageGeometryAction::SpacingType{oldOrigin[0], oldOrigin[1], oldOrigin[2]},
                                                                                            CreateImageGeometryAction::OriginType{pSpacingValue[0], pSpacingValue[1], pSpacingValue[2]}));

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // These values should have been updated during the preflightImpl(...) method
  preflightUpdatedValues.push_back({"Input Geometry Info", ::GenerateGeometryInfo(oldDims, oldSpacing, oldOrigin)});
  preflightUpdatedValues.push_back(
      {"Created Image Geometry Info", ::GenerateGeometryInfo(complex::SizeVec3(m_XP, m_YP, m_ZP), complex::FloatVec3(pSpacingValue[0], pSpacingValue[1], pSpacingValue[2]), oldOrigin)});
  const auto* cellDataGroup = dataStructure.getDataAs<DataGroup>(cellDataGroupPath);
  std::vector<std::string> cellDataArrayNames = cellDataGroup->getDataMap().getNames();

  usize originalImageSize = std::accumulate(oldDims.begin(), oldDims.end(), static_cast<usize>(1), std::multiplies<>{});
  std::vector<DataPath> cellDataPaths;
  bool createGroup = true;

  for(const auto& cellArrayPath : cellDataArrayNames)
  {
    const auto& cellArray = dataStructure.getDataRefAs<IDataArray>(cellDataGroupPath.createChildPath(cellArrayPath));
    usize arraySize = cellArray.getNumberOfTuples();
    if(arraySize != originalImageSize)
    {
      return {MakeErrorResult<OutputActions>(
          -1, fmt::format("Selected Array '{}' was size {}, but Image Geometry '{}' expects size {}", cellArray.getName(), arraySize, selectedImageGeom.getName(), originalImageSize))};
    }
    DataPath createdArrayPath = createdImageGeomPath.createChildPath(cellDataGroupPath.getTargetName()).createChildPath(cellArray.getName());

    if(createGroup)
    {
      resultOutputActions.value().actions.push_back(std::make_unique<CreateDataGroupAction>(createdArrayPath.getParent()));
      createGroup = false;
    }

    resultOutputActions.value().actions.push_back(
        std::make_unique<CreateArrayAction>(cellArray.getDataType(), std::vector<usize>{m_ZP, m_YP, m_XP}, cellArray.getIDataStoreRef().getComponentShape(), createdArrayPath));
    cellDataPaths.push_back(cellDataGroupPath.createChildPath(cellArrayPath));
  }

  // Validate that all the arrays in the DataGroup have the same number of tuples.
  if(!dataStructure.validateNumberOfTuples(cellDataPaths))
  {
    return {MakeErrorResult<OutputActions>(-11501, "Tuple count not consistent between input arrays.")};
  }

  if(pRemoveOriginalGeometry)
  {
    resultOutputActions.value().deferredActions.push_back(std::make_unique<DeleteDataAction>(selectedImageGeomPath));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ResampleImageGeomFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter*, const MessageHandler& messageHandler,
                                              const std::atomic_bool& shouldCancel) const
{
  ResampleImageGeomInputValues inputValues;

  inputValues.spacing = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);

  inputValues.inputImageGeometry = filterArgs.value<DataPath>(k_SelectedImageGeometry_Key);
  inputValues.cellDataGroupPath = filterArgs.value<DataPath>(k_SelectedCellDataGroup_Key);

  inputValues.renumberFeatures = filterArgs.value<bool>(k_RenumberFeatures_Key);
  inputValues.featureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.cellFeatureAttributeMatrix = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixPath_Key);

  inputValues.removeOriginalImageGeom = filterArgs.value<bool>(k_RemoveOriginalGeometry_Key);
  inputValues.newDataContainerPath = filterArgs.value<DataPath>(k_NewDataContainerPath_Key);

  return ResampleImageGeom(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
