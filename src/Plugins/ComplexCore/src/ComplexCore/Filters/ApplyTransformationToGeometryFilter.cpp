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
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Filter/Actions/RenameDataAction.hpp"
#include "complex/Filter/Actions/UpdateImageGeomAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/DynamicTableParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"
#include "complex/Utilities/DataGroupUtilities.hpp"
#include "complex/Utilities/FilterUtilities.hpp"
#include "complex/Utilities/GeometryHelpers.hpp"
#include "complex/Utilities/ImageRotationUtilities.hpp"
#include "complex/Utilities/Math/MatrixMath.hpp"
#include "complex/Utilities/ParallelAlgorithmUtilities.hpp"
#include "complex/Utilities/ParallelData3DAlgorithm.hpp"
#include "complex/Utilities/ParallelTaskAlgorithm.hpp"
#include "complex/Utilities/StringUtilities.hpp"

using namespace complex;

namespace
{

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

  params.insertSeparator(Parameters::Separator{"Image Geometry Cell Data Parameters"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseDataArraySelection_Key, "Select Data Arrays", "The data arrays to interpolate onto the transformed geometry", false));
  params.insert(std::make_unique<ChoicesParameter>(k_InterpolationType_Key, "Interpolation Type", "", k_NearestNeighborInterpolationIdx, k_InterpolationChoices));

  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_CellAttributeMatrixPath_Key, "Cell Attribute Matrix", "The path to the Cell level data that should be interpolated", DataPath{}));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_DataArraySelection_Key, "Data Array Selection", "The list of arrays to calculate histogram(s) for",
                                                               MultiArraySelectionParameter::ValueType{}, MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray},
                                                               complex::GetAllNumericTypes()));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseDataArraySelection_Key, k_DataArraySelection_Key, true);
  params.linkParameters(k_UseDataArraySelection_Key, k_InterpolationType_Key, true);
  params.linkParameters(k_UseDataArraySelection_Key, k_CellAttributeMatrixPath_Key, true);
  params.linkParameters(k_UseDataArraySelection_Key, k_DataArraySelection_Key, true);

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
  auto pComputedTransformationMatrixValue = filterArgs.value<DataPath>(k_ComputedTransformationMatrix_Key);
  auto pSelectedGeometryPathValue = filterArgs.value<DataPath>(k_SelectedImageGeometry_Key);

  PreflightResult preflightResult;

  complex::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  auto* igeom = dataStructure.getDataAs<IGeometry>(pSelectedGeometryPathValue);

  if(igeom == nullptr)
  {
    return {MakeErrorResult<OutputActions>(-82000, "Input Geometry must be either ImageGeom or Vertex, Edge, Triangle, Quad, Hexahedron, Tetrahedron.")};
  }

  std::vector<size_t> cDims = {4, 4};

  // Reset the final Transformation Matrix to all Zeros before we fill it with what the user has entered.
  ImageRotationUtilities::Matrix4fR m_TransformationMatrix;

  switch(pTransformationMatrixTypeValue)
  {
  case k_NoTransformIdx: // No-Op
  {
    resultOutputActions.warnings().push_back(Warning{82001, "No transformation has been selected, so this filter will perform no operations"});
  }
  case k_PrecomputedTransformationMatrixIdx: // Transformation matrix from array
  {
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
    m_TransformationMatrix = ImageRotationUtilities::GenerateManualTransformationMatrix(tableData);
    break;
  }
  case k_RotationIdx: // Rotation via axis-angle
  {
    auto pRotationValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Rotation_Key);
    m_TransformationMatrix = ImageRotationUtilities::GenerateRotationTransformationMatrix(pRotationValue);
    break;
  }
  case k_TranslationIdx: // Translation
  {
    auto pTranslationValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Translation_Key);
    m_TransformationMatrix = ImageRotationUtilities::GenerateTranslationTransformationMatrix(pTranslationValue);
    break;
  }
  case k_ScaleIdx: // Scale
  {
    auto pScaleValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Scale_Key);
    m_TransformationMatrix = ImageRotationUtilities::GenerateScaleTransformationMatrix(pScaleValue);

    break;
  }
  default: {
    return {MakeErrorResult<OutputActions>(-82003, "Invalid selection for transformation operation. Valid values are [0,5]")};
  }
  }

  preflightUpdatedValues.push_back({"Generated Transformation Matrix", ImageRotationUtilities::GenerateTransformationMatrixDescription(m_TransformationMatrix)});

  // if ImageGeom was selected to be transformed: This should work because if we didn't pass
  // the earlier test, we should not have gotten to here.
  const ImageGeom* imageGeom = dataStructure.getDataAs<ImageGeom>(pSelectedGeometryPathValue);
  if(imageGeom != nullptr)
  {
    auto pInterpolationTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_InterpolationType_Key);
    auto pUseDataArraySelectionValue = filterArgs.value<bool>(k_UseDataArraySelection_Key);
    auto pDataArraySelectionValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_DataArraySelection_Key);

    // For nearest neighbor we can use any data array as we are only going to copy from a hit cell, no interpolation
    auto pCellAttributeMatrixPath = filterArgs.value<DataPath>(k_CellAttributeMatrixPath_Key);
    auto* attriPtr = dataStructure.getDataAs<AttributeMatrix>(pCellAttributeMatrixPath);
    if(nullptr == attriPtr)
    {
      return {MakeErrorResult<OutputActions>(
          -82006, fmt::format("DataPath '{}' is not an Cell AttributeMatrix. Please select the Cell level AttributeMatrix of the Image Geometry", pCellAttributeMatrixPath.toString()))};
    }
    std::vector<std::string> selectedCellArrayNames = attriPtr->getDataMap().getNames();

    if(pInterpolationTypeValue == k_LinearInterpolationIdx)
    {
      if(pUseDataArraySelectionValue)
      {
        selectedCellArrayNames.clear();
        for(const auto& dataArrayPath : pDataArraySelectionValue)
        {
          selectedCellArrayNames.emplace_back(dataArrayPath.getTargetName());
        }
      }
      else
      {
        resultOutputActions.warnings().push_back(
            Warning{82004, "Linear Interpolation has been selected but no Cell arrays are selected to be interpolated. No data will be transferred to the new geometry"});
      }

      for(const auto& attrArrayName : selectedCellArrayNames)
      {
        const StringArray* strArrayPtr = dataStructure.getDataAs<StringArray>(pCellAttributeMatrixPath.createChildPath(attrArrayName));
        if(nullptr != strArrayPtr)
        {
          resultOutputActions.errors().push_back(
              {82009, fmt::format("Input Type Error, cannot run linear interpolation String arrays. '{}'", pCellAttributeMatrixPath.createChildPath(attrArrayName).toString())});
        }

        const BoolArray* boolArrayPtr = dataStructure.getDataAs<BoolArray>(pCellAttributeMatrixPath.createChildPath(attrArrayName));
        if(nullptr != boolArrayPtr)
        {
          resultOutputActions.errors().push_back(
              {82009, fmt::format("Input Type Error, cannot run linear interpolation Boolean arrays. '{}'", pCellAttributeMatrixPath.createChildPath(attrArrayName).toString())});
        }
      }
    }

    auto rotateArgs = ImageRotationUtilities::CreateRotationArgs(*imageGeom, m_TransformationMatrix);

    // If the user is purely doing a translation then just adjust the origin and be done.
    if(pTransformationMatrixTypeValue == k_TranslationIdx)
    {
      auto pTranslationValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Translation_Key);
      FloatVec3 originVec = {rotateArgs.originalOrigin[0] + pTranslationValue[0], rotateArgs.originalOrigin[1] + pTranslationValue[1], rotateArgs.originalOrigin[2] + pTranslationValue[2]};
      auto spacingVec = imageGeom->getSpacing();
      resultOutputActions.value().actions.push_back(std::make_unique<UpdateImageGeomAction>(originVec, spacingVec, pSelectedGeometryPathValue));
    }
    else // We are Rotating or scaling. we need to create a brand new Image Geometry
    {
      auto srcImagePath = filterArgs.value<DataPath>(k_SelectedImageGeometry_Key);
      DataPath destImagePath = srcImagePath;      // filterArgs.value<DataPath>(k_CreatedImageGeometry_Key);
      auto pRemoveOriginalGeometry = true;        // filterArgs.value<bool>(k_RemoveOriginalGeometry_Key);
      const auto& selectedImageGeom = *imageGeom; // dataStructure.getDataRefAs<ImageGeom>(srcImagePath);

      const std::vector<usize> dims = {static_cast<usize>(rotateArgs.xpNew), static_cast<usize>(rotateArgs.ypNew), static_cast<usize>(rotateArgs.zpNew)};
      const std::vector<float32> spacing = {rotateArgs.xResNew, rotateArgs.yResNew, rotateArgs.zResNew};
      auto origin = selectedImageGeom.getOrigin().toContainer<std::vector<float32>>();
      origin[0] += rotateArgs.xMinNew;
      origin[1] += rotateArgs.yMinNew;
      origin[2] += rotateArgs.zMinNew;

      std::vector<usize> dataArrayShape = {dims[2], dims[1], dims[0]}; // The DataArray shape goes slowest to fastest (ZYX)

      std::vector<DataPath> ignorePaths; // already copied over so skip these when collecting child paths to finish copying over later

      if(pRemoveOriginalGeometry)
      {
        // Generate a new name for the current Image Geometry
        auto tempPathVector = srcImagePath.getPathVector();
        std::string tempName = "." + tempPathVector.back();
        tempPathVector.back() = tempName;
        DataPath tempPath(tempPathVector);
        // Rename the current image geometry
        resultOutputActions.value().deferredActions.push_back(std::make_unique<RenameDataAction>(srcImagePath, tempName));
        // After the execute function has been done, delete the moved image geometry
        resultOutputActions.value().deferredActions.push_back(std::make_unique<DeleteDataAction>(tempPath));

        tempPathVector = srcImagePath.getPathVector();
        tempName = k_TempGeometryName;
        tempPathVector.back() = tempName;
        destImagePath = DataPath({tempPathVector});
      }

      {
        const AttributeMatrix* selectedCellData = selectedImageGeom.getCellData();
        if(selectedCellData == nullptr)
        {
          return {MakeErrorResult<OutputActions>(-5551, fmt::format("'{}' must have cell data attribute matrix", srcImagePath.toString()))};
        }
        std::string cellDataName = selectedCellData->getName();
        ignorePaths.push_back(srcImagePath.createChildPath(cellDataName));

        resultOutputActions.value().actions.push_back(std::make_unique<CreateImageGeometryAction>(destImagePath, dims, origin, spacing, cellDataName));

        // Create the Cell AttributeMatrix in the Destination Geometry
        DataPath newCellAttributeMatrixPath = destImagePath.createChildPath(cellDataName);

        for(const auto& [id, object] : *selectedCellData)
        {
          const auto& srcArray = dynamic_cast<const IDataArray&>(*object);
          DataType dataType = srcArray.getDataType();
          IDataStore::ShapeType componentShape = srcArray.getIDataStoreRef().getComponentShape();
          DataPath dataArrayPath = newCellAttributeMatrixPath.createChildPath(srcArray.getName());
          resultOutputActions.value().actions.push_back(std::make_unique<CreateArrayAction>(dataType, dataArrayShape, std::move(componentShape), dataArrayPath));
        }

        // Store the preflight updated value(s) into the preflightUpdatedValues vector using
        // the appropriate methods.
        // These values should have been updated during the preflightImpl(...) method
        const auto* srcImageGeom = dataStructure.getDataAs<ImageGeom>(srcImagePath);

        preflightUpdatedValues.push_back(
            {"Input Geometry Info", complex::GeometryHelpers::Description::GenerateGeometryInfo(srcImageGeom->getDimensions(), srcImageGeom->getSpacing(), srcImageGeom->getOrigin())});
        preflightUpdatedValues.push_back(
            {"Rotated Image Geometry Info", complex::GeometryHelpers::Description::GenerateGeometryInfo(dims, CreateImageGeometryAction::SpacingType{spacing[0], spacing[1], spacing[2]}, origin)});
      }

      // copy over the rest of the data
      auto childPaths = GetAllChildDataPaths(dataStructure, srcImagePath, DataObject::Type::DataObject, ignorePaths);
      if(childPaths.has_value())
      {
        for(const auto& childPath : childPaths.value())
        {
          std::string copiedChildName = complex::StringUtilities::replace(childPath.toString(), srcImagePath.getTargetName(), destImagePath.getTargetName());
          DataPath copiedChildPath = DataPath::FromString(copiedChildName).value();
          if(dataStructure.getDataAs<BaseGroup>(childPath) != nullptr)
          {
            std::vector<DataPath> allCreatedPaths = {copiedChildPath};
            auto pathsToBeCopied = GetAllChildDataPathsRecursive(dataStructure, childPath);
            if(pathsToBeCopied.has_value())
            {
              for(const auto& sourcePath : pathsToBeCopied.value())
              {
                std::string createdPathName = complex::StringUtilities::replace(sourcePath.toString(), srcImagePath.getTargetName(), destImagePath.getTargetName());
                allCreatedPaths.push_back(DataPath::FromString(createdPathName).value());
              }
            }
            resultOutputActions.value().actions.push_back(std::make_unique<CopyDataObjectAction>(childPath, copiedChildPath, allCreatedPaths));
          }
          else
          {
            resultOutputActions.value().actions.push_back(std::make_unique<CopyDataObjectAction>(childPath, copiedChildPath, std::vector<DataPath>{copiedChildPath}));
          }
        }
      }

      if(pRemoveOriginalGeometry)
      {
        resultOutputActions.value().deferredActions.push_back(std::make_unique<RenameDataAction>(destImagePath, srcImagePath.getTargetName()));
      }
    }
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
  inputValues.UseDataArraySelection = filterArgs.value<bool>(k_UseDataArraySelection_Key);
  inputValues.ManualMatrixTableData = filterArgs.value<DynamicTableParameter::ValueType>(k_ManualTransformationMatrix_Key);
  inputValues.Rotation = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Rotation_Key);
  inputValues.Translation = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Translation_Key);
  inputValues.Scale = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Scale_Key);
  inputValues.SelectedGeometryPath = filterArgs.value<DataPath>(k_SelectedImageGeometry_Key);
  inputValues.CellAttributeMatrixPath = filterArgs.value<DataPath>(k_CellAttributeMatrixPath_Key);
  inputValues.DataArraySelection = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_DataArraySelection_Key);
  inputValues.RemoveOriginalGeometry = true;

  return ApplyTransformationToGeometry(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
