#include "CropImageGeometry.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/INeighborList.hpp"
#include "complex/Filter/Actions/CopyArrayInstanceAction.hpp"
#include "complex/Filter/Actions/CopyDataObjectAction.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "complex/Filter/Actions/CreateImageGeometryAction.hpp"
#include "complex/Filter/Actions/DeleteDataAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/Utilities/ParallelDataAlgorithm.hpp"
#include "complex/Utilities/ParallelTaskAlgorithm.hpp"
#include "complex/Utilities/SamplingUtils.hpp"
#include "complex/Utilities/StringUtilities.hpp"

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
namespace
{
template <typename T>
void copyDataTuple(const IDataArray& oldArray, IDataArray& newArray, usize oldIndex, usize newIndex)
{
  const auto& oldArrayCast = static_cast<const DataArray<T>&>(oldArray);
  auto& newArrayCast = static_cast<DataArray<T>&>(newArray);

  auto numComponents = oldArrayCast.getNumberOfComponents();

  for(usize i = 0; i < numComponents; i++)
  {
    usize newOffset = newIndex * numComponents;
    usize oldOffset = oldIndex * numComponents;
    newArrayCast[newOffset + i] = oldArrayCast[oldOffset + i];
  }
}

void copyDataArrayTuple(const IDataArray& oldArray, IDataArray& newArray, usize oldIndex, usize newIndex)
{
  auto dataType = oldArray.getDataType();
  switch(dataType)
  {
  case DataType::boolean:
    copyDataTuple<bool>(oldArray, newArray, oldIndex, newIndex);
    break;
  case DataType::float32:
    copyDataTuple<float32>(oldArray, newArray, oldIndex, newIndex);
    break;
  case DataType::float64:
    copyDataTuple<float64>(oldArray, newArray, oldIndex, newIndex);
    break;
  case DataType::int8:
    copyDataTuple<int8>(oldArray, newArray, oldIndex, newIndex);
    break;
  case DataType::int16:
    copyDataTuple<int16>(oldArray, newArray, oldIndex, newIndex);
    break;
  case DataType::int32:
    copyDataTuple<int32>(oldArray, newArray, oldIndex, newIndex);
    break;
  case DataType::int64:
    copyDataTuple<int64>(oldArray, newArray, oldIndex, newIndex);
    break;
  case DataType::uint8:
    copyDataTuple<uint8>(oldArray, newArray, oldIndex, newIndex);
    break;
  case DataType::uint16:
    copyDataTuple<uint16>(oldArray, newArray, oldIndex, newIndex);
    break;
  case DataType::uint32:
    copyDataTuple<uint32>(oldArray, newArray, oldIndex, newIndex);
    break;
  case DataType::uint64:
    copyDataTuple<uint64>(oldArray, newArray, oldIndex, newIndex);
    break;
  default:
    break;
  }
}

USizeVec3 getCurrentVolumeDataContainerDimensions(const DataStructure& dataStructure, const DataPath& imageGeomPath)
{
  USizeVec3 data = {0, 0, 0};

  const auto* image = dataStructure.getDataAs<ImageGeom>(imageGeomPath);
  if(image != nullptr)
  {
    data[0] = image->getNumXCells();
    data[1] = image->getNumYCells();
    data[2] = image->getNumZCells();
  }
  return data;
}

FloatVec3 getCurrentVolumeDataContainerResolutions(const DataStructure& dataStructure, const DataPath& imageGeomPath)
{
  FloatVec3 data = {0, 0, 0};
  const auto* image = dataStructure.getDataAs<ImageGeom>(imageGeomPath);
  if(image != nullptr)
  {
    data = image->getSpacing();
  }
  return data;
}

template <typename T>
class CropImageGeomDataArray
{
public:
  CropImageGeomDataArray(const IDataArray& oldCellArray, IDataArray& newCellArray, const ImageGeom& srcImageGeom, std::array<uint64, 6> bounds, const std::atomic_bool& shouldCancel)
  : m_OldCellArray(dynamic_cast<const DataArray<T>&>(oldCellArray))
  , m_NewCellArray(dynamic_cast<DataArray<T>&>(newCellArray))
  , m_SrcImageGeom(srcImageGeom)
  , m_Bounds(bounds)
  , m_ShouldCancel(shouldCancel)
  {
  }

  void operator()() const
  {
    convert();
  }

protected:
  void convert() const
  {
    size_t numComps = m_OldCellArray.getNumberOfComponents();

    uint64 col = 0;
    uint64 row = 0;
    uint64 plane = 0;

    uint64 colold = 0;
    uint64 rowold = 0;
    uint64 planeold = 0;
    uint64 index = 0;
    uint64 index_old = 0;

    // Loop over every tuple in this array
    for(int64 i = 0; i < m_Bounds[5]; i++)
    {
      if(m_ShouldCancel)
      {
        return;
      }

      planeold = (i + m_Bounds[4]) * (m_SrcImageGeom.getNumXCells() * m_SrcImageGeom.getNumYCells());
      plane = (i * m_Bounds[1] * m_Bounds[3]);
      for(int64 j = 0; j < m_Bounds[3]; j++)
      {
        rowold = (j + m_Bounds[2]) * m_SrcImageGeom.getNumXCells();
        row = (j * m_Bounds[1]);
        for(int64 k = 0; k < m_Bounds[1]; k++)
        {
          colold = (k + m_Bounds[0]);
          col = k;
          index_old = planeold + rowold + colold;
          index = plane + row + col;

          for(size_t compIndex = 0; compIndex < numComps; compIndex++)
          {
            m_NewCellArray[index * numComps + compIndex] = m_OldCellArray[index_old * numComps + compIndex];
          }
        }
      }
    }
  }

private:
  const DataArray<T>& m_OldCellArray;
  DataArray<T>& m_NewCellArray;
  const ImageGeom& m_SrcImageGeom;
  std::array<uint64, 6> m_Bounds;
  const std::atomic_bool& m_ShouldCancel;
};

void cropDataArray(ParallelTaskAlgorithm& taskRunner, const IDataArray& oldCellArray, IDataArray& newCellArray, const ImageGeom& srcImageGeom, std::array<uint64, 6> bounds,
                   const std::atomic_bool& shouldCancel)
{
  DataType type = oldCellArray.getDataType();
  switch(type)
  {
  case DataType::boolean: {
    taskRunner.execute(CropImageGeomDataArray<bool>(oldCellArray, newCellArray, srcImageGeom, bounds, shouldCancel));
    break;
  }
  case DataType::int8: {
    taskRunner.execute(CropImageGeomDataArray<int8>(oldCellArray, newCellArray, srcImageGeom, bounds, shouldCancel));
    break;
  }
  case DataType::int16: {
    taskRunner.execute(CropImageGeomDataArray<int16>(oldCellArray, newCellArray, srcImageGeom, bounds, shouldCancel));
    break;
  }
  case DataType::int32: {
    taskRunner.execute(CropImageGeomDataArray<int32>(oldCellArray, newCellArray, srcImageGeom, bounds, shouldCancel));
    break;
  }
  case DataType::int64: {
    taskRunner.execute(CropImageGeomDataArray<int64>(oldCellArray, newCellArray, srcImageGeom, bounds, shouldCancel));
    break;
  }
  case DataType::uint8: {
    taskRunner.execute(CropImageGeomDataArray<uint8>(oldCellArray, newCellArray, srcImageGeom, bounds, shouldCancel));
    break;
  }
  case DataType::uint16: {
    taskRunner.execute(CropImageGeomDataArray<uint16>(oldCellArray, newCellArray, srcImageGeom, bounds, shouldCancel));
    break;
  }
  case DataType::uint32: {
    taskRunner.execute(CropImageGeomDataArray<uint32>(oldCellArray, newCellArray, srcImageGeom, bounds, shouldCancel));
    break;
  }
  case DataType::uint64: {
    taskRunner.execute(CropImageGeomDataArray<uint64>(oldCellArray, newCellArray, srcImageGeom, bounds, shouldCancel));
    break;
  }
  case DataType::float32: {
    taskRunner.execute(CropImageGeomDataArray<float32>(oldCellArray, newCellArray, srcImageGeom, bounds, shouldCancel));
    break;
  }
  case DataType::float64: {
    taskRunner.execute(CropImageGeomDataArray<float64>(oldCellArray, newCellArray, srcImageGeom, bounds, shouldCancel));
    break;
  }
  default: {
    throw std::runtime_error("Invalid DataType");
  }
  }
}

} // namespace

//------------------------------------------------------------------------------
std::string CropImageGeometry::name() const
{
  return FilterTraits<CropImageGeometry>::name;
}

//------------------------------------------------------------------------------
std::string CropImageGeometry::className() const
{
  return FilterTraits<CropImageGeometry>::className;
}

//------------------------------------------------------------------------------
Uuid CropImageGeometry::uuid() const
{
  return FilterTraits<CropImageGeometry>::uuid;
}

//------------------------------------------------------------------------------
std::string CropImageGeometry::humanName() const
{
  return "Crop Geometry (Image)";
}

//------------------------------------------------------------------------------
std::vector<std::string> CropImageGeometry::defaultTags() const
{
  return {"#Core", "#Crop Image Geometry"};
}

//------------------------------------------------------------------------------
Parameters CropImageGeometry::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ImageGeom_Key, "Image Geom", "DataPath to the target ImageGeom", DataPath(), std::set{IGeometry::Type::Image}));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_NewImageGeom_Key, "New Image Geom", "DataPath to create the new ImageGeom at", DataPath()));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_RemoveOriginalGeometry_Key, "Remove Original Image Geometry Group", "Remove the original geometry after cropping", true));

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<VectorUInt64Parameter>(k_MinVoxel_Key, "Min Voxel", "Lower bound of the volume to crop out", std::vector<uint64>{0, 0, 0},
                                                        std::vector<std::string>{"X (Column)", "Y (Row)", "Z (Plane)"}));
  params.insert(std::make_unique<VectorUInt64Parameter>(k_MaxVoxel_Key, "Max Voxel [Inclusive]", "Upper bound of the volume to crop out", std::vector<uint64>{0, 0, 0},
                                                        std::vector<std::string>{"X (Column)", "Y (Row)", "Z (Plane)"}));
  params.insert(std::make_unique<BoolParameter>(k_UpdateOrigin_Key, "Update Origin", "Specifies if the origin should be updated", false));

  params.insertSeparator(Parameters::Separator{"Renumber Features Input Parameters"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_RenumberFeatures_Key, "Renumber Features", "Specifies if the feature IDs should be renumbered", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIds_Key, "Feature IDs", "DataPath to Feature IDs array", DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::int32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_CellFeatureAttributeMatrix_Key, "Cell Feature Attribute Matrix", "DataPath to the call feature Attribute Matrix",
                                                                    DataPath({"CellFeatureData"})));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_RenumberFeatures_Key, k_FeatureIds_Key, true);
  params.linkParameters(k_RenumberFeatures_Key, k_CellFeatureAttributeMatrix_Key, true);

  return params;
}

IFilter::UniquePointer CropImageGeometry::clone() const
{
  return std::make_unique<CropImageGeometry>();
}

IFilter::PreflightResult CropImageGeometry::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{

  auto srcImagePath = args.value<DataPath>(k_ImageGeom_Key);
  auto destImagePath = args.value<DataPath>(k_NewImageGeom_Key);
  auto featureIdsArrayPath = args.value<DataPath>(k_FeatureIds_Key);
  auto minVoxels = args.value<std::vector<uint64>>(k_MinVoxel_Key);
  auto maxVoxels = args.value<std::vector<uint64>>(k_MaxVoxel_Key);
  auto shouldUpdateOrigin = args.value<bool>(k_UpdateOrigin_Key);
  auto shouldRenumberFeatures = args.value<bool>(k_RenumberFeatures_Key);
  auto cellFeatureAMPath = args.value<DataPath>(k_CellFeatureAttributeMatrix_Key);
  auto pRemoveOriginalGeometry = args.value<bool>(k_RemoveOriginalGeometry_Key);

  auto xMin = minVoxels[0];
  auto xMax = maxVoxels[0];
  auto yMax = maxVoxels[1];
  auto yMin = minVoxels[1];
  auto zMax = maxVoxels[2];
  auto zMin = minVoxels[2];

  complex::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  if(xMax < xMin)
  {
    std::string errMsg = fmt::format("X Max (%1) less than X Min (%2)", xMax, xMin);
    return {MakeErrorResult<OutputActions>(-5550, errMsg)};
  }
  if(yMax < yMin)
  {
    std::string errMsg = fmt::format("Y Max ({}) less than Y Min ({})", yMax, yMin);
    return {MakeErrorResult<OutputActions>(-5550, errMsg)};
  }
  if(zMax < zMin)
  {
    std::string errMsg = fmt::format("Z Max ({}) less than Z Min ({})", zMax, zMin);
    return {MakeErrorResult<OutputActions>(-5550, errMsg)};
  }

  // Validate the incoming DataContainer, Geometry, and AttributeMatrix.
  // Provides {0, 0, 0} or {1, 1, 1} respectfully if the geometry could not be found.
  auto srcDimensions = getCurrentVolumeDataContainerDimensions(data, srcImagePath);
  auto srcSpacing = getCurrentVolumeDataContainerResolutions(data, srcImagePath);

  const auto* srcImageGeom = data.getDataAs<ImageGeom>(srcImagePath);
  auto srcOrigin = srcImageGeom->getOrigin();

  if(xMax > srcImageGeom->getNumXCells() - 1)
  {
    std::string errMsg = fmt::format("The X Max ({}) is greater than the Image Geometry X extent ({})", xMax, srcImageGeom->getNumXCells() - 1);
    return {MakeErrorResult<OutputActions>(-5550, errMsg)};
  }

  if(yMax > srcImageGeom->getNumYCells() - 1)
  {
    std::string errMsg = fmt::format("The Y Max ({}) is greater than the Image Geometry Y extent ({})", yMax, srcImageGeom->getNumYCells() - 1);
    return {MakeErrorResult<OutputActions>(-5550, errMsg)};
  }

  if(zMax > srcImageGeom->getNumZCells() - 1)
  {
    std::string errMsg = fmt::format("The Z Max ({}) is greater than the Image Geometry Z extent ({})", zMax, srcImageGeom->getNumZCells() - 1);
    return {MakeErrorResult<OutputActions>(-5550, errMsg)};
  }

  const usize requiredTupleCount = srcDimensions[0] * srcDimensions[1] * srcDimensions[2];

  std::vector<usize> tDims(3, 0);
  if(static_cast<int>(xMax) - static_cast<int>(xMin) < 0)
  {
    xMax = xMin + 1;
  }
  if(static_cast<int>(yMax) - static_cast<int>(yMin) < 0)
  {
    yMax = yMin + 1;
  }
  if(static_cast<int>(zMax) - static_cast<int>(zMin) < 0)
  {
    zMax = zMin + 1;
  }

  tDims[0] = (xMax - xMin) + 1;
  tDims[1] = (yMax - yMin) + 1;
  tDims[2] = (zMax - zMin) + 1;

  FloatVec3 targetSpacing;
  targetSpacing = srcSpacing;

  std::vector<float32> targetOrigin(3);
  if(shouldUpdateOrigin)
  {
    targetOrigin[0] = static_cast<float>(xMin) * targetSpacing[0] + srcOrigin[0];
    targetOrigin[1] = static_cast<float>(yMin) * targetSpacing[1] + srcOrigin[1];
    targetOrigin[2] = static_cast<float>(zMin) * targetSpacing[2] + srcOrigin[2];
  }
  else
  {
    targetOrigin[0] = srcOrigin[0];
    targetOrigin[1] = srcOrigin[1];
    targetOrigin[2] = srcOrigin[2];
  }

  std::vector<DataPath> ignorePaths; // already copied over so skip these when collecting child paths to finish copying over later

  // saveAsNewImage
  {
    auto spacing = srcImageGeom->getSpacing();
    std::vector<float32> spacingVec(3);
    for(usize i = 0; i < 3; i++)
    {
      spacingVec[i] = spacing[i];
    }
    const AttributeMatrix* cellData = srcImageGeom->getCellData();
    if(cellData == nullptr)
    {
      return {MakeErrorResult<OutputActions>(-5551, fmt::format("'{}' must have cell data attribute matrix", srcImagePath.toString()))};
    }
    std::string cellDataName = cellData->getName();
    ignorePaths.push_back(srcImagePath.createChildPath(cellDataName));
    auto geomAction = std::make_unique<CreateImageGeometryAction>(destImagePath, tDims, targetOrigin, spacingVec, cellDataName);
    resultOutputActions.value().actions.push_back(std::move(geomAction));

    DataPath newCellFeaturesPath = destImagePath.createChildPath(cellDataName);

    for(const auto& [id, object] : *cellData)
    {
      const auto& srcArray = dynamic_cast<const IDataArray&>(*object);
      DataType dataType = srcArray.getDataType();
      IDataStore::ShapeType componentShape = srcArray.getIDataStoreRef().getComponentShape();
      DataPath dataArrayPath = newCellFeaturesPath.createChildPath(srcArray.getName());
      resultOutputActions.value().actions.push_back(std::make_unique<CreateArrayAction>(dataType, tDims, std::move(componentShape), dataArrayPath));
    }

    // Store the preflight updated value(s) into the preflightUpdatedValues vector using
    // the appropriate methods.
    // These values should have been updated during the preflightImpl(...) method
    preflightUpdatedValues.push_back({"Input Geometry Info", ::GenerateGeometryInfo(srcImageGeom->getDimensions(), srcImageGeom->getSpacing(), srcImageGeom->getOrigin())});
    preflightUpdatedValues.push_back({"Cropped Image Geometry Info", ::GenerateGeometryInfo(tDims, spacingVec, targetOrigin)});
  }

  if(shouldRenumberFeatures)
  {
    ignorePaths.push_back(cellFeatureAMPath);
    auto* featureIdsPtr = data.getDataAs<DataArray<int32>>(featureIdsArrayPath);
    if(nullptr == featureIdsPtr)
    {
      std::string errMsg = fmt::format("The DataArray '{}' which defines the Feature Ids to renumber is invalid. Does it exist? Is it the correct type?", featureIdsArrayPath.toString());
      return {MakeErrorResult<OutputActions>(-55500, errMsg)};
    }
    if(featureIdsPtr->getNumberOfComponents() != 1)
    {
      std::string errMsg = fmt::format("The Feature IDs array does not have the correct component dimensions. 1 component required. Array has {}", featureIdsPtr->getNumberOfComponents());
      return {MakeErrorResult<OutputActions>(-55501, errMsg)};
    }
    const AttributeMatrix* srcCellFeaturData = data.getDataAs<AttributeMatrix>(cellFeatureAMPath);
    if(nullptr == srcCellFeaturData)
    {
      std::string errMsg = fmt::format("Could not find the selected Attribute Matrix '{}'", cellFeatureAMPath.toString());
      return {MakeErrorResult<OutputActions>(-55502, errMsg)};
    }
    std::string warningMsg = "";
    DataPath destCellFeatureAMPath = destImagePath.createChildPath(cellFeatureAMPath.getTargetName());
    tDims = srcCellFeaturData->getShape();
    resultOutputActions.value().actions.push_back(std::make_unique<CreateAttributeMatrixAction>(destCellFeatureAMPath, tDims));
    for(const auto& [id, object] : *srcCellFeaturData)
    {
      if(const auto* srcArray = dynamic_cast<const IDataArray*>(object.get()); srcArray != nullptr)
      {
        DataType dataType = srcArray->getDataType();
        IDataStore::ShapeType componentShape = srcArray->getIDataStoreRef().getComponentShape();
        DataPath dataArrayPath = destCellFeatureAMPath.createChildPath(srcArray->getName());
        resultOutputActions.value().actions.push_back(std::make_unique<CreateArrayAction>(dataType, tDims, std::move(componentShape), dataArrayPath));
      }
      else if(const auto* srcNeighborListArray = dynamic_cast<const INeighborList*>(object.get()); srcNeighborListArray != nullptr)
      {
        warningMsg += "\n" + cellFeatureAMPath.toString() + "/" + srcNeighborListArray->getName();
      }
    }
    if(!warningMsg.empty())
    {
      resultOutputActions.m_Warnings.push_back(
          Warning({-55503, fmt::format("This filter modifies the Cell Level Array '{}', the following arrays are of type NeighborList and will not be copied over:{}", featureIdsArrayPath.toString(),
                                       warningMsg)}));
    }
  }

  // copy over the rest of the data
  auto childPaths = GetAllChildDataPaths(data, srcImagePath, DataObject::Type::DataObject, ignorePaths);
  if(childPaths.has_value())
  {
    for(const auto& childPath : childPaths.value())
    {
      std::string copiedChildName = complex::StringUtilities::replace(childPath.toString(), srcImagePath.getTargetName(), destImagePath.getTargetName());
      DataPath copiedChildPath = DataPath::FromString(copiedChildName).value();
      if(data.getDataAs<BaseGroup>(childPath) != nullptr)
      {
        std::vector<DataPath> allCreatedPaths = {copiedChildPath};
        auto pathsToBeCopied = GetAllChildDataPathsRecursive(data, childPath);
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
    resultOutputActions.value().deferredActions.push_back(std::make_unique<DeleteDataAction>(srcImagePath));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

Result<> CropImageGeometry::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                        const std::atomic_bool& shouldCancel) const
{
  auto srcImagePath = args.value<DataPath>(k_ImageGeom_Key);
  auto destImagePath = args.value<DataPath>(k_NewImageGeom_Key);
  auto minVoxels = args.value<std::vector<uint64>>(k_MinVoxel_Key);
  auto maxVoxels = args.value<std::vector<uint64>>(k_MaxVoxel_Key);
  auto shouldRenumberFeatures = args.value<bool>(k_RenumberFeatures_Key);
  auto featureIdsArrayPath = args.value<DataPath>(k_FeatureIds_Key);
  auto cellFeatureAMPath = args.value<DataPath>(k_CellFeatureAttributeMatrix_Key);

  uint64 xMin = minVoxels[0];
  uint64 xMax = maxVoxels[0];
  uint64 yMax = maxVoxels[1];
  uint64 yMin = minVoxels[1];
  uint64 zMax = maxVoxels[2];
  uint64 zMin = minVoxels[2];

  auto& srcImageGeom = data.getDataRefAs<ImageGeom>(srcImagePath);
  auto& destImageGeom = data.getDataRefAs<ImageGeom>(destImagePath);

  // No matter where the AM is (same DC or new DC), we have the correct DC and AM pointers...now it's time to crop

  SizeVec3 udims = srcImageGeom.getDimensions();

  int64 dims[3] = {
      static_cast<int64>(udims[0]),
      static_cast<int64>(udims[1]),
      static_cast<int64>(udims[2]),
  };

  // Check to see if the dims have actually changed.
  if(dims[0] == (xMax - xMin) && dims[1] == (yMax - yMin) && dims[2] == (zMax - zMin))
  {
    return {};
  }

  // Get current origin
  FloatVec3 oldOrigin = destImageGeom.getOrigin();

  // Check to make sure the new dimensions are not "out of bounds" and warn the user if they are
  if(dims[0] <= xMax)
  {
    std::string errMsg = fmt::format("The Max X value ({}) is greater than the Image Geometry X entent ({})."
                                     " This may lead to junk data being filled into the extra space.",
                                     xMax, dims[0]);
    return MakeErrorResult(-950, errMsg);
  }
  if(dims[1] <= yMax)
  {
    std::string errMsg = fmt::format("The Max Y value ({}) is greater than the Image Geometry Y entent ({})."
                                     " This may lead to junk data being filled into the extra space.",
                                     yMax, dims[1]);
    return MakeErrorResult(-951, errMsg);
  }
  if(dims[2] <= zMax)
  {
    std::string errMsg = fmt::format("The Max Z value ({}) is greater than the Image Geometry Z entent ({})."
                                     " This may lead to junk data being filled into the extra space.",
                                     zMax, dims[2]);
    return MakeErrorResult(-952, errMsg);
  }

  std::array<uint64, 6> bounds = {xMin, ((xMax - xMin) + 1), yMin, ((yMax - yMin) + 1), zMin, ((zMax - zMin) + 1)};

  ParallelTaskAlgorithm taskRunner;
  const auto& srcCellDataAM = srcImageGeom.getCellDataRef();
  auto& destCellDataAM = destImageGeom.getCellDataRef();

  for(const auto& [dataId, oldDataObject] : srcCellDataAM)
  {
    if(shouldCancel)
    {
      return {};
    }

    const auto& oldDataArray = dynamic_cast<const IDataArray&>(*oldDataObject);
    std::string srcName = oldDataArray.getName();

    auto& newDataArray = dynamic_cast<IDataArray&>(destCellDataAM.at(srcName));

    std::string progMsg = fmt::format("Cropping Volume || Copying Data Array {}", srcName);
    messageHandler(progMsg);

    cropDataArray(taskRunner, oldDataArray, newDataArray, srcImageGeom, bounds, shouldCancel);
  }

  // This will spill over if the number of DataArrays to process does not divide evenly by the number of threads.
  taskRunner.wait();

  if(shouldCancel)
  {
    return {};
  }

  std::vector<usize> tDims(3, 0);
  tDims[0] = bounds[1];
  tDims[1] = bounds[3];
  tDims[2] = bounds[5];

  if(shouldRenumberFeatures)
  {
    const AttributeMatrix* srcCellFeaturData = data.getDataAs<AttributeMatrix>(cellFeatureAMPath);
    DataPath destCellFeatureAMPath = destImagePath.createChildPath(cellFeatureAMPath.getTargetName());
    AttributeMatrix* cellFeaturData = data.getDataAs<AttributeMatrix>(destCellFeatureAMPath);
    for(const auto& [id, object] : *cellFeaturData)
    {
      if(shouldCancel)
      {
        return {};
      }
      auto& newDataArray = dynamic_cast<IDataArray&>(*object);
      const auto& oldDataArray = data.getDataRefAs<const IDataArray>(cellFeatureAMPath.createChildPath(newDataArray.getName()));

      for(usize i = 0; i < oldDataArray.getNumberOfTuples(); ++i)
      {
        copyDataArrayTuple(oldDataArray, newDataArray, i, i);
      }
    }
    // This will spill over if the number of DataArrays to process does not divide evenly by the number of threads.
    // taskRunner.wait();
    DataPath destFeatureIdsPath = destImagePath.createChildPath(srcCellDataAM.getName()).createChildPath(featureIdsArrayPath.getTargetName());
    return Sampling::RenumberFeatures(data, destImagePath, destCellFeatureAMPath, featureIdsArrayPath, destFeatureIdsPath, shouldCancel);
  }

  return {};
}
} // namespace complex
