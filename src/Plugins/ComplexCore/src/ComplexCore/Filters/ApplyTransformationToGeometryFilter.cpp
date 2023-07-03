#include "ApplyTransformationToGeometryFilter.hpp"

#include "ComplexCore/Filters/Algorithms/ApplyTransformationToGeometry.hpp"

#include "complex/Common/Constants.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/StringArray.hpp"
#include "complex/Filter/Actions/CopyDataObjectAction.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "complex/Filter/Actions/CreateImageGeometryAction.hpp"
#include "complex/Filter/Actions/DeleteDataAction.hpp"
#include "complex/Filter/Actions/RenameDataAction.hpp"
#include "complex/Filter/Actions/UpdateImageGeomAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/DynamicTableParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"
#include "complex/Utilities/DataGroupUtilities.hpp"
#include "complex/Utilities/GeometryHelpers.hpp"
#include "complex/Utilities/ImageRotationUtilities.hpp"
#include "complex/Utilities/Math/MatrixMath.hpp"
#include "complex/Utilities/ParallelAlgorithmUtilities.hpp"
#include "complex/Utilities/StringUtilities.hpp"

using namespace complex;

namespace
{
const std::string K_UNKNOWN_PRECOMPUTED_MATRIX_STR = "Precomputed transformation matrix unknown during preflight.";
}

namespace complex
{
//------------------------------------------------------------------------------
std::string ApplyTransformationToGeometryFilter::name() const
{
  return FilterTraits<ApplyTransformationToGeometryFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ApplyTransformationToGeometryFilter::className() const
{
  return FilterTraits<ApplyTransformationToGeometryFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ApplyTransformationToGeometryFilter::uuid() const
{
  return FilterTraits<ApplyTransformationToGeometryFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ApplyTransformationToGeometryFilter::humanName() const
{
  return "Apply Transformation to Geometry";
}

//------------------------------------------------------------------------------
std::vector<std::string> ApplyTransformationToGeometryFilter::defaultTags() const
{
  return {"DREAM3D Review", "Rotation/Transforming"};
}

//------------------------------------------------------------------------------
Parameters ApplyTransformationToGeometryFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter

  params.insertSeparator(Parameters::Separator{"Input Parameters"});

  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_TransformationType_Key, "Transformation Type", "The type of transformation to perform.", k_RotationIdx, k_TransformationChoices));

  DynamicTableInfo tableInfo;
  tableInfo.setColsInfo(DynamicTableInfo::StaticVectorInfo({"1", "2", "3", "4"}));
  tableInfo.setRowsInfo(DynamicTableInfo::StaticVectorInfo({"1", "2", "3", "4"}));
  const DynamicTableInfo::TableDataType defaultTable{{{1.0F, 0.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 1.0F, 0.0F}, {0.0F, 0.0F, 0.0F, 1.0F}}};
  params.insert(std::make_unique<DynamicTableParameter>(k_ManualTransformationMatrix_Key, "Transformation Matrix", "The 4x4 Transformation Matrix", defaultTable, tableInfo));

  params.insert(std::make_unique<VectorFloat32Parameter>(k_Rotation_Key, "Rotation Axis-Angle", "<xyz> w (w in degrees)", std::vector<float32>{0.0F, 0.0F, 1.0F, 90.0F},
                                                         std::vector<std::string>{"x", "y", "z", "w (Deg)"}));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Translation_Key, "Translation", "", std::vector<float32>{0.0F, 0.0F, 0.0F}, std::vector<std::string>{"X", "Y", "Z"}));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Scale_Key, "Scale", "0>= value < 1: Shrink, value = 1: No transform, value > 1.0 enlarge", std::vector<float32>{1.0F, 1.0F, 1.0F},
                                                         std::vector<std::string>{"X", "Y", "Z"}));

  params.insert(std::make_unique<ArraySelectionParameter>(k_ComputedTransformationMatrix_Key, "Transformation Matrix", "", DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::float32}));

  params.insertSeparator({"Input Geometry"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometry_Key, "Selected Geometry", "The target geometry on which to perform the transformation", DataPath{},
                                                             IGeometry::GetAllGeomTypes()));

  params.insertSeparator(Parameters::Separator{"Image Geometry Resampling/Interpolation"});
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_InterpolationType_Key, "Resampling or Interpolation", "", k_NearestNeighborInterpolationIdx, k_InterpolationChoices));

  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_CellAttributeMatrixPath_Key, "Cell Attribute Matrix", "The path to the Cell level data that should be interpolated", DataPath{}));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_TransformationType_Key, k_ComputedTransformationMatrix_Key, k_PrecomputedTransformationMatrixIdx);
  params.linkParameters(k_TransformationType_Key, k_ManualTransformationMatrix_Key, k_ManualTransformationMatrixIdx);
  params.linkParameters(k_TransformationType_Key, k_Rotation_Key, k_RotationIdx);
  params.linkParameters(k_TransformationType_Key, k_Translation_Key, k_TranslationIdx);
  params.linkParameters(k_TransformationType_Key, k_Scale_Key, k_ScaleIdx);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ApplyTransformationToGeometryFilter::clone() const
{
  return std::make_unique<ApplyTransformationToGeometryFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ApplyTransformationToGeometryFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                            const std::atomic_bool& shouldCancel) const
{
  auto pTransformationMatrixTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_TransformationType_Key);
  auto tableData = filterArgs.value<DynamicTableParameter::ValueType>(k_ManualTransformationMatrix_Key);
  auto pComputedTransformationMatrixPath = filterArgs.value<DataPath>(k_ComputedTransformationMatrix_Key);
  auto pSelectedGeometryPathValue = filterArgs.value<DataPath>(k_SelectedImageGeometry_Key);
  auto pCellAttributeMatrixPath = filterArgs.value<DataPath>(k_CellAttributeMatrixPath_Key);

  // PreflightResult preflightResult;

  complex::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  const auto* iGeometryPtr = dataStructure.getDataAs<IGeometry>(pSelectedGeometryPathValue);

  if(iGeometryPtr == nullptr)
  {
    return {MakeErrorResult<OutputActions>(-82000, "Input Geometry must be either ImageGeom or Vertex, Edge, Triangle, Quad, Hexahedron, Tetrahedron.")};
  }

  const std::vector<usize> cDims = {4, 4};

  // Reset the final Transformation Matrix to all Zeros before we fill it with what the user has entered.
  ImageRotationUtilities::Matrix4fR transformationMatrix;
  std::string transformationMatrixDesc;
  transformationMatrix.setIdentity();

  // if ImageGeom was selected to be transformed: This should work because if we didn't pass
  // the earlier test, we should not have gotten to here.
  const ImageGeom* imageGeomPtr = dataStructure.getDataAs<ImageGeom>(pSelectedGeometryPathValue);
  if(imageGeomPtr != nullptr)
  {
    switch(pTransformationMatrixTypeValue)
    {
    case k_NoTransformIdx: // No-Op
    {
      resultOutputActions.warnings().push_back(Warning{82001, "No transformation has been selected, so this filter will perform no operations"});
      transformationMatrixDesc = "No transformation matrix selected.";
    }
    case k_PrecomputedTransformationMatrixIdx: // Transformation matrix from array
    {
      const Float32Array* precomputedMatrixPtr = dataStructure.getDataAs<Float32Array>(pComputedTransformationMatrixPath);
      if(nullptr == precomputedMatrixPtr)
      {
        return {
            MakeErrorResult<OutputActions>(-82010, fmt::format("Precomputed transformation matrix must have a valid path. Invalid path given: '{}'", pComputedTransformationMatrixPath.toString()))};
      }
      transformationMatrixDesc = K_UNKNOWN_PRECOMPUTED_MATRIX_STR;
      break;
    }
    case k_ManualTransformationMatrixIdx: // Manual transformation matrix
    {
      const usize numTableRows = tableData.size();
      const usize numTableCols = tableData[0].size();
      if(numTableRows != 4)
      {
        return {MakeErrorResult<OutputActions>(-82002, "Manually entered transformation matrix must have exactly 4 rows")};
      }
      if(numTableCols != 4)
      {
        return {MakeErrorResult<OutputActions>(-82006, "Manually entered transformation matrix must have exactly 4 columns")};
      }
      transformationMatrix = ImageRotationUtilities::GenerateManualTransformationMatrix(tableData);
      transformationMatrixDesc = ImageRotationUtilities::GenerateTransformationMatrixDescription(transformationMatrix);
      break;
    }
    case k_RotationIdx: // Rotation via axis-angle
    {
      auto pRotationValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Rotation_Key);
      auto origin = imageGeomPtr->getOrigin();
      ImageRotationUtilities::Matrix4fR translationToGlobalOriginMat = ImageRotationUtilities::GenerateTranslationTransformationMatrix({-origin[0], -origin[1], -origin[2]});
      transformationMatrix = ImageRotationUtilities::GenerateRotationTransformationMatrix(pRotationValue);
      ImageRotationUtilities::Matrix4fR translationFromGlobalOriginMat = ImageRotationUtilities::GenerateTranslationTransformationMatrix({origin[0], origin[1], origin[2]});
      transformationMatrix = translationFromGlobalOriginMat * transformationMatrix * translationToGlobalOriginMat;
      transformationMatrixDesc = ImageRotationUtilities::GenerateTransformationMatrixDescription(transformationMatrix);
      break;
    }
    case k_TranslationIdx: // Translation
    {
      auto pTranslationValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Translation_Key);
      transformationMatrix = ImageRotationUtilities::GenerateTranslationTransformationMatrix(pTranslationValue);
      transformationMatrixDesc = ImageRotationUtilities::GenerateTransformationMatrixDescription(transformationMatrix);
      break;
    }
    case k_ScaleIdx: // Scale
    {
      auto pScaleValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Scale_Key);
      auto origin = imageGeomPtr->getOrigin();
      ImageRotationUtilities::Matrix4fR translationToGlobalOriginMat = ImageRotationUtilities::GenerateTranslationTransformationMatrix({-origin[0], -origin[1], -origin[2]});
      transformationMatrix = ImageRotationUtilities::GenerateScaleTransformationMatrix(pScaleValue);
      ImageRotationUtilities::Matrix4fR translationFromGlobalOriginMat = ImageRotationUtilities::GenerateTranslationTransformationMatrix({origin[0], origin[1], origin[2]});
      transformationMatrix = translationFromGlobalOriginMat * transformationMatrix * translationToGlobalOriginMat;
      transformationMatrixDesc = ImageRotationUtilities::GenerateTransformationMatrixDescription(transformationMatrix);
      break;
    }
    default: {
      return {MakeErrorResult<OutputActions>(-82003, "Invalid selection for transformation operation. Valid values are [0,5]")};
    }
    }

    preflightUpdatedValues.push_back({"Generated Transformation Matrix", transformationMatrixDesc});

    auto pInterpolationTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_InterpolationType_Key);
    // auto pDataArraySelectionValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_DataArraySelection_Key);

    // For nearest neighbor we can use any data array as we are only going to copy from a hit cell, no interpolation
    const auto* srcCellAttrMatrixPtr = dataStructure.getDataAs<AttributeMatrix>(pCellAttributeMatrixPath);
    if(nullptr == srcCellAttrMatrixPtr)
    {
      return {MakeErrorResult<OutputActions>(
          -82006, fmt::format("DataPath '{}' is not an Cell AttributeMatrix. Please select the Cell level AttributeMatrix of the Image Geometry", pCellAttributeMatrixPath.toString()))};
    }
    std::vector<std::string> selectedCellArrayNames = srcCellAttrMatrixPtr->getDataMap().getNames();

    if(pInterpolationTypeValue == k_LinearInterpolationIdx)
    {
      // Remove all the DataArrays from the src Cell AttributeMatrix and substitute with just what the user wants to interpolate on.
      selectedCellArrayNames.clear();
      for(const auto& arrayName : srcCellAttrMatrixPtr->getDataMap().getNames())
      {
        const DataPath dataArrayPath = pCellAttributeMatrixPath.createChildPath(arrayName);
        const StringArray* strArrayPtr = dataStructure.getDataAs<StringArray>(dataArrayPath);
        if(nullptr != strArrayPtr)
        {
          resultOutputActions.warnings().push_back(
              Warning{82009, fmt::format("DataArray '{}' will be deleted from final transformed geometry. Cannot perform interpolation on String Arrays", dataArrayPath.toString())});
          continue;
        }

        const BoolArray* boolArrayPtr = dataStructure.getDataAs<BoolArray>(dataArrayPath);
        if(nullptr != boolArrayPtr)
        {
          resultOutputActions.warnings().push_back(
              Warning{82010, fmt::format("DataArray '{}' will be deleted from final transformed geometry. Cannot perform interpolation on Bool Arrays", dataArrayPath.toString())});
          continue;
        }

        const INeighborList* neighborListPtr = dataStructure.getDataAs<INeighborList>(dataArrayPath);
        if(nullptr != neighborListPtr)
        {
          resultOutputActions.warnings().push_back(
              Warning{82011, fmt::format("DataArray '{}' will be deleted from final transformed geometry. Cannot perform interpolation on NeighborList Arrays", dataArrayPath.toString())});
          continue;
        }
        selectedCellArrayNames.emplace_back(arrayName);
      }
    }

    if(pTransformationMatrixTypeValue == k_TranslationIdx)
    {
      // If the user is purely doing a translation then just adjust the origin and be done.
      auto pTranslationValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Translation_Key);
      FloatVec3 originVec = imageGeomPtr->getOrigin();
      originVec = {originVec[0] + pTranslationValue[0], originVec[1] + pTranslationValue[1], originVec[2] + pTranslationValue[2]};
      auto spacingVec = imageGeomPtr->getSpacing();
      resultOutputActions.value().appendAction(std::make_unique<UpdateImageGeomAction>(originVec, spacingVec, pSelectedGeometryPathValue));
    }
    else if(pTransformationMatrixTypeValue == k_ScaleIdx)
    {
      // If the user is purely doing a scaling then just adjust the spacing and be done.
      auto pScaleValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Scale_Key);
      FloatVec3 spacingVec = imageGeomPtr->getSpacing();
      spacingVec = {spacingVec[0] * pScaleValue[0], spacingVec[1] * pScaleValue[1], spacingVec[2] * pScaleValue[2]};
      auto originVec = imageGeomPtr->getOrigin();
      resultOutputActions.value().appendAction(std::make_unique<UpdateImageGeomAction>(originVec, spacingVec, pSelectedGeometryPathValue));
    }
    else // We are Rotating or scaling, manual transformation or precomputed. we need to create a brand new Image Geometry
    {
      auto rotateArgs = ImageRotationUtilities::CreateRotationArgs(*imageGeomPtr, transformationMatrix);

      auto srcImagePath = filterArgs.value<DataPath>(k_SelectedImageGeometry_Key);
      DataPath destImagePath = srcImagePath;         // filterArgs.value<DataPath>(k_CreatedImageGeometry_Key);
      auto pRemoveOriginalGeometry = true;           // filterArgs.value<bool>(k_RemoveOriginalGeometry_Key);
      const auto& selectedImageGeom = *imageGeomPtr; // dataStructure.getDataRefAs<ImageGeom>(srcImagePath);

      const std::vector<usize> dims = {static_cast<usize>(rotateArgs.xpNew), static_cast<usize>(rotateArgs.ypNew), static_cast<usize>(rotateArgs.zpNew)};
      const std::vector<float32> spacing = {rotateArgs.xResNew, rotateArgs.yResNew, rotateArgs.zResNew};
      auto origin = selectedImageGeom.getOrigin().toContainer<std::vector<float32>>();
      origin[0] = rotateArgs.xMinNew;
      origin[1] = rotateArgs.yMinNew;
      origin[2] = rotateArgs.zMinNew;

      if(pRemoveOriginalGeometry)
      {
        // Create an Image Geometry name with a "." as a prefix to the original Image Geometry Name
        std::vector<std::string> tempPathVector = srcImagePath.getPathVector();
        tempPathVector.back() = "." + tempPathVector.back();
        destImagePath = DataPath({tempPathVector});
      }

      std::vector<usize> const dataArrayShape = {dims[2], dims[1], dims[0]}; // The DataArray shape goes slowest to fastest (ZYX), opposite of ImageGeometry dimensions

      std::vector<DataPath> ignorePaths; // already copied over so skip these when collecting child paths to finish copying over later

      {
        const AttributeMatrix* selectedCellDataPtr = selectedImageGeom.getCellData();
        if(selectedCellDataPtr == nullptr)
        {
          return {MakeErrorResult<OutputActions>(-5551, fmt::format("'{}' must have cell data attribute matrix", srcImagePath.toString()))};
        }
        const std::string cellDataName = selectedCellDataPtr->getName();
        ignorePaths.push_back(srcImagePath.createChildPath(cellDataName)); // This is needed so that we don't attempt to copy it later on
        // Create the new Image Geometry
        resultOutputActions.value().appendAction(std::make_unique<CreateImageGeometryAction>(destImagePath, dims, origin, spacing, cellDataName));

        // Create a DataPath object that points to the Cell AttributeMatrix in the new ImageGeometry
        const DataPath targetCellAttrMatrix = destImagePath.createChildPath(cellDataName);

        // Create the DataArrays in the target Cell Attribute Matrix, based on the interpolation type
        for(const auto& cellArrayName : selectedCellArrayNames)
        {
          const DataPath srcCellArrayDataPath = srcImagePath.createChildPath(cellDataName).createChildPath(cellArrayName);
          const auto& srcArray = dataStructure.getDataRefAs<IDataArray>(srcCellArrayDataPath);
          const IDataStore::ShapeType componentShape = srcArray.getIDataStoreRef().getComponentShape();
          const std::string dataStoreFormat = srcArray.getDataFormat();
          resultOutputActions.value().appendAction(
              std::make_unique<CreateArrayAction>(srcArray.getDataType(), dataArrayShape, componentShape, targetCellAttrMatrix.createChildPath(srcArray.getName()), dataStoreFormat));
        }

        // Store the preflight updated value(s) into the preflightUpdatedValues vector using
        // the appropriate methods.
        // These values should have been updated during the preflightImpl(...) method
        const auto* srcImageGeomPtr = dataStructure.getDataAs<ImageGeom>(srcImagePath);

        preflightUpdatedValues.push_back(
            {"Input Geometry Info", complex::GeometryHelpers::Description::GenerateGeometryInfo(srcImageGeomPtr->getDimensions(), srcImageGeomPtr->getSpacing(), srcImageGeomPtr->getOrigin())});

        if(pTransformationMatrixTypeValue == k_PrecomputedTransformationMatrixIdx)
        {
          preflightUpdatedValues.push_back({"Transformed Image Geometry Info", K_UNKNOWN_PRECOMPUTED_MATRIX_STR});
        }
        else
        {
          preflightUpdatedValues.push_back({"Transformed Image Geometry Info",
                                            complex::GeometryHelpers::Description::GenerateGeometryInfo(dims, CreateImageGeometryAction::SpacingType{spacing[0], spacing[1], spacing[2]}, origin)});
        }
      }

      // copy over the rest of the data from the src Image Geometry into the Target Image Geometry
      auto childPaths = GetAllChildDataPaths(dataStructure, srcImagePath, DataObject::Type::DataObject, ignorePaths);
      if(childPaths.has_value())
      {
        for(const auto& childPath : childPaths.value())
        {
          const std::string copiedChildName = complex::StringUtilities::replace(childPath.toString(), srcImagePath.getTargetName(), destImagePath.getTargetName());
          const DataPath copiedChildPath = DataPath::FromString(copiedChildName).value();
          if(dataStructure.getDataAs<BaseGroup>(childPath) != nullptr)
          {
            std::vector<DataPath> allCreatedPaths = {copiedChildPath};
            auto pathsToBeCopied = GetAllChildDataPathsRecursive(dataStructure, childPath);
            if(pathsToBeCopied.has_value())
            {
              for(const auto& sourcePath : pathsToBeCopied.value())
              {
                const std::string createdPathName = complex::StringUtilities::replace(sourcePath.toString(), srcImagePath.getTargetName(), destImagePath.getTargetName());
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

      if(pRemoveOriginalGeometry)
      {
        // After the execute function has been done, delete the original image geometry
        resultOutputActions.value().appendDeferredAction(std::make_unique<DeleteDataAction>(srcImagePath));
        // Rename the target Image Geometry (the one that just got created) to the original image geometry's name
        resultOutputActions.value().appendDeferredAction(std::make_unique<RenameDataAction>(destImagePath, srcImagePath.getTargetName()));
      }
    }
  }
  else
  {
    // An image geometry was not chosen, so throw a warning communicating to the user that the cell attribute matrix will not be used
    auto warning = Warning{-5555, fmt::format("The Selected Geometry is not an image geometry, so the Attribute Matrix '{}' will not be used when applying this transformation.",
                                              pCellAttributeMatrixPath.getTargetName())};
    resultOutputActions.warnings().push_back(warning);
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ApplyTransformationToGeometryFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                          const std::atomic_bool& shouldCancel) const
{
  ApplyTransformationToGeometryInputValues inputValues;

  inputValues.TransformationSelection = filterArgs.value<ChoicesParameter::ValueType>(k_TransformationType_Key);
  inputValues.InterpolationSelection = filterArgs.value<ChoicesParameter::ValueType>(k_InterpolationType_Key);
  inputValues.ComputedTransformationMatrix = filterArgs.value<DataPath>(k_ComputedTransformationMatrix_Key);
  inputValues.ManualMatrixTableData = filterArgs.value<DynamicTableParameter::ValueType>(k_ManualTransformationMatrix_Key);
  inputValues.Rotation = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Rotation_Key);
  inputValues.Translation = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Translation_Key);
  inputValues.Scale = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Scale_Key);
  inputValues.SelectedGeometryPath = filterArgs.value<DataPath>(k_SelectedImageGeometry_Key);
  inputValues.CellAttributeMatrixPath = filterArgs.value<DataPath>(k_CellAttributeMatrixPath_Key);
  inputValues.RemoveOriginalGeometry = true;

  return ApplyTransformationToGeometry(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
