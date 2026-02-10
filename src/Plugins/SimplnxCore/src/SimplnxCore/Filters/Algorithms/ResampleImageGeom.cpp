#include "ResampleImageGeom.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/ParallelAlgorithmUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/SamplingUtils.hpp"

using namespace nx::core;

namespace
{
// -----------------------------------------------------------------------------
template <typename T>
class ResampleImageGeomArrayImpl
{
public:
  ResampleImageGeomArrayImpl(const IDataArray& srcArray, IDataArray& destArray, const FloatVec3& newSpacing, const FloatVec3& origSpacing, const SizeVec3& origDims, const SizeVec3& destDims,
                             const std::atomic_bool& shouldCancel)
  : m_SrcArray(srcArray)
  , m_DestArray(destArray)
  , m_NewSpacing(newSpacing)
  , m_OrigSpacing(origSpacing)
  , m_OrigDims(origDims)
  , m_DestDims(destDims)
  , m_ShouldCancel(shouldCancel)
  {
  }

  void operator()(const Range& range) const
  {
    const auto& srcDataStore = m_SrcArray.template getIDataStoreRefAs<AbstractDataStore<T>>();
    auto& destDataStore = m_DestArray.template getIDataStoreRefAs<AbstractDataStore<T>>();

    for(usize idx = range.min(); idx < range.max(); idx++)
    {
      if(m_ShouldCancel)
      {
        return;
      }

      // Decompose linear index to 3D (z-slowest, x-fastest)
      usize z = idx / (m_DestDims[0] * m_DestDims[1]);
      usize rem = idx % (m_DestDims[0] * m_DestDims[1]);
      usize y = rem / m_DestDims[0];
      usize x = rem % m_DestDims[0];

      // Compute source voxel coordinates
      auto col = static_cast<int64>(static_cast<float32>(x) * m_NewSpacing[0] / m_OrigSpacing[0]);
      auto row = static_cast<int64>(static_cast<float32>(y) * m_NewSpacing[1] / m_OrigSpacing[1]);
      auto plane = static_cast<int64>(static_cast<float32>(z) * m_NewSpacing[2] / m_OrigSpacing[2]);

      int64 srcIndex = static_cast<int64>(plane * m_OrigDims[1] * m_OrigDims[0]) + static_cast<int64>(row * m_OrigDims[0]) + col;

      if(srcIndex >= 0)
      {
        destDataStore.copyFrom(idx, srcDataStore, static_cast<usize>(srcIndex), 1);
      }
      else
      {
        destDataStore.fillTuple(idx, 0);
      }
    }
  }

private:
  const IDataArray& m_SrcArray;
  IDataArray& m_DestArray;
  FloatVec3 m_NewSpacing;
  FloatVec3 m_OrigSpacing;
  SizeVec3 m_OrigDims;
  SizeVec3 m_DestDims;
  const std::atomic_bool& m_ShouldCancel;
};
} // namespace

// -----------------------------------------------------------------------------
ResampleImageGeom::ResampleImageGeom(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, ResampleImageGeomInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(msgHandler)
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
  const auto& selectedImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->SelectedImageGeometryPath);
  SizeVec3 sourceDims = selectedImageGeom.getDimensions();
  FloatVec3 origSpacing = selectedImageGeom.getSpacing();

  auto& destImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->CreatedImageGeometryPath);
  SizeVec3 destDims = destImageGeom.getDimensions();

  FloatVec3 newSpacing = {m_InputValues->Spacing[0], m_InputValues->Spacing[1], m_InputValues->Spacing[2]};
  usize totalDestTuples = destDims[0] * destDims[1] * destDims[2];

  const auto& srcCellDataAM = selectedImageGeom.getCellDataRef();
  auto& destCellDataAM = destImageGeom.getCellDataRef();

  for(const auto& [dataId, oldDataObject] : srcCellDataAM)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    const auto& oldDataArray = dynamic_cast<const IDataArray&>(*oldDataObject);
    const std::string srcName = oldDataArray.getName();
    auto& newDataArray = dynamic_cast<IDataArray&>(destCellDataAM.at(srcName));
    m_MessageHandler(fmt::format("Resample Volume || Resampling Data Array {}", srcName));

    ParallelDataAlgorithm dataAlg;
    dataAlg.setRange(0, totalDestTuples);
    ExecuteParallelFunction<ResampleImageGeomArrayImpl>(oldDataArray.getDataType(), dataAlg, oldDataArray, newDataArray, newSpacing, origSpacing, sourceDims, destDims, m_ShouldCancel);
  }

  if(m_ShouldCancel)
  {
    return {};
  }

  // Careful with this next section. We purposefully copy in the original dataStructure arrays
  // into the destination feature attribute matrix so that we have somewhere to start.
  // During the renumbering phase is when those copied arrays will get potentially resized
  // to their proper number of tuples.
  DataPath cellFeatureAMPath = m_InputValues->CellFeatureAttributeMatrix;
  auto destImagePath = m_InputValues->CreatedImageGeometryPath;
  DataPath featureIdsArrayPath = m_InputValues->FeatureIdsArrayPath;

  if(m_InputValues->RenumberFeatures)
  {
    const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(featureIdsArrayPath);
    auto validateNumFeatResult = ValidateFeatureIdsToFeatureAttributeMatrixIndexing(m_DataStructure, cellFeatureAMPath, featureIds, false, m_MessageHandler);
    if(validateNumFeatResult.invalid())
    {
      return validateNumFeatResult;
    }

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
    return Sampling::RenumberFeatures(m_DataStructure, destImagePath, destCellFeatureAMPath, featureIdsArrayPath, destFeatureIdsPath, m_MessageHandler, m_ShouldCancel);
  }

  return {};
}
