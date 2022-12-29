#include "ResampleImageGeom.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/StringArray.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"
#include "complex/Utilities/ParallelData3DAlgorithm.hpp"
#include "complex/Utilities/ParallelDataAlgorithm.hpp"
#include "complex/Utilities/ParallelTaskAlgorithm.hpp"
#include "complex/Utilities/SamplingUtils.hpp"
#include "complex/Utilities/StringUtilities.hpp"

using namespace complex;

namespace
{
// -----------------------------------------------------------------------------
class ChangeResolutionImpl
{
public:
  ChangeResolutionImpl(ResampleImageGeom* filter, std::vector<int64>& newindices, std::vector<float> spacing, FloatVec3 sourceSpacing, SizeVec3 sourceDims, SizeVec3 destDims)
  : m_Filter(filter)
  , m_NewIndices(newindices)
  , m_Spacing(std::move(spacing))
  , m_OrigSpacing(std::move(sourceSpacing))
  , m_OrigDims(std::move(sourceDims))
  , m_CopyDims(std::move(destDims))
  {
  }
  ~ChangeResolutionImpl() = default;

  // -----------------------------------------------------------------------------
  void compute(size_t xStart, size_t xEnd, size_t yStart, size_t yEnd, size_t zStart, size_t zEnd) const
  {
    for(size_t i = zStart; i < zEnd; i++)
    {
      for(size_t j = yStart; j < yEnd; j++)
      {
        if(m_Filter->getCancel())
        {
          return;
        }

        for(size_t k = xStart; k < xEnd; k++)
        {
          float x = (k * m_Spacing[0]);
          float y = (j * m_Spacing[1]);
          float z = (i * m_Spacing[2]);
          int64 col = static_cast<int64>(x / m_OrigSpacing[0]);
          int64 row = static_cast<int64>(y / m_OrigSpacing[1]);
          int64 plane = static_cast<int64>(z / m_OrigSpacing[2]);
          int64 indexOld = (plane * m_OrigDims[1] * m_OrigDims[0]) + (row * m_OrigDims[0]) + col;
          size_t index = static_cast<size_t>((i * m_CopyDims[0] * m_CopyDims[1]) + (j * m_CopyDims[0]) + k);
          m_NewIndices[index] = indexOld;
        }
      }
    }
  }

  void operator()(const Range3D& r) const
  {
    compute(r[0], r[1], r[2], r[3], r[4], r[5]);
  }

private:
  ResampleImageGeom* m_Filter = nullptr;
  std::vector<int64>& m_NewIndices;
  std::vector<float> m_Spacing;
  FloatVec3 m_OrigSpacing;
  SizeVec3 m_OrigDims;
  SizeVec3 m_CopyDims;
};

template <typename T>
void CopyDataTuple(const IDataArray& oldArray, IDataArray& newArray, usize oldIndex, usize newIndex)
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

} // namespace

// -----------------------------------------------------------------------------
ResampleImageGeom::ResampleImageGeom(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ResampleImageGeomInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ResampleImageGeom::~ResampleImageGeom() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ResampleImageGeom::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ResampleImageGeom::operator()()
{
  m_MessageHandler(IFilter::Message::Type::Info, "Computing new indices...");

  const auto& selectedImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->inputImageGeometry);
  SizeVec3 sourceDims = selectedImageGeom.getDimensions();
  FloatVec3 origSpacing = selectedImageGeom.getSpacing();

  auto& destImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->newDataContainerPath);
  SizeVec3 destDims = destImageGeom.getDimensions();

  std::vector<int64> newIndices(destDims[2] * destDims[1] * destDims[0]);
  ParallelData3DAlgorithm dataAlg;
  dataAlg.setRange(destDims[0], destDims[1], destDims[2]);
  dataAlg.setParallelizationEnabled(true);
  dataAlg.execute(ChangeResolutionImpl(this, newIndices, m_InputValues->spacing, origSpacing, sourceDims, destDims));

  auto cellDataGroupPath = m_InputValues->cellDataGroupPath;
  auto& cellDataGroup = m_DataStructure.getDataRefAs<AttributeMatrix>(cellDataGroupPath);
  std::vector<DataPath> selectedCellArrays;

  // Create the vector of selected cell DataPaths
  for(DataGroup::Iterator child = cellDataGroup.begin(); child != cellDataGroup.end(); ++child)
  {
    selectedCellArrays.push_back(m_InputValues->cellDataGroupPath.createChildPath(child->second->getName()));
  }

  // The actual cropping of the dataStructure arrays is done in parallel where parallel here
  // refers to the cropping of each DataArray being done on a separate thread.
  ParallelTaskAlgorithm taskRunner;
  const auto& srcCellDataAM = selectedImageGeom.getCellDataRef();
  auto& destCellDataAM = destImageGeom.getCellDataRef();

  // copy over/resample the cell data
  for(const auto& [dataId, oldDataObject] : srcCellDataAM)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    const auto& oldDataArray = dynamic_cast<const IDataArray&>(*oldDataObject);
    const std::string srcName = oldDataArray.getName();

    auto& newDataArray = dynamic_cast<IDataArray&>(destCellDataAM.at(srcName));

    m_MessageHandler(fmt::format("Resample Volume || Copying Dat Array {}", srcName));

    DataType type = oldDataArray.getDataType();

    switch(type)
    {
    case DataType::boolean: {
      taskRunner.execute(CopyTupleUsingIndexList<bool>(oldDataArray, newDataArray, newIndices));
      break;
    }
    case DataType::int8: {
      taskRunner.execute(CopyTupleUsingIndexList<int8>(oldDataArray, newDataArray, newIndices));
      break;
    }
    case DataType::int16: {
      taskRunner.execute(CopyTupleUsingIndexList<int16>(oldDataArray, newDataArray, newIndices));
      break;
    }
    case DataType::int32: {
      taskRunner.execute(CopyTupleUsingIndexList<int32>(oldDataArray, newDataArray, newIndices));
      break;
    }
    case DataType::int64: {
      taskRunner.execute(CopyTupleUsingIndexList<int64>(oldDataArray, newDataArray, newIndices));
      break;
    }
    case DataType::uint8: {
      taskRunner.execute(CopyTupleUsingIndexList<uint8>(oldDataArray, newDataArray, newIndices));
      break;
    }
    case DataType::uint16: {
      taskRunner.execute(CopyTupleUsingIndexList<uint16>(oldDataArray, newDataArray, newIndices));
      break;
    }
    case DataType::uint32: {
      taskRunner.execute(CopyTupleUsingIndexList<uint32>(oldDataArray, newDataArray, newIndices));
      break;
    }
    case DataType::uint64: {
      taskRunner.execute(CopyTupleUsingIndexList<uint64>(oldDataArray, newDataArray, newIndices));
      break;
    }
    case DataType::float32: {
      taskRunner.execute(CopyTupleUsingIndexList<float32>(oldDataArray, newDataArray, newIndices));
      break;
    }
    case DataType::float64: {
      taskRunner.execute(CopyTupleUsingIndexList<float64>(oldDataArray, newDataArray, newIndices));
      break;
    }
    default: {
      throw std::runtime_error("Invalid DataType");
    }
    }
  }

  taskRunner.wait(); // This will spill over if the number of DataArrays to process does not divide evenly by the number of threads.
  if(m_ShouldCancel)
  {
    return {};
  }

  // Careful with this next section. We purposefully copy in the original dataStructure arrays
  // into the destination feature attribute matrix so that we have somewhere to start.
  // During the renumbering phase is when those copied arrays will get potentially resized
  // to their proper number of tuples.
  DataPath cellFeatureAMPath = m_InputValues->cellFeatureAttributeMatrix;
  auto destImagePath = m_InputValues->newDataContainerPath;
  DataPath featureIdsArrayPath = m_InputValues->featureIdsArrayPath;

  if(m_InputValues->renumberFeatures)
  {
    std::vector<DataPath> sourceFeatureDataPaths;
    auto childPathsResult = GetAllChildArrayDataPaths(m_DataStructure, cellFeatureAMPath);
    if(childPathsResult.has_value())
    {
      sourceFeatureDataPaths = childPathsResult.value();
    }
    std::vector<DataPath> destFeatureDataPaths = sourceFeatureDataPaths;
    DataPath destCellFeatureAMPath = destImagePath.createChildPath(cellFeatureAMPath.getTargetName());

    for(auto& dataPath : destFeatureDataPaths)
    {
      dataPath = destCellFeatureAMPath.createChildPath(dataPath.getTargetName());
    }

    // Loop over all the DataPaths and do a deep copy on each DataArray|StringArray
    // so that the updating of the Feature level data can happen. We do a bit of
    // under-the-covers where we actually remove the existing array that preflight
    // created, so we can use the convenience of the DataArray.deepCopy() function.
    for(size_t index = 0; index < sourceFeatureDataPaths.size(); index++)
    {
      DataObject* dataObject = m_DataStructure.getData(sourceFeatureDataPaths[index]);
      if(dataObject->getDataObjectType() == DataObject::Type::DataArray)
      {
        auto result = DeepCopy<IDataArray>(m_DataStructure, sourceFeatureDataPaths[index], destFeatureDataPaths[index]);
        if(result.invalid())
        {
          return result;
        }
      }
      else if(dataObject->getDataObjectType() == DataObject::Type::StringArray)
      {
        auto result = DeepCopy<StringArray>(m_DataStructure, sourceFeatureDataPaths[index], destFeatureDataPaths[index]);
        if(result.invalid())
        {
          return result;
        }
      }
    }

    // NOW DO THE ACTUAL RENUMBERING and updating.
    DataPath destFeatureIdsPath = destImagePath.createChildPath(srcCellDataAM.getName()).createChildPath(featureIdsArrayPath.getTargetName());
    return Sampling::RenumberFeatures(m_DataStructure, destImagePath, destCellFeatureAMPath, featureIdsArrayPath, destFeatureIdsPath, m_ShouldCancel);
  }

  return {};
}
