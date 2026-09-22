#include "simplnx/Utilities/Geometry/GeometryTransformation.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/BaseGroup.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry0D.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/INeighborList.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Filter/Actions/CopyDataObjectAction.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Filter/Actions/DeleteDataAction.hpp"
#include "simplnx/Filter/Actions/RenameDataAction.hpp"
#include "simplnx/Filter/Actions/UpdateImageGeomAction.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/GeometryHelpers.hpp"
#include "simplnx/Utilities/ImageRotationUtilities.hpp"
#include "simplnx/Utilities/ParallelAlgorithmUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <sstream>

using namespace nx::core;

namespace
{
const std::string k_UnknownPrecomputedMatrixDescription = "Precomputed transformation matrix unknown during preflight.";
}

IFilter::PreflightResult nx::core::PreflightGeometryTransformation(const DataStructure& dataStructure, const ApplyTransformationToGeometryInputValues& inputValues)
{
  const auto pTransformationMatrixTypeValue = inputValues.TransformationSelection;
  const auto& tableData = inputValues.ManualMatrixTableData;
  const DataPath& pComputedTransformationMatrixPath = inputValues.ComputedTransformationMatrix;
  const DataPath& pSelectedGeometryPathValue = inputValues.SelectedGeometryPath;
  const DataPath& pCellAttributeMatrixPath = inputValues.CellAttributeMatrixPath;
  const bool pTranslateGeometryToGlobalOrigin = inputValues.TranslateGeometryToGlobalOrigin;
  const bool saveTransform = inputValues.SaveTransformMatrix;
  const DataPath& transformMatrixDataPath = inputValues.TransformMatrixPath;

  Result<OutputActions> resultOutputActions;

  std::vector<IFilter::PreflightValue> preflightUpdatedValues;

  const ShapeType cDims = {4, 4};

  // Reset the final Transformation Matrix to all Zeros before we fill it with what the user has entered.
  ImageRotationUtilities::Matrix4fR transformationMatrix;
  std::string transformationMatrixDesc;
  transformationMatrix.setIdentity();

  // if ImageGeom was selected to be transformed: This should work because if we didn't pass
  // the earlier test, we should not have gotten to here.
  const auto* imageGeomPtr = dataStructure.getDataAs<ImageGeom>(pSelectedGeometryPathValue);
  if(imageGeomPtr != nullptr)
  {
    auto origin = imageGeomPtr->getOrigin();
    const ImageRotationUtilities::Matrix4fR translationToGlobalOriginMat = ImageRotationUtilities::GenerateTranslationTransformationMatrix({-origin[0], -origin[1], -origin[2]});
    const ImageRotationUtilities::Matrix4fR translationFromGlobalOriginMat = ImageRotationUtilities::GenerateTranslationTransformationMatrix({origin[0], origin[1], origin[2]});

    switch(pTransformationMatrixTypeValue)
    {
    case detail::k_NoTransformIdx: // No-Op
    {
      resultOutputActions.warnings().push_back(Warning{82001, "No transformation has been selected. This filter will NOT modify any data."});
      transformationMatrixDesc = "No transformation matrix selected.";
      break;
    }
    case detail::k_PrecomputedTransformationMatrixIdx: // Transformation matrix from array
    {
      const auto* precomputedMatrixPtr = dataStructure.getDataAs<Float32Array>(pComputedTransformationMatrixPath);
      if(nullptr == precomputedMatrixPtr)
      {
        return {
            MakeErrorResult<OutputActions>(-82010, fmt::format("Precomputed transformation matrix must have a valid path. Invalid path given: '{}'", pComputedTransformationMatrixPath.toString()))};
      }
      auto totalElements = precomputedMatrixPtr->getNumberOfTuples() * precomputedMatrixPtr->getNumberOfComponents();
      if(totalElements != 16)
      {
        return {MakeErrorResult<OutputActions>(
            -82019, fmt::format("Precomputed transformation matrix at path '{}' has {} total elements ({} tuples * {} components), but it MUST have 16 total elements.",
                                pComputedTransformationMatrixPath.toString(), totalElements, precomputedMatrixPtr->getNumberOfTuples(), precomputedMatrixPtr->getNumberOfComponents()))};
      }
      transformationMatrixDesc = k_UnknownPrecomputedMatrixDescription;
      break;
    }
    case detail::k_ManualTransformationMatrixIdx: // Manual transformation matrix
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
      if(pTranslateGeometryToGlobalOrigin)
      {
        transformationMatrix = translationFromGlobalOriginMat * transformationMatrix * translationToGlobalOriginMat;
      }
      transformationMatrixDesc = ImageRotationUtilities::GenerateTransformationMatrixDescription(transformationMatrix);
      break;
    }
    case detail::k_RotationIdx: // Rotation via axis-angle
    {
      auto pRotationValue = inputValues.Rotation;
      transformationMatrix = ImageRotationUtilities::GenerateRotationTransformationMatrix(pRotationValue);
      if(pTranslateGeometryToGlobalOrigin)
      {
        transformationMatrix = translationFromGlobalOriginMat * transformationMatrix * translationToGlobalOriginMat;
      }
      transformationMatrixDesc = ImageRotationUtilities::GenerateTransformationMatrixDescription(transformationMatrix);
      break;
    }
    case detail::k_TranslationIdx: // Translation
    {
      auto pTranslationValue = inputValues.Translation;
      transformationMatrix = ImageRotationUtilities::GenerateTranslationTransformationMatrix(pTranslationValue);
      if(pTranslateGeometryToGlobalOrigin)
      {
        transformationMatrix = translationFromGlobalOriginMat * transformationMatrix * translationToGlobalOriginMat;
      }
      transformationMatrixDesc = ImageRotationUtilities::GenerateTransformationMatrixDescription(transformationMatrix);
      break;
    }
    case detail::k_ScaleIdx: // Scale
    {
      auto pScaleValue = inputValues.Scale;
      transformationMatrix = ImageRotationUtilities::GenerateScaleTransformationMatrix(pScaleValue);
      if(pTranslateGeometryToGlobalOrigin)
      {
        transformationMatrix = translationFromGlobalOriginMat * transformationMatrix * translationToGlobalOriginMat;
      }
      transformationMatrixDesc = ImageRotationUtilities::GenerateTransformationMatrixDescription(transformationMatrix);
      break;
    }
    }

    preflightUpdatedValues.push_back({"Generated Transformation Matrix", transformationMatrixDesc});

    std::stringstream errorMessage;
    errorMessage << "You have selected to transform an 'Image Geometry', please correct the following issues:\n";
    bool imageGeomInterpolationError = false;

    auto pInterpolationTypeValue = inputValues.InterpolationSelection;
    if(pInterpolationTypeValue == detail::k_NoInterpolationIdx)
    {
      errorMessage << "* Select either 'Nearest Neighbor Resampling' or 'Linear Interpolation' from the 'Image Geometry Resampling/Interpolation' parameter section.\n";
      imageGeomInterpolationError = true;
    }

    const auto* srcCellAttrMatrixPtr = dataStructure.getDataAs<AttributeMatrix>(pCellAttributeMatrixPath);
    if(nullptr == srcCellAttrMatrixPtr)
    {
      errorMessage << "* Select the Image Geometry's cell level Attribute Matrix. This will contain all the data that will be interpolated onto the new Image Geometry.";
      imageGeomInterpolationError = true;
    }
    if(imageGeomInterpolationError)
    {
      return {MakeErrorResult<OutputActions>(-82006, errorMessage.str())};
    }

    std::vector<std::string> selectedCellArrayNames = srcCellAttrMatrixPtr->getDataMap().getNames();

    if(pInterpolationTypeValue == detail::k_LinearInterpolationIdx)
    {
      // Remove all the DataArrays from the src Cell AttributeMatrix and substitute with just what the user wants to interpolate on.
      selectedCellArrayNames.clear();
      for(const auto& arrayName : srcCellAttrMatrixPtr->getDataMap().getNames())
      {
        const DataPath dataArrayPath = pCellAttributeMatrixPath.createChildPath(arrayName);
        const auto* strArrayPtr = dataStructure.getDataAs<StringArray>(dataArrayPath);
        if(nullptr != strArrayPtr)
        {
          resultOutputActions.warnings().push_back(
              Warning{82009, fmt::format("DataArray '{}' will be deleted from final transformed geometry. Cannot perform interpolation on String Arrays", dataArrayPath.toString())});
          continue;
        }

        const auto* boolArrayPtr = dataStructure.getDataAs<BoolArray>(dataArrayPath);
        if(nullptr != boolArrayPtr)
        {
          resultOutputActions.warnings().push_back(
              Warning{82010, fmt::format("DataArray '{}' will be deleted from final transformed geometry. Cannot perform interpolation on Bool Arrays", dataArrayPath.toString())});
          continue;
        }

        const auto* neighborListPtr = dataStructure.getDataAs<INeighborList>(dataArrayPath);
        if(nullptr != neighborListPtr)
        {
          resultOutputActions.warnings().push_back(
              Warning{82011, fmt::format("DataArray '{}' will be deleted from final transformed geometry. Cannot perform interpolation on NeighborList Arrays", dataArrayPath.toString())});
          continue;
        }
        selectedCellArrayNames.emplace_back(arrayName);
      }
    }

    if(pTransformationMatrixTypeValue == detail::k_TranslationIdx)
    {
      // If the user is purely doing a translation then just adjust the origin and be done.
      auto pTranslationValue = inputValues.Translation;
      FloatVec3 originVec = imageGeomPtr->getOrigin();
      originVec = {originVec[0] + pTranslationValue[0], originVec[1] + pTranslationValue[1], originVec[2] + pTranslationValue[2]};
      auto spacingVec = imageGeomPtr->getSpacing();
      resultOutputActions.value().appendAction(std::make_unique<UpdateImageGeomAction>(originVec, spacingVec, pSelectedGeometryPathValue));
    }
    else if(pTransformationMatrixTypeValue == detail::k_ScaleIdx)
    {
      // If the user is purely doing a scaling then just adjust the spacing and origin and be done.
      auto pScaleValue = inputValues.Scale;
      FloatVec3 spacingVec = imageGeomPtr->getSpacing();
      spacingVec = {spacingVec[0] * pScaleValue[0], spacingVec[1] * pScaleValue[1], spacingVec[2] * pScaleValue[2]};
      auto minMaxCoords = ImageRotationUtilities::DetermineMinMaxCoords(*imageGeomPtr, transformationMatrix);
      std::vector<float32> originVec = {minMaxCoords[0], minMaxCoords[2], minMaxCoords[4]};
      resultOutputActions.value().appendAction(std::make_unique<UpdateImageGeomAction>(originVec, spacingVec, pSelectedGeometryPathValue));
    }
    else // We are Rotating or manual transformation or precomputed. we need to create a brand new Image Geometry
    {
      auto rotateArgs = ImageRotationUtilities::CreateRotationArgs(*imageGeomPtr, transformationMatrix);

      auto srcImagePath = inputValues.SelectedGeometryPath;
      DataPath destImagePath = srcImagePath;
      const bool pRemoveOriginalGeometry = inputValues.RemoveOriginalGeometry;
      const auto& selectedImageGeom = *imageGeomPtr; // dataStructure.getDataRefAs<ImageGeom>(srcImagePath);

      const std::vector<usize> dims = {static_cast<usize>(rotateArgs.outputDims[0]), static_cast<usize>(rotateArgs.outputDims[1]), static_cast<usize>(rotateArgs.outputDims[2])};
      const std::vector<float32> spacing = {rotateArgs.outputSpacing[0], rotateArgs.outputSpacing[1], rotateArgs.outputSpacing[2]};
      auto originVec = selectedImageGeom.getOrigin().toContainer<std::vector<float32>>();
      originVec[0] = rotateArgs.outputXMin;
      originVec[1] = rotateArgs.outputYMin;
      originVec[2] = rotateArgs.outputZMin;

      if(pRemoveOriginalGeometry)
      {
        // Create an Image Geometry name with a "." as a prefix to the original Image Geometry Name
        std::vector<std::string> tempPathVector = srcImagePath.getPathVector();
        tempPathVector.back() = "." + tempPathVector.back();
        destImagePath = DataPath({tempPathVector});
      }

      // DataArray tuple dimensions use ZYX order, opposite Image Geometry XYZ order.
      std::vector<usize> const dataArrayShape = {dims[2], dims[1], dims[0]};

      std::vector<DataPath> ignorePaths; // already copied over so skip these when collecting child paths to finish copying over later

      {
        const AttributeMatrix* selectedCellDataPtr = selectedImageGeom.getCellData();
        if(selectedCellDataPtr == nullptr)
        {
          return {MakeErrorResult<OutputActions>(-5581, fmt::format("'{}' must have cell data attribute matrix", srcImagePath.toString()))};
        }
        const std::string cellDataName = selectedCellDataPtr->getName();
        ignorePaths.push_back(srcImagePath.createChildPath(cellDataName)); // This prevents a later copy attempt.
        // Create the new Image Geometry
        resultOutputActions.value().appendAction(std::make_unique<CreateImageGeometryAction>(destImagePath, dims, originVec, spacing, cellDataName));

        // Create a DataPath object that points to the Cell AttributeMatrix in the new ImageGeometry
        const DataPath targetCellAttrMatrix = destImagePath.createChildPath(cellDataName);

        // Create the DataArrays in the target Cell Attribute Matrix, based on the interpolation type
        for(const auto& cellArrayName : selectedCellArrayNames)
        {
          const DataPath srcCellArrayDataPath = srcImagePath.createChildPath(cellDataName).createChildPath(cellArrayName);
          const auto& srcArray = dataStructure.getDataRefAs<IDataArray>(srcCellArrayDataPath);
          const ShapeType componentShape = srcArray.getIDataStoreRef().getComponentShape();
          resultOutputActions.value().appendAction(
              std::make_unique<CreateArrayAction>(srcArray.getDataType(), dataArrayShape, componentShape, targetCellAttrMatrix.createChildPath(srcArray.getName())));
        }

        // Store the preflight updated value(s) into the preflightUpdatedValues vector using
        // the appropriate methods.
        // These values should have been updated during the preflightImpl(...) method
        const auto* srcImageGeomPtr = dataStructure.getDataAs<ImageGeom>(srcImagePath);

        preflightUpdatedValues.push_back({"Input Geometry Info", nx::core::GeometryHelpers::Description::GenerateGeometryInfo(srcImageGeomPtr->getDimensions(), srcImageGeomPtr->getSpacing(),
                                                                                                                              srcImageGeomPtr->getOrigin(), srcImageGeomPtr->getUnits())});

        if(pTransformationMatrixTypeValue == detail::k_PrecomputedTransformationMatrixIdx)
        {
          preflightUpdatedValues.push_back({"Transformed Image Geometry Info", k_UnknownPrecomputedMatrixDescription});
        }
        else
        {
          preflightUpdatedValues.push_back(
              {"Transformed Image Geometry Info",
               nx::core::GeometryHelpers::Description::GenerateGeometryInfo(dims, CreateImageGeometryAction::SpacingType{spacing[0], spacing[1], spacing[2]}, originVec, srcImageGeomPtr->getUnits())});
        }
      }

      // copy over the rest of the data from the src Image Geometry into the Target Image Geometry
      auto childPaths = GetAllChildDataPaths(dataStructure, srcImagePath, DataObject::Type::DataObject, ignorePaths);
      if(childPaths.has_value())
      {
        for(const auto& childPath : childPaths.value())
        {
          const std::string copiedChildName = nx::core::StringUtilities::replace(childPath.toString(), srcImagePath.getTargetName(), destImagePath.getTargetName());
          const DataPath copiedChildPath = DataPath::FromString(copiedChildName).value();
          if(dataStructure.getDataAs<BaseGroup>(childPath) != nullptr)
          {
            std::vector<DataPath> allCreatedPaths = {copiedChildPath};
            auto pathsToBeCopied = GetAllChildDataPathsRecursive(dataStructure, childPath);
            if(pathsToBeCopied.has_value())
            {
              for(const auto& sourcePath : pathsToBeCopied.value())
              {
                const std::string createdPathName = nx::core::StringUtilities::replace(sourcePath.toString(), srcImagePath.getTargetName(), destImagePath.getTargetName());
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
    if(!pCellAttributeMatrixPath.getTargetName().empty())
    {
      auto warning = Warning{-5555, fmt::format("The selected geometry is not an 'Image Geometry'. NO interpolation is performed on the data. The 'Cell Attribute Matrix' DataPath can be empty.",
                                                pCellAttributeMatrixPath.getTargetName())};
      resultOutputActions.warnings().push_back(warning);
    }

    // For non-ImageGeom geometries, vertex data is modified in place
    nx::core::AppendDataObjectModifications(dataStructure, resultOutputActions.value().modifiedActions, pSelectedGeometryPathValue, {});
  }

  // Are we saving the transform matrix
  if(saveTransform)
  {
    if(transformMatrixDataPath.empty())
    {
      return {MakeErrorResult<OutputActions>(-5588, fmt::format("The DataPath for the saved Transformation Matrix is empty. Please select or set a DataPath to save the transformation matrix into."))};
    }
    resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(DataType::float32, std::vector<usize>{4, 4}, std::vector<usize>{1}, transformMatrixDataPath));
  }
  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

// -----------------------------------------------------------------------------
ApplyTransformationToGeometry::ApplyTransformationToGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                             ApplyTransformationToGeometryInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ApplyTransformationToGeometry::~ApplyTransformationToGeometry() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ApplyTransformationToGeometry::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ApplyTransformationToGeometry::applyImageGeometryTransformation()
{
  // Preflight actions apply translation through the Image Geometry origin.
  if(m_InputValues->TransformationSelection == detail::k_TranslationIdx)
  {
    return {};
  }

  // Preflight actions apply scale through the Image Geometry origin and spacing.
  if(m_InputValues->TransformationSelection == detail::k_ScaleIdx)
  {
    return {};
  }

  DataPath destImagePath;
  if(m_InputValues->RemoveOriginalGeometry)
  {
    // Create an Image Geometry name with a "." as a prefix to the original Image Geometry Name
    std::vector<std::string> tempPathVector = m_InputValues->SelectedGeometryPath.getPathVector();
    tempPathVector.back() = "." + tempPathVector.back();
    destImagePath = DataPath({tempPathVector});
  }

  auto& srcImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->SelectedGeometryPath);
  auto& destImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(destImagePath);

  const auto rotateArgs = ImageRotationUtilities::CreateRotationArgs(srcImageGeom, m_TransformationMatrix);

  auto selectedCellDataChildren = GetAllChildArrayDataPaths(m_DataStructure, srcImageGeom.getCellDataPath());
  auto selectedCellArrays = selectedCellDataChildren.has_value() ? selectedCellDataChildren.value() : std::vector<DataPath>{};

  ImageRotationUtilities::FilterProgressCallback filterProgressCallback(m_MessageHandler, m_ShouldCancel);

  // Resident image arrays can resample in independent tasks.
  ParallelTaskAlgorithm taskRunner;

  const DataPath srcCelLDataAMPath = srcImageGeom.getCellDataPath();
  const auto& srcCellDataAM = srcImageGeom.getCellDataRef();

  const DataPath destCellDataAMPath = destImageGeom.getCellDataPath();

  if(m_InputValues->TransformationSelection == detail::k_PrecomputedTransformationMatrixIdx)
  {
    // Adjust the destination because the transformation matrix was unavailable during preflight.
    auto& destCellDataAM = destImageGeom.getCellDataRef();
    const std::vector<usize> dims = {static_cast<usize>(rotateArgs.outputDims[0]), static_cast<usize>(rotateArgs.outputDims[1]), static_cast<usize>(rotateArgs.outputDims[2])};
    const std::vector<float32> spacing = {rotateArgs.outputSpacing[0], rotateArgs.outputSpacing[1], rotateArgs.outputSpacing[2]};
    auto origin = srcImageGeom.getOrigin().toContainer<std::vector<float32>>();
    origin[0] = rotateArgs.outputXMin;
    origin[1] = rotateArgs.outputYMin;
    origin[2] = rotateArgs.outputZMin;

    // DataArray tuple dimensions use ZYX order, opposite Image Geometry XYZ order.
    std::vector<usize> const dataArrayShape = {dims[2], dims[1], dims[0]};
    destImageGeom.setDimensions(dims);
    destImageGeom.setOrigin(origin);
    destImageGeom.setSpacing(spacing);
    Result<> resizeResult = destCellDataAM.resizeTuples(dataArrayShape);
    if(resizeResult.invalid())
    {
      return resizeResult;
    }
  }

  // OOC transformations use bounded source pages inside ImageRotationUtilities.
  // Run arrays serially so their page caches and output slices do not multiply
  // resident memory by the number of cell arrays being transformed.
  bool usesOutOfCoreStore = false;
  for(const auto& [dataId, srcDataObject] : srcCellDataAM)
  {
    const auto* srcDataArray = m_DataStructure.getDataAs<IDataArray>(srcCelLDataAMPath.createChildPath(srcDataObject->getName()));
    const auto* destDataArray = m_DataStructure.getDataAs<IDataArray>(destCellDataAMPath.createChildPath(srcDataObject->getName()));
    usesOutOfCoreStore = usesOutOfCoreStore || IsOutOfCore(*srcDataArray) || IsOutOfCore(*destDataArray);
  }
  const bool useOutOfCoreAlgorithm = !ForceInCoreAlgorithm() && (usesOutOfCoreStore || ForceOocAlgorithm());
  RecordAlgorithmPathExecution(useOutOfCoreAlgorithm ? AlgorithmPath::OutOfCore : AlgorithmPath::InCore, usesOutOfCoreStore);
  taskRunner.setParallelizationEnabled(!useOutOfCoreAlgorithm);

  for(const auto& [dataId, srcDataObject] : srcCellDataAM)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    const auto* srcDataArrayPtr = m_DataStructure.getDataAs<IDataArray>(srcCelLDataAMPath.createChildPath(srcDataObject->getName()));
    auto* destDataArrayPtr = m_DataStructure.getDataAs<IDataArray>(destCellDataAMPath.createChildPath(srcDataObject->getName()));

    if(m_InputValues->InterpolationSelection == detail::k_NearestNeighborInterpolationIdx)
    {
      m_MessageHandler.sendInfoMessage(fmt::format("Applying Transform || Nearest Neighbor Interpolation {}", srcDataObject->getName()));

      ExecuteParallelFunction<ImageRotationUtilities::RotateImageGeometryWithNearestNeighbor>(srcDataArrayPtr->getDataType(), taskRunner, srcDataArrayPtr, destDataArrayPtr, rotateArgs,
                                                                                              m_TransformationMatrix, false, &filterProgressCallback);
    }
    else if(m_InputValues->InterpolationSelection == detail::k_LinearInterpolationIdx)
    {
      m_MessageHandler.sendInfoMessage(fmt::format("Applying Transform || Trilinear Interpolation {}", srcDataObject->getName()));

      ExecuteParallelFunction<ImageRotationUtilities::RotateImageGeometryWithTrilinearInterpolation, NoBooleanType>(srcDataArrayPtr->getDataType(), taskRunner, srcDataArrayPtr, destDataArrayPtr,
                                                                                                                    rotateArgs, m_TransformationMatrix, &filterProgressCallback);
    }

    if(getCancel())
    {
      break;
    }
  }

  taskRunner.wait();

  // Surface any error/warning a parallel resample task reported through the shared callback.
  return filterProgressCallback.takeResult();
}

// -----------------------------------------------------------------------------
Result<> ApplyTransformationToGeometry::applyNodeGeometryTransformation()
{
  auto& nodeGeometry0D = m_DataStructure.getDataRefAs<INodeGeometry0D>(m_InputValues->SelectedGeometryPath);

  IGeometry::SharedVertexList& vertexList = nodeGeometry0D.getVerticesRef();

  ImageRotationUtilities::FilterProgressCallback filterProgressCallback(m_MessageHandler, m_ShouldCancel);

  // Node geometry uses direct disjoint vertex ranges. Generic DataArray and DataStore concurrent access is not guaranteed.
  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, vertexList.getNumberOfTuples());
  dataAlg.execute(ImageRotationUtilities::ApplyTransformationToNodeGeometry(vertexList, m_TransformationMatrix, &filterProgressCallback));

  return {};
}

// -----------------------------------------------------------------------------
Result<> ApplyTransformationToGeometry::operator()()
{
  if(!m_InputValues->RemoveOriginalGeometry)
  {
    return MakeErrorResult(-84500, fmt::format("Keeping the original geometry is not supported."));
  }

  switch(m_InputValues->TransformationSelection)
  {
  case detail::k_NoTransformIdx: // No-Op
  {
    return {};
  }
  case detail::k_PrecomputedTransformationMatrixIdx: // Transformation matrix from array
  {
    const auto& precomputed = m_DataStructure.getDataAsUnsafe<Float32Array>(m_InputValues->ComputedTransformationMatrix)->getDataStoreRef();
    m_TransformationMatrix = ImageRotationUtilities::CopyPrecomputedToTransformationMatrix(precomputed);
    break;
  }
  case detail::k_ManualTransformationMatrixIdx: // Manual transformation matrix
  {
    m_TransformationMatrix = ImageRotationUtilities::GenerateManualTransformationMatrix(m_InputValues->ManualMatrixTableData);
    break;
  }
  case detail::k_RotationIdx: // Rotation via axis-angle
  {
    m_TransformationMatrix = ImageRotationUtilities::GenerateRotationTransformationMatrix(m_InputValues->Rotation);
    break;
  }
  case detail::k_TranslationIdx: // Translation
  {
    m_TransformationMatrix = ImageRotationUtilities::GenerateTranslationTransformationMatrix(m_InputValues->Translation);
    break;
  }
  case detail::k_ScaleIdx: // Scale
  {
    m_TransformationMatrix = ImageRotationUtilities::GenerateScaleTransformationMatrix(m_InputValues->Scale);
    break;
  }
  }

  auto* imageGeometryPtr = m_DataStructure.getDataAs<ImageGeom>(m_InputValues->SelectedGeometryPath);
  auto* nodeGeometry0D = m_DataStructure.getDataAs<INodeGeometry0D>(m_InputValues->SelectedGeometryPath);
  if(m_InputValues->TranslateGeometryToGlobalOrigin)
  {
    auto boundingBox = (imageGeometryPtr != nullptr) ? imageGeometryPtr->getBoundingBoxf() : nodeGeometry0D->getBoundingBox();
    Point3Df minPoint = boundingBox.getMinPoint();
    const ImageRotationUtilities::Matrix4fR translationToGlobalOriginMat = ImageRotationUtilities::GenerateTranslationTransformationMatrix({-minPoint[0], -minPoint[1], -minPoint[2]});
    const ImageRotationUtilities::Matrix4fR translationFromGlobalOriginMat = ImageRotationUtilities::GenerateTranslationTransformationMatrix({minPoint[0], minPoint[1], minPoint[2]});
    m_TransformationMatrix = translationFromGlobalOriginMat * m_TransformationMatrix * translationToGlobalOriginMat;
  }

  // Store the matrix in row-major order with columns changing fastest.
  if(m_InputValues->SaveTransformMatrix)
  {
    auto& transformMatrix = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->TransformMatrixPath);
    usize index = 0;
    for(usize row = 0; row < 4; row++)
    {
      for(usize col = 0; col < 4; col++)
      {
        transformMatrix[index++] = m_TransformationMatrix(row, col);
      }
    }
  }

  if(imageGeometryPtr == nullptr)
  {
    return applyNodeGeometryTransformation();
  }
  return applyImageGeometryTransformation();
}

Result<> nx::core::ApplyGeometryTransformation(DataStructure& dataStructure, const ApplyTransformationToGeometryInputValues& inputValues, const IFilter::MessageHandler& messageHandler,
                                               const std::atomic_bool& shouldCancel)
{
  ApplyTransformationToGeometryInputValues resolvedValues = inputValues;
  return ApplyTransformationToGeometry(dataStructure, messageHandler, shouldCancel, &resolvedValues)();
}
