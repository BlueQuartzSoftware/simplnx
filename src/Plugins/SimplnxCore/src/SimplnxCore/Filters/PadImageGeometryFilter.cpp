#include "PadImageGeometryFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/PadImageGeometry.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CopyDataObjectAction.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Filter/Actions/DeleteDataAction.hpp"
#include "simplnx/Filter/Actions/RenameDataAction.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/GeometryHelpers.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"
#include "simplnx/Utilities/SamplingUtils.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
PadImageGeometryFilter::PadImageGeometryFilter() = default;

//------------------------------------------------------------------------------
PadImageGeometryFilter::~PadImageGeometryFilter() noexcept = default;

//------------------------------------------------------------------------------
std::string PadImageGeometryFilter::name() const
{
  return FilterTraits<PadImageGeometryFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string PadImageGeometryFilter::className() const
{
  return FilterTraits<PadImageGeometryFilter>::className;
}

//------------------------------------------------------------------------------
Uuid PadImageGeometryFilter::uuid() const
{
  return FilterTraits<PadImageGeometryFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string PadImageGeometryFilter::humanName() const
{
  return "Pad Image Geometry";
}

//------------------------------------------------------------------------------
std::vector<std::string> PadImageGeometryFilter::defaultTags() const
{
  return {className(), "Core", "Generation", "Expand", "Image Geometry", "Create", "Append"};
}

//------------------------------------------------------------------------------
Parameters PadImageGeometryFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_PadXDim_Key, "Pad X Dimension", "Enable padding in the X dimension.", true));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_PadYDim_Key, "Pad Y Dimension", "Enable padding in the Y dimension.", true));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_PadZDim_Key, "Pad Z Dimension", "Enable padding in the Z dimension.", true));

  params.insert(std::make_unique<VectorInt32Parameter>(k_XMinMax_Key, "X Min & Max Padding (in voxels)", "The Min and Max padding for X Axis", std::vector<int32>{0, 0},
                                                       std::vector<std::string>{"X Min", "X Max"}));
  params.insert(std::make_unique<VectorInt32Parameter>(k_YMinMax_Key, "Y Min & Max Padding (in voxels)", "The Min and Max padding for Y Axis", std::vector<int32>{0, 0},
                                                       std::vector<std::string>{"Y Min", "Y Max"}));
  params.insert(std::make_unique<VectorInt32Parameter>(k_ZMinMax_Key, "Z Min & Max Padding (in voxels)", "The Min and Max padding for Z Axis", std::vector<int32>{0, 0},
                                                       std::vector<std::string>{"Z Min", "Z Max"}));
  params.insert(std::make_unique<Int32Parameter>(k_DefaultFillValue_Key, "Default Padding Value", "The value to apply to the added elements in each of the cell data arrays", 0));
  params.insert(std::make_unique<BoolParameter>(k_UpdateOrigin_Key, "Update Origin", "Should the origin of the Image Geometry be updated", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_PerformInPlace_Key, "Perform In Place", "Removes the original Image Geometry after filter is completed", true));

  params.insertSeparator(Parameters::Separator{"Input Image Geometry"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_AttributeMatrixPath_Key, "Cell Data", "The Attribute Matrix that holds the cell data for the geometry", DataPath{}));

  params.insertSeparator(Parameters::Separator{"Output Image Geometry"});
  params.insert(
      std::make_unique<DataGroupCreationParameter>(k_CreatedImageGeometryPath_Key, "Created Image Geometry", "The DataPath to store the created Image Geometry", DataPath({"Padded Image Geometry"})));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_PadXDim_Key, k_XMinMax_Key, true);
  params.linkParameters(k_PadYDim_Key, k_YMinMax_Key, true);
  params.linkParameters(k_PadZDim_Key, k_ZMinMax_Key, true);

  params.linkParameters(k_PerformInPlace_Key, k_CreatedImageGeometryPath_Key, false);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType PadImageGeometryFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer PadImageGeometryFilter::clone() const
{
  return std::make_unique<PadImageGeometryFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult PadImageGeometryFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                               const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto srcImagePath = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);
  auto destImagePath = filterArgs.value<DataPath>(k_CreatedImageGeometryPath_Key);

  auto pXMinMaxValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_XMinMax_Key);
  auto pYMinMaxValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_YMinMax_Key);
  auto pZMinMaxValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_ZMinMax_Key);
  auto pUpdateOriginValue = filterArgs.value<bool>(k_UpdateOrigin_Key);
  auto pAttributeMatrixPathValue = filterArgs.value<DataPath>(k_AttributeMatrixPath_Key);
  auto pPerformInPlace = filterArgs.value<bool>(k_PerformInPlace_Key);
  auto pPadXDim = filterArgs.value<BoolParameter::ValueType>(k_PadXDim_Key);
  auto pPadYDim = filterArgs.value<BoolParameter::ValueType>(k_PadYDim_Key);
  auto pPadZDim = filterArgs.value<BoolParameter::ValueType>(k_PadZDim_Key);

  if(!pPadXDim && !pPadYDim && !pPadZDim)
  {
    return {MakeErrorResult<OutputActions>(-4010, "At least one dimension must be selected to crop!")};
  }

  Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  const auto& srcImageGeom = dataStructure.getDataRefAs<ImageGeom>(srcImagePath);
  auto srcDims = srcImageGeom.getDimensions();
  auto srcOrigin = srcImageGeom.getOrigin();
  auto srcSpacing = srcImageGeom.getSpacing();

  SizeVec3 destGeomDims = srcImageGeom.getDimensions();
  if(pPadXDim)
  {
    destGeomDims[0] += (pXMinMaxValue[0] + pXMinMaxValue[1]);
  }
  if(pPadYDim)
  {
    destGeomDims[1] += (pYMinMaxValue[0] + pYMinMaxValue[1]);
  }
  if(pPadZDim)
  {
    destGeomDims[2] += (pZMinMaxValue[0] + pZMinMaxValue[1]);
  }

  FloatVec3 targetOrigin = srcImageGeom.getOrigin();
  if(pUpdateOriginValue)
  {
    if(pPadXDim)
    {
      targetOrigin[0] -= pXMinMaxValue[0] * srcSpacing[0];
    }

    if(pPadYDim)
    {
      targetOrigin[1] -= pYMinMaxValue[0] * srcSpacing[1];
    }

    if(pPadZDim)
    {
      targetOrigin[2] -= pZMinMaxValue[0] * srcSpacing[2];
    }
  }

  // The ImageGeometryDimensions go from Fastest to Slowest, XYZ.
  std::vector<usize> destArrayDims = {destGeomDims[2], destGeomDims[1], destGeomDims[0]}; // The DataArray shape goes slowest to fastest (ZYX)
  std::vector<DataPath> ignorePaths;                                                      // already copied over so skip these when collecting child paths to finish copying over later

  if(pPerformInPlace)
  {
    // Generate a new name for the current Image Geometry
    auto tempPathVector = srcImagePath.getPathVector();
    std::string tempName = "." + tempPathVector.back();
    tempPathVector.back() = tempName;
    DataPath tempPath(tempPathVector);
    // Rename the current image geometry
    resultOutputActions.value().appendDeferredAction(std::make_unique<RenameDataAction>(srcImagePath, tempName));
    // After the execute function has been done, delete the moved image geometry
    resultOutputActions.value().appendDeferredAction(std::make_unique<DeleteDataAction>(tempPath));

    tempPathVector = srcImagePath.getPathVector();
    tempName = pad_image_geometry::k_TempGeometryName;
    tempPathVector.back() = tempName;
    destImagePath = DataPath({tempPathVector});
  }

  // This section gets the cell attribute matrix for the input Image Geometry and
  // then creates new arrays from each array that is in that attribute matrix. We
  // also push this attribute matrix into the `ignorePaths` variable since we do
  // not need to manually copy these arrays to the destination image geometry
  {
    // Get the name of the Cell Attribute Matrix, so we can use that in the CreateImageGeometryAction
    const AttributeMatrix* selectedCellData = srcImageGeom.getCellData();
    if(selectedCellData == nullptr)
    {
      return {MakeErrorResult<OutputActions>(-4014, fmt::format("'{}' must have cell data attribute matrix", srcImagePath.toString()))};
    }
    std::string cellDataName = selectedCellData->getName();
    ignorePaths.push_back(srcImagePath.createChildPath(cellDataName));

    resultOutputActions.value().appendAction(std::make_unique<CreateImageGeometryAction>(destImagePath, destGeomDims.toContainer<CreateImageGeometryAction::DimensionType>(),
                                                                                         targetOrigin.toContainer<CreateImageGeometryAction::OriginType>(),
                                                                                         srcSpacing.toContainer<CreateImageGeometryAction::SpacingType>(), cellDataName, srcImageGeom.getUnits()));

    // Now loop over each array in the source image geometry's cell attribute matrix and create the corresponding arrays
    // in the destination image geometry's attribute matrix
    DataPath newCellAttributeMatrixPath = destImagePath.createChildPath(cellDataName);
    for(const auto& [identifier, object] : *selectedCellData)
    {
      const auto& srcArray = dynamic_cast<const AbstractDataArray&>(*object);
      DataType dataType = srcArray.getDataType();
      ShapeType componentShape = srcArray.getIDataStoreRef().getComponentShape();
      DataPath dataArrayPath = newCellAttributeMatrixPath.createChildPath(srcArray.getName());
      resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(dataType, destArrayDims, std::move(componentShape), dataArrayPath));
    }

    // Store the preflight updated value(s) into the preflightUpdatedValues vector using the appropriate methods.
    std::string cropOptionsStr = "This filter will pad the image in the following dimension(s):  ";
    cropOptionsStr.append(pPadXDim ? "X" : "");
    cropOptionsStr.append(pPadYDim ? "Y" : "");
    cropOptionsStr.append(pPadZDim ? "Z" : "");
    preflightUpdatedValues.push_back({"Pad Dimensions", cropOptionsStr});

    preflightUpdatedValues.push_back({"Input Geometry Info", nx::core::GeometryHelpers::Description::GenerateGeometryInfo(srcImageGeom.getDimensions(), srcImageGeom.getSpacing(),
                                                                                                                          srcImageGeom.getOrigin(), srcImageGeom.getUnits())});
    preflightUpdatedValues.push_back({"Padded Image Geometry Info", nx::core::GeometryHelpers::Description::GenerateGeometryInfo(
                                                                        destGeomDims, srcSpacing.toContainer<CreateImageGeometryAction::SpacingType>(), targetOrigin, srcImageGeom.getUnits())});
  }

  // This section covers copying the other Attribute Matrix objects from the source geometry
  // to the destination geometry
  auto childPaths = GetAllChildDataPaths(dataStructure, srcImagePath, IDataObject::Type::AbstractDataObject, ignorePaths);
  if(childPaths.has_value())
  {
    for(const auto& childPath : childPaths.value())
    {
      std::string copiedChildName = nx::core::StringUtilities::replace(childPath.toString(), srcImagePath.getTargetName(), destImagePath.getTargetName());
      DataPath copiedChildPath = DataPath::FromString(copiedChildName).value();
      if(dataStructure.getDataAs<BaseGroup>(childPath) != nullptr)
      {
        std::vector<DataPath> allCreatedPaths = {copiedChildPath};
        auto pathsToBeCopied = GetAllChildDataPathsRecursive(dataStructure, childPath);
        if(pathsToBeCopied.has_value())
        {
          for(const auto& sourcePath : pathsToBeCopied.value())
          {
            std::string createdPathName = nx::core::StringUtilities::replace(sourcePath.toString(), srcImagePath.getTargetName(), destImagePath.getTargetName());
            allCreatedPaths.push_back(DataPath::FromString(createdPathName).value());
          }
        }
        resultOutputActions.value().appendAction(std::make_unique<CopyDataObjectAction>(childPath, copiedChildPath, allCreatedPaths));
      }
      else
      {
        resultOutputActions.value().appendAction(std::make_unique<CopyDataObjectAction>(childPath, copiedChildPath, std::vector<DataPath>{copiedChildPath}));
      }
    }
  }

  if(pPerformInPlace)
  {
    resultOutputActions.value().appendDeferredAction(std::make_unique<RenameDataAction>(destImagePath, srcImagePath.getTargetName()));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> PadImageGeometryFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                             const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  PadImageGeometryInputValues inputValues;

  inputValues.SelectedImageGeometryPath = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);
  inputValues.CreatedOutputPath = filterArgs.value<DataPath>(k_CreatedImageGeometryPath_Key);

  inputValues.XMinMax = filterArgs.value<VectorInt32Parameter::ValueType>(k_XMinMax_Key);
  inputValues.YMinMax = filterArgs.value<VectorInt32Parameter::ValueType>(k_YMinMax_Key);
  inputValues.ZMinMax = filterArgs.value<VectorInt32Parameter::ValueType>(k_ZMinMax_Key);
  inputValues.DefaultFillValue = filterArgs.value<int32>(k_DefaultFillValue_Key);
  inputValues.UpdateOrigin = filterArgs.value<bool>(k_UpdateOrigin_Key);
  inputValues.AttributeMatrixPath = filterArgs.value<DataPath>(k_AttributeMatrixPath_Key);
  inputValues.RemoveOriginalGeometry = filterArgs.value<bool>(k_PerformInPlace_Key);
  inputValues.PadInX = filterArgs.value<BoolParameter::ValueType>(k_PadXDim_Key);
  inputValues.PadInY = filterArgs.value<BoolParameter::ValueType>(k_PadYDim_Key);
  inputValues.PadInZ = filterArgs.value<BoolParameter::ValueType>(k_PadZDim_Key);

  return PadImageGeometry(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_AttributeMatrixPathKey = "AttributeMatrixPath";
constexpr StringLiteral k_DefaultFillValueKey = "DefaultFillValue";
constexpr StringLiteral k_UpdateOriginKey = "UpdateOrigin";
constexpr StringLiteral k_XMinMaxKey = "XMinMax";
constexpr StringLiteral k_YMinMaxKey = "YMinMax";
constexpr StringLiteral k_ZMinMaxKey = "ZMinMax";
} // namespace SIMPL
} // namespace

//------------------------------------------------------------------------------
Result<Arguments> PadImageGeometryFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = PadImageGeometryFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionToGeometrySelectionFilterParameterConverter>(args, json, SIMPL::k_AttributeMatrixPathKey, k_SelectedImageGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::AttributeMatrixSelectionFilterParameterConverter>(args, json, SIMPL::k_AttributeMatrixPathKey, k_AttributeMatrixPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<int32>>(args, json, SIMPL::k_DefaultFillValueKey, k_DefaultFillValue_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_UpdateOriginKey, k_UpdateOrigin_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntVec2FilterParameterConverter>(args, json, SIMPL::k_XMinMaxKey, k_XMinMax_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntVec2FilterParameterConverter>(args, json, SIMPL::k_YMinMaxKey, k_YMinMax_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntVec2FilterParameterConverter>(args, json, SIMPL::k_ZMinMaxKey, k_ZMinMax_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}

} // namespace nx::core
