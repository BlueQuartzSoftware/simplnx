#include "MapPointCloudToRegularGridFilter.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/NumericTypeParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

namespace complex
{
namespace
{

} // namespace

std::string MapPointCloudToRegularGridFilter::name() const
{
  return FilterTraits<MapPointCloudToRegularGridFilter>::name;
}

std::string MapPointCloudToRegularGridFilter::className() const
{
  return FilterTraits<MapPointCloudToRegularGridFilter>::className;
}

Uuid MapPointCloudToRegularGridFilter::uuid() const
{
  return FilterTraits<MapPointCloudToRegularGridFilter>::uuid;
}

std::string MapPointCloudToRegularGridFilter::humanName() const
{
  return "Map Point Cloud to Regular Grid";
}

Parameters MapPointCloudToRegularGridFilter::parameters() const
{
  Parameters params;
  //params.insert(std::make_unique<NumericTypeParameter>(k_NumericType_Key, "Numeric Type", "Numeric Type of data to create", NumericType::int32));
  params.insert(std::make_unique<VectorInt32Parameter>(k_NumComps_Key, "Grid Dimensions", "Target grid size", std::vector<int32>{0,0,0}));
  params.insert(std::make_unique<DataPathSelectionParameter>(k_NumTuples_Key, "Image Geometry", "Path to the target Image Geometry", DataPath()));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_NumTuples_Key, "Arrays to Map", "Path to the target Image Geometry", std::vector<DataPath>()));
  params.insert(std::make_unique<BoolParameter>(k_DataPath_Key, "Use Mask", "Array storing the file data", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_DataPath_Key, "Mask", "Array storing the file data", DataPath()));
  params.insert(std::make_unique<ArrayCreationParameter>(k_DataPath_Key, "Voxel Indices", "Array storing the file data", DataPath()));
  return params;
}

IFilter::UniquePointer MapPointCloudToRegularGridFilter::clone() const
{
  return std::make_unique<MapPointCloudToRegularGridFilter>();
}

IFilter::PreflightResult MapPointCloudToRegularGridFilter::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const
{
  auto numericType = args.value<NumericType>(k_NumericType_Key);
  auto components = args.value<uint64>(k_NumComps_Key);
  auto numTuples = args.value<uint64>(k_NumTuples_Key);
  auto dataArrayPath = args.value<DataPath>(k_DataPath_Key);

  //

  std::vector<IDataArray*> dataArrays;

  VertexGeom::Pointer vertex = getDataContainerArray()->getPrereqGeometryFromDataContainer<VertexGeom>(this, getDataContainerName());

  if(getErrorCode() < 0)
  {
    return;
  }

  dataArrays.push_back(vertex->getVertices());

  if(m_CreateDataContainer == 0)
  {
    if(getGridDimensions()[0] <= 0 || getGridDimensions()[1] <= 0 || getGridDimensions()[2] <= 0)
    {
      QString ss = QObject::tr("All grid dimensions must be positive.\n "
                               "Current grid dimensions:\n x = %1\n y = %2\n z = %3\n")
                       .arg(getGridDimensions()[0])
                       .arg(getGridDimensions()[1])
                       .arg(getGridDimensions()[2]);
      setErrorCondition(-11000, ss);
    }

    if(getErrorCode() < 0)
    {
      return;
    }

    ImageGeom::Pointer image = ImageGeom::CreateGeometry(SIMPL::Geometry::ImageGeometry);
    size_t dims[3] = {static_cast<size_t>(getGridDimensions()[0]), static_cast<size_t>(getGridDimensions()[1]), static_cast<size_t>(getGridDimensions()[2])};
    image->setDimensions(dims);

    DataContainer::Pointer m = getDataContainerArray()->createNonPrereqDataContainer(this, getImageDataContainerName());

    if(getErrorCode() < 0)
    {
      return;
    }

    m->setGeometry(image);
  }

  if(m_CreateDataContainer == 1)
  {
    ImageGeom* vertex = getDataContainerArray()->getPrereqGeometryFromDataContainer<ImageGeom>(this, getImageDataContainerPath());
    if(getErrorCode() < 0)
    {
      return;
    }
  }

  std::vector<size_t> cDims(1, 1);

  voxelIndicesPtr = getDataContainerArray()->createNonPrereqArrayFromPath<USizeArray>(this, voxelIndicesArrayPath, 0, cDims);
  if(nullptr != voxelIndicesPtr)
  {
    m_VoxelIndices = m_VoxelIndicesPtr->getDataStore();
  } /* Now assign the raw pointer to data from the DataArray<T> object */
  if(getErrorCode() >= 0)
  {
    dataArrays.push_back(m_VoxelIndicesPtr.lock());
  }

  if(useMask)
  {
    maskPtr = getDataContainerArray()->getPrereqArrayFromPath<BoolArray>(this, maskArrayPath, cDims);
    if(nullptr != maskPtr)
    {
      mask = maskPtr->getDataStore();
    } /* Now assign the raw pointer to data from the DataArray<T> object */
    if(getErrorCode() >= 0)
    {
      dataArrays.push_back(maskPtr);
    }
  }

  getDataContainerArray()->validateNumberOfTuples(this, dataArrays);

  //
  auto action = std::make_unique<CreateArrayAction>(numericType, std::vector<usize>{numTuples}, components, dataArrayPath);

  OutputActions actions;
  actions.actions.push_back(std::move(action));

  return {std::move(actions)};
}

Result<> MapPointCloudToRegularGridFilter::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  auto numericType = args.value<NumericType>(k_NumericType_Key);
  auto components = args.value<uint64>(k_NumComps_Key);
  auto numTuples = args.value<uint64>(k_NumTuples_Key);
  auto path = args.value<DataPath>(k_DataPath_Key);

  switch(numericType)
  {
  case NumericType::int8: {
    auto& dataArray = ArrayFromPath<int8>(data, path);
    auto& v = *(dataArray.getDataStore());
    std::fill(v.begin(), v.end(), 0);
    break;
  }
  case NumericType::uint8: {
    auto& dataArray = ArrayFromPath<uint8>(data, path);
    auto& v = *(dataArray.getDataStore());
    std::fill(v.begin(), v.end(), 0);
    break;
  }
  case NumericType::int16: {
    auto& dataArray = ArrayFromPath<int16>(data, path);
    auto& v = *(dataArray.getDataStore());
    std::fill(v.begin(), v.end(), 0);
    break;
  }
  case NumericType::uint16: {
    auto& dataArray = ArrayFromPath<uint16>(data, path);
    auto& v = *(dataArray.getDataStore());
    std::fill(v.begin(), v.end(), 0);
    break;
  }
  case NumericType::int32: {
    auto& dataArray = ArrayFromPath<int32>(data, path);
    auto& v = *(dataArray.getDataStore());
    std::fill(v.begin(), v.end(), 0);
    break;
  }
  case NumericType::uint32: {
    auto& dataArray = ArrayFromPath<uint32>(data, path);
    auto& v = *(dataArray.getDataStore());
    std::fill(v.begin(), v.end(), 0);
    break;
  }
  case NumericType::int64: {
    auto& dataArray = ArrayFromPath<int64>(data, path);
    auto& v = *(dataArray.getDataStore());
    std::fill(v.begin(), v.end(), 0LL);
    break;
  }
  case NumericType::uint64: {
    auto& dataArray = ArrayFromPath<uint64>(data, path);
    auto& v = *(dataArray.getDataStore());
    std::fill(v.begin(), v.end(), 0ULL);
    break;
  }
  case NumericType::float32: {
    auto& dataArray = ArrayFromPath<float32>(data, path);
    auto& v = *(dataArray.getDataStore());
    std::fill(v.begin(), v.end(), 0.0f);
    break;
  }
  case NumericType::float64: {
    auto& dataArray = ArrayFromPath<float64>(data, path);
    auto& v = *(dataArray.getDataStore());
    std::fill(v.begin(), v.end(), 0.0);
    break;
  }
  default:
    throw std::runtime_error("Invalid type");
  }

  return {};
}
} // namespace complex
