#include "NearestPointFuseRegularGrids.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/ParallelAlgorithmUtilities.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"

using namespace nx::core;

namespace
{
template <class K, typename T>
void CopyData(const K& inputArray, K& destArray, const ImageGeom& sampleImageGeom, const ImageGeom& refImageGeom, const std::atomic_bool& shouldCancel, T fillValue)
{
  // Get dimensions and resolutions of two grids
  Vec3<float32> sampleRes = sampleImageGeom.getSpacing();
  Vec3<float32> refRes = refImageGeom.getSpacing();
  Vec3<float32> refOrigin = refImageGeom.getOrigin();
  Vec3<float32> sampleOrigin = sampleImageGeom.getOrigin();
  Vec3<usize> refDims = refImageGeom.getDimensions();
  Vec3<usize> sampleDims = sampleImageGeom.getDimensions();

  float32 x = 0.0f, y = 0.0f, z = 0.0f;
  usize col = 0, row = 0, plane = 0;
  usize refIndex = 0;
  usize sampleIndex = 0;
  usize planeComp = 0, rowComp = 0;
  usize numComps = destArray.getNumberOfComponents();
  for(usize i = 0; i < refDims[2]; i++)
  {
    if(shouldCancel)
    {
      return;
    }

    planeComp = i * refDims[0] * refDims[1];
    for(usize j = 0; j < refDims[1]; j++)
    {
      rowComp = j * refDims[0];
      for(usize k = 0; k < refDims[0]; k++)
      {
        refIndex = planeComp + rowComp + k;
        usize destIndex = refIndex * numComps;

        x = (k * refRes[0] + refOrigin[0]);
        y = (j * refRes[1] + refOrigin[1]);
        z = (i * refRes[2] + refOrigin[2]);
        if((x - sampleOrigin[0]) < 0)
        {
          std::fill(destArray.begin() + destIndex, destArray.begin() + destIndex + numComps, fillValue);
          continue;
        }
        if((y - sampleOrigin[1]) < 0)
        {
          std::fill(destArray.begin() + destIndex, destArray.begin() + destIndex + numComps, fillValue);
          continue;
        }
        if((z - sampleOrigin[2]) < 0)
        {
          std::fill(destArray.begin() + destIndex, destArray.begin() + destIndex + numComps, fillValue);
          continue;
        }

        col = usize((x - sampleOrigin[0]) / sampleRes[0]);
        row = usize((y - sampleOrigin[1]) / sampleRes[1]);
        plane = usize((z - sampleOrigin[2]) / sampleRes[2]);
        if(col >= sampleDims[0] || row >= sampleDims[1] || plane >= sampleDims[2])
        {
          std::fill(destArray.begin() + destIndex, destArray.begin() + destIndex + numComps, fillValue);
          continue;
        }

        sampleIndex = (plane * sampleDims[0] * sampleDims[1]) + (row * sampleDims[0]) + col;

        usize sourceIndex = sampleIndex * inputArray.getNumberOfComponents();
        for(usize offset = 0; offset < numComps; ++offset)
        {
          destArray[destIndex + offset] = inputArray.at(sourceIndex + offset);
        }
      }
    }
  }
}

template <typename T>
class CopyFromIndicesListImpl
{
public:
  CopyFromIndicesListImpl(const IArray& sourceArray, IArray& destArray, const ImageGeom& sampleImageGeom, const ImageGeom& refImageGeom, float64 fillValue, const std::atomic_bool& shouldCancel)
  : m_SourceArray(sourceArray)
  , m_DestArray(destArray)
  , m_ArrayType(destArray.getArrayType())
  , m_SampleGeom(sampleImageGeom)
  , m_RefGeom(refImageGeom)
  , m_FillValue(fillValue)
  , m_ShouldCancel(shouldCancel)
  {
  }
  ~CopyFromIndicesListImpl() = default;

  CopyFromIndicesListImpl(const CopyFromIndicesListImpl&) = default;
  CopyFromIndicesListImpl(CopyFromIndicesListImpl&&) noexcept = default;
  CopyFromIndicesListImpl& operator=(const CopyFromIndicesListImpl&) = delete;
  CopyFromIndicesListImpl& operator=(CopyFromIndicesListImpl&&) noexcept = delete;

  void operator()() const
  {
    if(m_ArrayType == IArray::ArrayType::NeighborListArray)
    {
      return;
    }
    else if(m_ArrayType == IArray::ArrayType::DataArray)
    {
      using DataArrayT = DataArray<T>;
      auto destArray = dynamic_cast<DataArrayT&>(m_DestArray);
      CopyData<DataArrayT>(dynamic_cast<const DataArrayT&>(m_SourceArray), destArray, m_SampleGeom, m_RefGeom, m_ShouldCancel, static_cast<T>(m_FillValue));
    }
    else if(m_ArrayType == IArray::ArrayType::StringArray)
    {
      CopyData<StringArray>(dynamic_cast<const StringArray&>(m_SourceArray), dynamic_cast<StringArray&>(m_DestArray), m_SampleGeom, m_RefGeom, m_ShouldCancel, "");
    }
  }

private:
  const IArray& m_SourceArray;
  IArray& m_DestArray;
  IArray::ArrayType m_ArrayType = IArray::ArrayType::Any;
  const ImageGeom& m_SampleGeom;
  const ImageGeom& m_RefGeom;
  float64 m_FillValue;
  const std::atomic_bool& m_ShouldCancel;
};
} // namespace

// -----------------------------------------------------------------------------
NearestPointFuseRegularGrids::NearestPointFuseRegularGrids(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                           NearestPointFuseRegularGridsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
NearestPointFuseRegularGrids::~NearestPointFuseRegularGrids() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& NearestPointFuseRegularGrids::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> NearestPointFuseRegularGrids::operator()()
{
  auto& refImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ReferenceGeometryPath);
  auto& sampleImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->SamplingGeometryPath);
  auto& refAM = m_DataStructure.getDataRefAs<AttributeMatrix>(m_InputValues->ReferenceCellAttributeMatrixPath);
  auto& sampleAM = m_DataStructure.getDataRefAs<AttributeMatrix>(m_InputValues->SamplingCellAttributeMatrixPath);
  Vec3<float32> sampleRes = sampleImageGeom.getSpacing();

  // Further down we divide by sampleRes, so here check to make sure that no components of the resolution are 0
  // This would be incredible unusual behavior if it were to occur.
  bool resHasZero = std::find(sampleRes.begin(), sampleRes.end(), 0.0f) != std::end(sampleRes) ? true : false;
  if(resHasZero)
  {
    return MakeErrorResult(-5555, fmt::format("A component of the resolution for the Image Geometry associated with DataContainer '{}' is 0. This would result in a division by 0 operation",
                                              m_InputValues->SamplingGeometryPath.toString()));
  }

  // Copy according to  calculated values
  ParallelTaskAlgorithm taskRunner;
  auto sampleVoxelArrays = sampleAM.findAllChildrenOfType<IArray>();
  for(const auto& array : sampleVoxelArrays)
  {
    // this path was created in preflight
    DataPath createdArrayPath = m_InputValues->ReferenceCellAttributeMatrixPath.createChildPath(array->getName());
    auto& refAMArray = m_DataStructure.getDataRefAs<IArray>(createdArrayPath);

    DataType dataType = DataType::int32;
    if(refAMArray.getArrayType() == IArray::ArrayType::DataArray)
    {
      dataType = dynamic_cast<IDataArray&>(refAMArray).getDataType();
    }

    ExecuteParallelFunction<CopyFromIndicesListImpl>(dataType, taskRunner, *array, refAMArray, sampleImageGeom, refImageGeom, m_InputValues->fillValue, getCancel());
  }
  taskRunner.wait();

  return {};
}
