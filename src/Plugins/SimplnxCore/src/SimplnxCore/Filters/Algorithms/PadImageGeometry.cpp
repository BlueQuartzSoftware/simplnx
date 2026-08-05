#include "PadImageGeometry.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/ParallelAlgorithmUtilities.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"

using namespace nx::core;

namespace
{
template <typename T>
class PadImageGeomDataArray
{
public:
  PadImageGeomDataArray(const IDataArray& oldCellArray, IDataArray& newCellArray, const ImageGeom& srcImageGeom, const PadImageGeometryInputValues* inputValues, const std::atomic_bool& shouldCancel)
  : m_OldCellStore(oldCellArray.template getIDataStoreRefAs<AbstractDataStore<T>>())
  , m_NewCellStore(newCellArray.template getIDataStoreRefAs<AbstractDataStore<T>>())
  , m_SrcImageGeom(srcImageGeom)
  , m_InputValues(inputValues)
  , m_ShouldCancel(shouldCancel)
  {
  }

  ~PadImageGeomDataArray() = default;

  PadImageGeomDataArray(const PadImageGeomDataArray&) = default;
  PadImageGeomDataArray(PadImageGeomDataArray&&) noexcept = default;
  PadImageGeomDataArray& operator=(const PadImageGeomDataArray&) = delete;
  PadImageGeomDataArray& operator=(PadImageGeomDataArray&&) noexcept = delete;

  void operator()() const
  {
    convert();
  }

protected:
  void convert() const
  {
    m_NewCellStore.fill(static_cast<T>(m_InputValues->DefaultFillValue));

    auto srcDims = m_SrcImageGeom.getDimensions();

    const size_t xDim = srcDims[0];
    const size_t yDim = srcDims[1];
    const size_t zDim = srcDims[2];

    const size_t newXDim = xDim + (m_InputValues->PadInX ? m_InputValues->XMinMax[0] + m_InputValues->XMinMax[1] : 0);
    const size_t newYDim = yDim + (m_InputValues->PadInY ? m_InputValues->YMinMax[0] + m_InputValues->YMinMax[1] : 0);

    const size_t newXOffset = m_InputValues->PadInX ? m_InputValues->XMinMax[0] : 0;
    const size_t newYOffset = m_InputValues->PadInY ? m_InputValues->YMinMax[0] : 0;
    const size_t newZOffset = m_InputValues->PadInZ ? m_InputValues->ZMinMax[0] : 0;

    // Copy one row at a time (along X axis)
    for(size_t z = 0; z < zDim; ++z)
    {
      for(size_t y = 0; y < yDim; ++y)
      {
        size_t inputStartIdx = xDim * yDim * z + xDim * y;

        size_t outIdx = (newXDim * newYDim * (z + newZOffset)) + (newXDim * (y + newYOffset)) + newXOffset;

        m_NewCellStore.copyFrom(outIdx, m_OldCellStore, inputStartIdx, xDim);
      }
    }
  }

private:
  const AbstractDataStore<T>& m_OldCellStore;
  AbstractDataStore<T>& m_NewCellStore;
  const ImageGeom& m_SrcImageGeom;
  const PadImageGeometryInputValues* m_InputValues;
  const std::atomic_bool& m_ShouldCancel;
};

} // namespace

// -----------------------------------------------------------------------------
PadImageGeometry::PadImageGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, PadImageGeometryInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
PadImageGeometry::~PadImageGeometry() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& PadImageGeometry::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> PadImageGeometry::operator()()
{

  auto& srcImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->SelectedImageGeometryPath);

  // Nothing is changing
  if(!m_InputValues->PadInX && !m_InputValues->PadInY && !m_InputValues->PadInZ)
  {
    return {};
  }
  if(m_InputValues->XMinMax[0] == 0 && m_InputValues->XMinMax[1] == 0 && m_InputValues->PadInX)
  {
    return {};
  }
  if(m_InputValues->YMinMax[0] == 0 && m_InputValues->YMinMax[1] == 0 && m_InputValues->PadInY)
  {
    return {};
  }
  if(m_InputValues->ZMinMax[0] == 0 && m_InputValues->ZMinMax[1] == 0 && m_InputValues->PadInZ)
  {
    return {};
  }

  // No matter where the AM is (same DC or new DC), we have the correct DC and AM pointers...now it's time to pad
  SizeVec3 imageDims = srcImageGeom.getDimensions();

  DataPath destImagePath = m_InputValues->CreatedOutputPath;
  if(m_InputValues->RemoveOriginalGeometry)
  {
    auto tempPathVector = m_InputValues->SelectedImageGeometryPath.getPathVector();
    std::string tempName = pad_image_geometry::k_TempGeometryName;
    tempPathVector.back() = tempName;
    destImagePath = DataPath({tempPathVector});
  }

  auto& destImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(destImagePath);
  FloatVec3 oldOrigin = destImageGeom.getOrigin();

  // The actual padding of the dataStructure arrays is done in parallel where parallel here
  // refers to the padding of each DataArray being done on a separate thread.
  ParallelTaskAlgorithm taskRunner;
  taskRunner.setParallelizationEnabled(true);
  const auto& srcCellDataAM = srcImageGeom.getCellDataRef();
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

    m_MessageHandler.sendInfoMessage(fmt::format("Padding Volume || Copying Data Array {}", srcName));
    ExecuteParallelFunction<PadImageGeomDataArray>(oldDataArray.getDataType(), taskRunner, oldDataArray, newDataArray, srcImageGeom, m_InputValues, m_ShouldCancel);
  }
  taskRunner.wait(); // This will spill over if the number of DataArrays to process does not divide evenly by the number of threads.

  if(m_ShouldCancel)
  {
    return {};
  }

  // The deferred actions will take care of removing the original and renaming the output if
  // the user decided to do the crop "in place"
  return {};
}
