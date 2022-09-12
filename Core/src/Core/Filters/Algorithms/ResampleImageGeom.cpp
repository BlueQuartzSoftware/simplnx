

#include "ResampleImageGeom.hpp"

#include "Core/Utilities/CoreUtilities.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Utilities/ParallelData3DAlgorithm.hpp"
#include "complex/Utilities/ParallelDataAlgorithm.hpp"
#include "complex/Utilities/SamplingUtils.hpp"
#include "complex/Utilities/StringUtilities.hpp"

#ifdef COMPLEX_ENABLE_MULTICORE
#define RUN_TASK g->run
#else
#define RUN_TASK
#endif

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
  void compute(size_t zStart, size_t zEnd, size_t yStart, size_t yEnd, size_t xStart, size_t xEnd) const
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
          int64 index_old = (plane * m_OrigDims[1] * m_OrigDims[0]) + (row * m_OrigDims[0]) + col;
          size_t index = static_cast<size_t>((i * m_CopyDims[0] * m_CopyDims[1]) + (j * m_CopyDims[0]) + k);
          m_NewIndices[index] = index_old;
        }
      }
    }
  }

  void operator()(const ComplexRange3D& r) const
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

  const auto& destImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->newDataContainerPath);
  SizeVec3 destDims = destImageGeom.getDimensions();

  // Allow data-based parallelization
  size_t grain = destDims[2] == 1 ? 1 : destDims[2] / std::thread::hardware_concurrency();
  if(grain == 0) // This can happen if dims[2] > number of processors
  {
    grain = 1;
  }
  std::vector<int64> newIndices(destDims[2] * destDims[1] * destDims[0]);
  ParallelData3DAlgorithm dataAlg;
  dataAlg.setRange(destDims[0], destDims[1], destDims[2]);
  dataAlg.setGrain(grain);
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

#ifdef COMPLEX_ENABLE_MULTICORE
  std::shared_ptr<tbb::task_group> g(new tbb::task_group);
  // C++11 RIGHT HERE....
  int32_t nthreads = static_cast<int32_t>(std::thread::hardware_concurrency()); // Returns ZERO if not defined on this platform
  int32_t threadCount = 0;
#endif

  for(const auto& cellArrayPath : selectedCellArrays)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    m_MessageHandler(fmt::format("Resampling {}", cellArrayPath.toString()));
    const auto& oldCellArray = m_DataStructure.getDataRefAs<IDataArray>(cellArrayPath);
    std::string aPath = cellArrayPath.toString();
    aPath = complex::StringUtilities::replace(aPath, m_InputValues->inputImageGeometry.toString(), m_InputValues->newDataContainerPath.toString());
    DataPath createdArrayPath = DataPath::FromString(aPath).value(); // createdImageGeomPath.createChildPath(cellArray.getName());
    auto& newCellArray = m_DataStructure.getDataRefAs<IDataArray>(createdArrayPath);
    DataType type = oldCellArray.getDataType();

    switch(type)
    {
    case DataType::boolean: {
      RUN_TASK(CopyTupleUsingIndexList<bool>(oldCellArray, newCellArray, newIndices));
      break;
    }
    case DataType::int8: {
      RUN_TASK(CopyTupleUsingIndexList<int8>(oldCellArray, newCellArray, newIndices));
      break;
    }
    case DataType::int16: {
      RUN_TASK(CopyTupleUsingIndexList<int16>(oldCellArray, newCellArray, newIndices));
      break;
    }
    case DataType::int32: {
      RUN_TASK(CopyTupleUsingIndexList<int32>(oldCellArray, newCellArray, newIndices));
      break;
    }
    case DataType::int64: {
      RUN_TASK(CopyTupleUsingIndexList<int64>(oldCellArray, newCellArray, newIndices));
      break;
    }
    case DataType::uint8: {
      RUN_TASK(CopyTupleUsingIndexList<uint8>(oldCellArray, newCellArray, newIndices));
      break;
    }
    case DataType::uint16: {
      RUN_TASK(CopyTupleUsingIndexList<uint16>(oldCellArray, newCellArray, newIndices));
      break;
    }
    case DataType::uint32: {
      RUN_TASK(CopyTupleUsingIndexList<uint32>(oldCellArray, newCellArray, newIndices));
      break;
    }
    case DataType::uint64: {
      RUN_TASK(CopyTupleUsingIndexList<uint64>(oldCellArray, newCellArray, newIndices));
      break;
    }
    case DataType::float32: {
      RUN_TASK(CopyTupleUsingIndexList<float32>(oldCellArray, newCellArray, newIndices));
      break;
    }
    case DataType::float64: {
      RUN_TASK(CopyTupleUsingIndexList<float64>(oldCellArray, newCellArray, newIndices));
      break;
    }
    default: {
      throw std::runtime_error("Invalid DataType");
    }
    }
#ifdef COMPLEX_ENABLE_MULTICORE
    threadCount++;
    if(threadCount == nthreads)
    {
      g->wait();
      threadCount = 0;
    }
#endif
  }

#ifdef COMPLEX_ENABLE_MULTICORE
  // This will spill over if the number of DataArrays to process does not divide evenly by the number of threads.
  g->wait();
#endif

  if(m_InputValues->renumberFeatures)
  {
    const AttributeMatrix* srcCellFeaturData = m_DataStructure.getDataAs<AttributeMatrix>(m_InputValues->cellFeatureAttributeMatrix);
    DataPath destCellFeaturesPath = m_InputValues->newDataContainerPath.createChildPath(m_InputValues->cellFeatureAttributeMatrix.getTargetName());
    AttributeMatrix& cellFeaturData = m_DataStructure.getDataRefAs<AttributeMatrix>(destCellFeaturesPath);
    for(const auto& [id, object] : cellFeaturData)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      auto& newDataArray = dynamic_cast<IDataArray&>(*object);
      const auto& oldDataArray = m_DataStructure.getDataRefAs<const IDataArray>(m_InputValues->cellFeatureAttributeMatrix.createChildPath(newDataArray.getName()));

      for(usize i = 0; i < oldDataArray.getNumberOfTuples(); ++i)
      {
        auto dataType = oldDataArray.getDataType();
        switch(dataType)
        {
        case DataType::boolean:
          copyDataTuple<bool>(oldDataArray, newDataArray, i, i);
          break;
        case DataType::float32:
          copyDataTuple<float32>(oldDataArray, newDataArray, i, i);
          break;
        case DataType::float64:
          copyDataTuple<float64>(oldDataArray, newDataArray, i, i);
          break;
        case DataType::int8:
          copyDataTuple<int8>(oldDataArray, newDataArray, i, i);
          break;
        case DataType::int16:
          copyDataTuple<int16>(oldDataArray, newDataArray, i, i);
          break;
        case DataType::int32:
          copyDataTuple<int32>(oldDataArray, newDataArray, i, i);
          break;
        case DataType::int64:
          copyDataTuple<int64>(oldDataArray, newDataArray, i, i);
          break;
        case DataType::uint8:
          copyDataTuple<uint8>(oldDataArray, newDataArray, i, i);
          break;
        case DataType::uint16:
          copyDataTuple<uint16>(oldDataArray, newDataArray, i, i);
          break;
        case DataType::uint32:
          copyDataTuple<uint32>(oldDataArray, newDataArray, i, i);
          break;
        case DataType::uint64:
          copyDataTuple<uint64>(oldDataArray, newDataArray, i, i);
          break;
        default:
          break;
        }
      }
    }
    DataPath destFeatureIdsPath = m_InputValues->newDataContainerPath.createChildPath(cellDataGroup.getName()).createChildPath(m_InputValues->featureIdsArrayPath.getTargetName());
    auto result = Sampling::RenumberFeatures(m_DataStructure, m_InputValues->newDataContainerPath, destCellFeaturesPath, m_InputValues->featureIdsArrayPath, destFeatureIdsPath, m_ShouldCancel);
    if(result.invalid())
    {
      return result;
    }
  }

  if(m_InputValues->removeOriginalImageGeom)
  {
    return MakeErrorResult(-23500, "Removing Original Geometry Group is not Implemented.");
  }

  return {};
}
