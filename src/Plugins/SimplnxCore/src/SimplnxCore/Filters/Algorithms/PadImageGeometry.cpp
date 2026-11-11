#include "PadImageGeometry.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/GeometryUtilities.hpp"
#include "simplnx/Utilities/ParallelAlgorithmUtilities.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"

#include <cmath>
#include <functional>

using namespace nx::core;

namespace
{
constexpr std::array<int32, 3> k_PaddingLength = {-86016, -86017, -86018};
constexpr std::array<std::array<int32, 2>, 3> k_NegativePadding = {{{-86019, -86020}, {-86021, -86022}, {-86023, -86024}}};
constexpr int32 k_PaddedOrigin = -86027;

template <typename T>
class PadImageGeomDataArray
{
public:
  PadImageGeomDataArray(const IDataArray& oldCellArray, IDataArray& newCellArray, const ImageGeom& srcImageGeom, const SizeVec3& paddedDims, const PadImageGeometryInputValues* inputValues,
                        const std::atomic_bool& shouldCancel)
  : m_OldCellStore(oldCellArray.template getIDataStoreRefAs<AbstractDataStore<T>>())
  , m_NewCellStore(newCellArray.template getIDataStoreRefAs<AbstractDataStore<T>>())
  , m_SrcImageGeom(srcImageGeom)
  , m_PaddedDims(paddedDims)
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

    const usize newXDim = m_PaddedDims[0];
    const usize newYDim = m_PaddedDims[1];

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
  const SizeVec3 m_PaddedDims;
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
Result<SizeVec3> PadImageGeometry::ValidateInputs(const ImageGeom& srcGeom, const PadImageGeometryInputValues& inputValues)
{
  const std::array<bool, 3> active = {inputValues.PadInX, inputValues.PadInY, inputValues.PadInZ};
  const std::array<std::reference_wrapper<const VectorInt32Parameter::ValueType>, 3> vectors = {std::cref(inputValues.XMinMax), std::cref(inputValues.YMinMax), std::cref(inputValues.ZMinMax)};
  std::array<std::array<int32, 2>, 3> padding{};
  for(usize axis = 0; axis < 3; ++axis)
  {
    if(!active[axis])
    {
      continue;
    }
    const auto& values = vectors[axis].get();
    if(values.size() != 2)
    {
      return MakeErrorResult<SizeVec3>(k_PaddingLength[axis], fmt::format("Parameter '{} Min & Max Padding' must contain 2 values, but contains {}.", "XYZ"[axis], values.size()));
    }
    for(usize side = 0; side < 2; ++side)
    {
      if(values[side] < 0)
      {
        return MakeErrorResult<SizeVec3>(k_NegativePadding[axis][side],
                                         fmt::format("Parameter '{} Min & Max Padding' {} value {} must be nonnegative.", "XYZ"[axis], side == 0 ? "Min" : "Max", values[side]));
      }
      padding[axis][side] = values[side];
    }
  }
  return GeometryUtilities::ComputePaddedDimensions(srcGeom.getDimensions(), padding, active);
}

// -----------------------------------------------------------------------------
Result<> PadImageGeometry::operator()()
{

  auto& srcImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->SelectedImageGeometryPath);
  auto dimensions = ValidateInputs(srcImageGeom, *m_InputValues);
  if(dimensions.invalid())
  {
    return ConvertResult(std::move(dimensions));
  }
  if(m_InputValues->UpdateOrigin)
  {
    const auto origin = srcImageGeom.getOrigin(), spacing = srcImageGeom.getSpacing();
    const std::array<bool, 3> active = {m_InputValues->PadInX, m_InputValues->PadInY, m_InputValues->PadInZ};
    const std::array<std::reference_wrapper<const VectorInt32Parameter::ValueType>, 3> vectors = {std::cref(m_InputValues->XMinMax), std::cref(m_InputValues->YMinMax),
                                                                                                  std::cref(m_InputValues->ZMinMax)};
    for(usize axis = 0; axis < 3; ++axis)
    {
      if(!active[axis])
      {
        continue;
      }
      const auto minimum = (vectors[axis].get())[0];
      const float32 result = origin[axis] - minimum * spacing[axis];
      if(!std::isfinite(result))
      {
        return MakeErrorResult(k_PaddedOrigin, fmt::format("Parameter '{} Min & Max Padding' Min value {} with origin {} and spacing {} produces origin result {}. Use values with a finite result.",
                                                           "XYZ"[axis], minimum, origin[axis], spacing[axis], result));
      }
    }
  }

  // Nothing is changing
  if(!m_InputValues->PadInX && !m_InputValues->PadInY && !m_InputValues->PadInZ)
  {
    return {};
  }
  if(m_InputValues->PadInX && m_InputValues->XMinMax[0] == 0 && m_InputValues->XMinMax[1] == 0)
  {
    return {};
  }
  if(m_InputValues->PadInY && m_InputValues->YMinMax[0] == 0 && m_InputValues->YMinMax[1] == 0)
  {
    return {};
  }
  if(m_InputValues->PadInZ && m_InputValues->ZMinMax[0] == 0 && m_InputValues->ZMinMax[1] == 0)
  {
    return {};
  }

  // No matter where the AM is (same DC or new DC), we have the correct DC and AM pointers...now it's time to pad

  DataPath destImagePath = m_InputValues->CreatedOutputPath;
  if(m_InputValues->RemoveOriginalGeometry)
  {
    auto tempPathVector = m_InputValues->SelectedImageGeometryPath.getPathVector();
    std::string tempName = pad_image_geometry::k_TempGeometryName;
    tempPathVector.back() = tempName;
    destImagePath = DataPath({tempPathVector});
  }

  auto& destImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(destImagePath);

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
    ExecuteParallelFunction<PadImageGeomDataArray>(oldDataArray.getDataType(), taskRunner, oldDataArray, newDataArray, srcImageGeom, dimensions.value(), m_InputValues, m_ShouldCancel);
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
