#include "ResampleImageGeomFilter.hpp"

#include "ComplexCore/Filters/Algorithms/ResampleImageGeom.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/INeighborList.hpp"
#include "complex/Filter/Actions/CopyDataObjectAction.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "complex/Filter/Actions/CreateImageGeometryAction.hpp"
#include "complex/Filter/Actions/DeleteDataAction.hpp"
#include "complex/Filter/Actions/RenameDataAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/Utilities/DataGroupUtilities.hpp"
#include "complex/Utilities/GeometryHelpers.hpp"
#include "complex/Utilities/StringUtilities.hpp"

using namespace complex;

namespace
{

const std::string k_TempGeometryName = ".resampled_image_geometry";

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
  return {"Sampling", "Spacing", "Image Geometry", "Conversion"};
}

//------------------------------------------------------------------------------
Parameters ResampleImageGeomFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Spacing_Key, "New Spacing",
                                                         "The new spacing values (dx, dy, dz). Larger spacing will cause less voxels, smaller spacing will cause more voxels.",
                                                         std::vector<float32>{1.0F, 1.0F, 1.0F}, std::vector<std::string>{"X", "Y", "Z"}));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_RemoveOriginalGeometry_Key, "Perform In Place", "Removes the original Image Geometry after filter is completed", true));

  params.insertSeparator({"Input Image Geometry"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometry_Key, "Selected Image Geometry", "The target geometry to resample", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));

  params.insertSeparator(Parameters::Separator{"Renumber Features Input Parameters"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_RenumberFeatures_Key, "Renumber Features", "Specifies if the feature IDs should be renumbered", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Feature IDs", "DataPath to Cell Feature IDs array", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(
      std::make_unique<AttributeMatrixSelectionParameter>(k_FeatureAttributeMatrix_Key, "Cell Feature Attribute Matrix", "DataPath to the feature Attribute Matrix", DataPath({"CellFeatureData"})));

  params.insertSeparator({"Output Image Geometry"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_CreatedImageGeometry_Key, "Created Image Geometry", "The location of the resampled geometry", DataPath()));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_RenumberFeatures_Key, k_CellFeatureIdsArrayPath_Key, true);
  params.linkParameters(k_RenumberFeatures_Key, k_FeatureAttributeMatrix_Key, true);
  params.linkParameters(k_RemoveOriginalGeometry_Key, k_CreatedImageGeometry_Key, false);

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
  auto destImagePath = filterArgs.value<DataPath>(k_CreatedImageGeometry_Key);
  auto shouldRenumberFeatures = filterArgs.value<bool>(k_RenumberFeatures_Key);
  auto featureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  auto cellFeatureAmPath = filterArgs.value<DataPath>(k_FeatureAttributeMatrix_Key);
  auto srcImagePath = filterArgs.value<DataPath>(k_SelectedImageGeometry_Key);

  complex::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  // Validate the user supplied spacing values.
  if(pSpacingValue[0] < 0.0F || pSpacingValue[1] < 0.0F || pSpacingValue[2] < 0.0F)
  {
    return {MakeErrorResult<OutputActions>(-11500, fmt::format("Input Spacing has a negative value. {}, {}, {}", pSpacingValue[0], pSpacingValue[1], pSpacingValue[2]))};
  }

  const auto* srcImageGeom = dataStructure.getDataAs<ImageGeom>(srcImagePath);

  complex::SizeVec3 srcDimensions = srcImageGeom->getDimensions();
  complex::FloatVec3 srcSpacing = srcImageGeom->getSpacing();
  std::vector<float32> srcOrigin = srcImageGeom->getOrigin().toContainer<std::vector<float>>();

  // This section ensures that if the spacing is set in such a way that we have at
  // least 1 for the value of the dimension
  auto mXp = static_cast<size_t>(((srcSpacing[0] * static_cast<float>(srcDimensions[0])) / pSpacingValue[0]));
  auto mYp = static_cast<size_t>(((srcSpacing[1] * static_cast<float>(srcDimensions[1])) / pSpacingValue[1]));
  auto mZp = static_cast<size_t>(((srcSpacing[2] * static_cast<float>(srcDimensions[2])) / pSpacingValue[2]));
  if(mXp == 0)
  {
    mXp = 1;
  }
  if(mYp == 0)
  {
    mYp = 1;
  }
  if(mZp == 0)
  {
    mZp = 1;
  }

  std::vector<usize> geomDims = {mXp, mYp, mZp};                               // The ImageGeometryDimensions go from Fastest to Slowest, XYZ.
  std::vector<usize> dataArrayShape = {geomDims[2], geomDims[1], geomDims[0]}; // The DataArray shape goes slowest to fastest (ZYX)

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

  // This section gets the cell attribute matrix for the input Image Geometry and
  // then creates new arrays from each array that is in that attribute matrix. We
  // also push this attribute matrix into the `ignorePaths` variable since we do
  // not need to manually copy these arrays to the destination image geometry
  {
    // Get the name of the Cell Attribute Matrix, so we can use that in teh CreateImageGeometryAction
    const AttributeMatrix* selectedCellData = srcImageGeom->getCellData();
    if(selectedCellData == nullptr)
    {
      return {MakeErrorResult<OutputActions>(-5551, fmt::format("'{}' must have cell data attribute matrix", srcImagePath.toString()))};
    }
    std::string cellDataName = selectedCellData->getName();
    ignorePaths.push_back(srcImagePath.createChildPath(cellDataName));

    resultOutputActions.value().actions.push_back(
        std::make_unique<CreateImageGeometryAction>(destImagePath, geomDims, srcOrigin, CreateImageGeometryAction::OriginType{pSpacingValue[0], pSpacingValue[1], pSpacingValue[2]}, cellDataName));

    // Now loop over each array in the source image geometry's cell attribute matrix and create the corresponding arrays
    // in the destination image geometry's attribute matrix
    DataPath newCellAttributeMatrixPath = destImagePath.createChildPath(cellDataName);
    for(const auto& [identifier, object] : *selectedCellData)
    {
      const auto& srcArray = dynamic_cast<const IDataArray&>(*object);
      DataType dataType = srcArray.getDataType();
      IDataStore::ShapeType componentShape = srcArray.getIDataStoreRef().getComponentShape();
      DataPath dataArrayPath = newCellAttributeMatrixPath.createChildPath(srcArray.getName());
      resultOutputActions.value().actions.push_back(std::make_unique<CreateArrayAction>(dataType, dataArrayShape, std::move(componentShape), dataArrayPath));
    }

    // Store the preflight updated value(s) into the preflightUpdatedValues vector using
    preflightUpdatedValues.push_back(
        {"Input Geometry Info", complex::GeometryHelpers::Description::GenerateGeometryInfo(srcImageGeom->getDimensions(), srcImageGeom->getSpacing(), srcImageGeom->getOrigin())});
    preflightUpdatedValues.push_back({"Resampled Image Geometry Info", complex::GeometryHelpers::Description::GenerateGeometryInfo(geomDims, pSpacingValue, srcOrigin)});
  }

  // This section covers the option of renumbering the Feature Data where we need to do a
  // similar creation of the Data Arrays based on the arrays in the Source Image Geometry's
  // Feature Attribute Matrix
  if(shouldRenumberFeatures)
  {
    ignorePaths.push_back(cellFeatureAmPath);
    const auto* srcCellFeatureData = dataStructure.getDataAs<AttributeMatrix>(cellFeatureAmPath);
    if(nullptr == srcCellFeatureData)
    {
      return {MakeErrorResult<OutputActions>(-55502, fmt::format("Could not find the selected Attribute Matrix '{}'", cellFeatureAmPath.toString()))};
    }
    std::string warningMsg;
    DataPath destCellFeatureAmPath = destImagePath.createChildPath(cellFeatureAmPath.getTargetName());
    auto tDims = srcCellFeatureData->getShape();
    resultOutputActions.value().actions.push_back(std::make_unique<CreateAttributeMatrixAction>(destCellFeatureAmPath, tDims));
    for(const auto& [identifier, object] : *srcCellFeatureData)
    {
      if(const auto* srcArray = dynamic_cast<const IDataArray*>(object.get()); srcArray != nullptr)
      {
        DataType dataType = srcArray->getDataType();
        IDataStore::ShapeType componentShape = srcArray->getIDataStoreRef().getComponentShape();
        DataPath dataArrayPath = destCellFeatureAmPath.createChildPath(srcArray->getName());
        resultOutputActions.value().actions.push_back(std::make_unique<CreateArrayAction>(dataType, tDims, std::move(componentShape), dataArrayPath));
      }
      else if(const auto* srcNeighborListArray = dynamic_cast<const INeighborList*>(object.get()); srcNeighborListArray != nullptr)
      {
        warningMsg += "\n" + cellFeatureAmPath.toString() + "/" + srcNeighborListArray->getName();
      }
    }
    if(!warningMsg.empty())
    {
      resultOutputActions.m_Warnings.push_back(
          Warning({-55503, fmt::format("This filter modifies the Cell Level Array '{}', the following arrays are of type NeighborList and will not be copied over:{}", featureIdsArrayPath.toString(),
                                       warningMsg)}));
    }
  }

  // This section copies any remaining data groups or data arrays that are loose
  // in the source image geometry and creates CopyDataObjectAction instances for
  // those objects
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

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ResampleImageGeomFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter*, const MessageHandler& messageHandler,
                                              const std::atomic_bool& shouldCancel) const
{
  ResampleImageGeomInputValues inputValues;

  inputValues.Spacing = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);

  inputValues.SelectedImageGeometryPath = filterArgs.value<DataPath>(k_SelectedImageGeometry_Key);
  const auto* cellDataGroup = dataStructure.getDataRefAs<ImageGeom>(inputValues.SelectedImageGeometryPath).getCellData();
  inputValues.CellDataGroupPath = inputValues.SelectedImageGeometryPath.createChildPath(cellDataGroup->getName());

  inputValues.RenumberFeatures = filterArgs.value<bool>(k_RenumberFeatures_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputValues.CellFeatureAttributeMatrix = filterArgs.value<DataPath>(k_FeatureAttributeMatrix_Key);

  inputValues.RemoveOriginalImageGeom = filterArgs.value<bool>(k_RemoveOriginalGeometry_Key);
  inputValues.CreatedImageGeometryPath = filterArgs.value<DataPath>(k_CreatedImageGeometry_Key);

  if(inputValues.RemoveOriginalImageGeom)
  {
    auto tempPathVector = inputValues.SelectedImageGeometryPath.getPathVector();
    std::string tempName = k_TempGeometryName;
    tempPathVector.back() = tempName;
    inputValues.CreatedImageGeometryPath = DataPath({tempPathVector});
  }

  return ResampleImageGeom(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
