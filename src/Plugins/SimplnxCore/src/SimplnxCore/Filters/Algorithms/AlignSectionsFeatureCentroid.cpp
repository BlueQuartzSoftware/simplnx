#include <array>
#include <cmath>

#include "AlignSectionsFeatureCentroid.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"

using namespace nx::core;

namespace
{
int64 ComputeCentroidShift(float32 centroid, float32 referenceCentroid, float32 spacing)
{
  // Empty mask slices have NaN centroids and contribute no relative shift.
  if(!std::isfinite(centroid) || !std::isfinite(referenceCentroid))
  {
    return 0;
  }
  return static_cast<int64>((centroid - referenceCentroid) / spacing);
}
} // namespace

AlignSectionsFeatureCentroid::AlignSectionsFeatureCentroid(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                           AlignSectionsFeatureCentroidInputValues* inputValues)
: AlignSections(dataStructure, shouldCancel, mesgHandler)
, m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

AlignSectionsFeatureCentroid::~AlignSectionsFeatureCentroid() noexcept = default;

Result<> AlignSectionsFeatureCentroid::operator()()
{
  if(m_ShouldCancel)
  {
    return {};
  }
  const auto& gridGeom = m_DataStructure.getDataRefAs<IGridGeometry>(m_InputValues->image_geometry_path);

  return execute(gridGeom.getDimensions(), m_InputValues->image_geometry_path);
}

Result<> AlignSectionsFeatureCentroid::findShifts(std::vector<int64>& xShifts, std::vector<int64>& yShifts)
{
  bool usesOutOfCoreStore = false;
  // Select storage-neutral bulk access before MaskCompare can perform element access.
  {
    const auto& maskCheck = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->mask_array_path);
    usesOutOfCoreStore = IsOutOfCore(maskCheck);
    if(!ForceInCoreAlgorithm() && (ForceOocAlgorithm() || usesOutOfCoreStore))
    {
      RecordAlgorithmPathExecution(AlgorithmPath::OutOfCore, usesOutOfCoreStore);
      return findShiftsOoc(xShifts, yShifts);
    }
  }

  RecordAlgorithmPathExecution(AlgorithmPath::InCore, usesOutOfCoreStore);

  std::unique_ptr<mask_compare_utilities::MaskCompare> maskCompare;
  try
  {
    maskCompare = mask_compare_utilities::InstantiateMaskCompare(m_DataStructure, m_InputValues->mask_array_path);
  } catch(const std::out_of_range& exception)
  {
    // Direct callers can bypass preflight, so return an invalid mask as a Result.
    std::string message = fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->mask_array_path.toString());
    return MakeErrorResult(-53900, message);
  }

  auto* gridGeomPtr = m_DataStructure.getDataAs<ImageGeom>(m_InputValues->image_geometry_path);

  SizeVec3Type dims = gridGeomPtr->getDimensions();

  std::array<int64, 3> sdims = {
      static_cast<int64>(dims[0]),
      static_cast<int64>(dims[1]),
      static_cast<int64>(dims[2]),
  };

  int32 progInt = 0;

  usize slice = 0;
  usize point = 0;
  nx::core::FloatVec3Type spacing = gridGeomPtr->getSpacing();
  std::vector<float32> xCentroid(dims[2], 0.0f);
  std::vector<float32> yCentroid(dims[2], 0.0f);

  ThrottledMessageHandler progressThrottle(getThreadSafeMessageHandler());
  // Traverse source slices from highest Z to lowest Z.
  for(usize iter = 0; iter < dims[2]; iter++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    progressThrottle.updatePercent("Determining Shifts", iter, dims[2]);

    usize count = 0;
    xCentroid[iter] = 0;
    yCentroid[iter] = 0;

    slice = static_cast<usize>((dims[2] - 1) - iter);
    for(usize l = 0; l < dims[1]; l++)
    {
      for(usize n = 0; n < dims[0]; n++)
      {
        point = ((slice)*dims[0] * dims[1]) + (l * dims[0]) + n;

        if(maskCompare->isTrue(point))
        {
          xCentroid[iter] = xCentroid[iter] + (static_cast<float32>(n) * spacing[0]);
          yCentroid[iter] = yCentroid[iter] + (static_cast<float32>(l) * spacing[1]);
          count++;
        }
      }
    }
    xCentroid[iter] = xCentroid[iter] / static_cast<float32>(count);
    yCentroid[iter] = yCentroid[iter] / static_cast<float32>(count);
  }

  bool xWarning = false;
  bool yWarning = false;
  if(m_InputValues->store_alignment_shifts)
  {
    // These unsigned intermediates preserve current direct-path behavior.
    // A negative relative shift can wrap before it reaches signed output storage.
    usize relativexshift = 0;
    usize relativeyshift = 0;

    auto& slicesStore = m_DataStructure.getDataAs<UInt32ArrayType>(m_InputValues->slices_array_path)->getDataStoreRef();
    auto& relativeShiftsStore = m_DataStructure.getDataAs<Int64ArrayType>(m_InputValues->relative_shifts_array_path)->getDataStoreRef();
    auto& cumulativeShiftsStore = m_DataStructure.getDataAs<Int64ArrayType>(m_InputValues->cumulative_shifts_array_path)->getDataStoreRef();
    auto& centroidsStore = m_DataStructure.getDataAs<Float32ArrayType>(m_InputValues->centroids_array_path)->getDataStoreRef();

    // Convert physical centroid differences to integer voxel offsets.
    for(usize iter = (m_InputValues->use_reference_slice && m_InputValues->reference_slice != 0) ? 0 : 1; iter < dims[2]; iter++)
    {
      slice = (dims[2] - 1) - iter;
      if(m_InputValues->use_reference_slice)
      {
        // Reference-relative and cumulative shifts are identical in this mode.
        relativexshift = ComputeCentroidShift(xCentroid[iter], xCentroid[static_cast<usize>(m_InputValues->reference_slice)], spacing[0]);
        relativeyshift = ComputeCentroidShift(yCentroid[iter], yCentroid[static_cast<usize>(m_InputValues->reference_slice)], spacing[1]);
        xShifts[iter] = relativexshift;
        yShifts[iter] = relativeyshift;
      }
      else
      {
        // Accumulate consecutive-slice offsets.
        relativexshift = ComputeCentroidShift(xCentroid[iter], xCentroid[iter - 1], spacing[0]);
        relativeyshift = ComputeCentroidShift(yCentroid[iter], yCentroid[iter - 1], spacing[1]);
        xShifts[iter] = xShifts[iter - 1] + relativexshift;
        yShifts[iter] = yShifts[iter - 1] + relativeyshift;
      }

      if((xShifts[iter] < -sdims[0] || xShifts[iter] > sdims[0]) && !xWarning)
      {
        std::string message = fmt::format("A shift was greater than the X dimension of the Image Geometry. "
                                          "All subsequent slices are probably wrong. Slice={}  X Dim={}  X Shift={}  sDims[0]={}",
                                          iter, dims[0], xShifts[iter], sdims[0]);
        m_MessageHandler.sendInfoMessage(message);
        xWarning = true;
      }
      if((yShifts[iter] < -sdims[1] || yShifts[iter] > sdims[1]) && !yWarning)
      {
        std::string message = fmt::format("A shift was greater than the Y dimension of the Image Geometry. "
                                          "All subsequent slices are probably wrong. Slice={}  Y Dim={}  Y Shift={}  sDims[1]={}",
                                          iter, dims[1], yShifts[iter], sdims[1]);
        m_MessageHandler.sendInfoMessage(message);
        yWarning = true;
      }
      if(std::isnan(xCentroid[iter]) && !xWarning)
      {
        std::string message = fmt::format("The X Centroid was NaN. All subsequent slices are probably wrong. Slice={}", iter);
        m_MessageHandler.sendInfoMessage(message);
        xWarning = true;
      }
      if(std::isnan(yCentroid[iter]) && !yWarning)
      {
        std::string message = fmt::format("The Y Centroid was NaN. All subsequent slices are probably wrong. Slice={}", iter);
        m_MessageHandler.sendInfoMessage(message);
        yWarning = true;
      }

      usize xIndex = iter * 2;
      usize yIndex = (iter * 2) + 1;
      slicesStore[xIndex] = slice;
      slicesStore[yIndex] = iter == 0 ? dims[2] - 1 - static_cast<usize>(m_InputValues->reference_slice) : slice + 1;
      relativeShiftsStore[xIndex] = relativexshift;
      relativeShiftsStore[yIndex] = relativeyshift;
      cumulativeShiftsStore[xIndex] = xShifts[iter];
      cumulativeShiftsStore[yIndex] = yShifts[iter];
      centroidsStore[xIndex] = xCentroid[iter];
      centroidsStore[yIndex] = yCentroid[iter];
    }
  }
  else
  {
    // Compute shifts without writing diagnostic arrays.
    for(usize iter = (m_InputValues->use_reference_slice && m_InputValues->reference_slice != 0) ? 0 : 1; iter < dims[2]; iter++)
    {
      if(m_InputValues->use_reference_slice)
      {
        xShifts[iter] = ComputeCentroidShift(xCentroid[iter], xCentroid[static_cast<usize>(m_InputValues->reference_slice)], spacing[0]);
        yShifts[iter] = ComputeCentroidShift(yCentroid[iter], yCentroid[static_cast<usize>(m_InputValues->reference_slice)], spacing[1]);
      }
      else
      {
        xShifts[iter] = xShifts[iter - 1] + ComputeCentroidShift(xCentroid[iter], xCentroid[iter - 1], spacing[0]);
        yShifts[iter] = yShifts[iter - 1] + ComputeCentroidShift(yCentroid[iter], yCentroid[iter - 1], spacing[1]);
      }

      if((xShifts[iter] < -sdims[0] || xShifts[iter] > sdims[0]) && !xWarning)
      {
        std::string message = fmt::format("A shift was greater than the X dimension of the Image Geometry. "
                                          "All subsequent slices are probably wrong. Slice={}  X Dim={}  X Shift={}  sDims[0]={}",
                                          iter, dims[0], xShifts[iter], sdims[0]);
        m_MessageHandler.sendInfoMessage(message);
        xWarning = true;
      }
      if((yShifts[iter] < -sdims[1] || yShifts[iter] > sdims[1]) && !yWarning)
      {
        std::string message = fmt::format("A shift was greater than the Y dimension of the Image Geometry. "
                                          "All subsequent slices are probably wrong. Slice={}  Y Dim={}  Y Shift={}  sDims[1]={}",
                                          iter, dims[1], yShifts[iter], sdims[1]);
        m_MessageHandler.sendInfoMessage(message);
        yWarning = true;
      }
      if(std::isnan(xCentroid[iter]) && !xWarning)
      {
        std::string message = fmt::format("The X Centroid was NaN. All subsequent slices are probably wrong. Slice={}", iter);
        m_MessageHandler.sendInfoMessage(message);
        xWarning = true;
      }
      if(std::isnan(yCentroid[iter]) && !yWarning)
      {
        std::string message = fmt::format("The Y Centroid was NaN. All subsequent slices are probably wrong. Slice={}", iter);
        m_MessageHandler.sendInfoMessage(message);
        yWarning = true;
      }
    }
  }

  return {};
}

Result<> AlignSectionsFeatureCentroid::findShiftsOoc(std::vector<int64>& xShifts, std::vector<int64>& yShifts)
{
  // Resolve Bool or UInt8 storage for one-slice bulk reads.
  const auto& maskArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->mask_array_path);
  const AbstractDataStore<uint8>* maskUInt8StorePtr = nullptr;
  const AbstractDataStore<bool>* maskBoolStorePtr = nullptr;
  if(maskArray.getDataType() == DataType::uint8)
  {
    maskUInt8StorePtr = &dynamic_cast<const DataArray<uint8>&>(maskArray).getDataStoreRef();
  }
  else if(maskArray.getDataType() == DataType::boolean)
  {
    maskBoolStorePtr = &dynamic_cast<const DataArray<bool>&>(maskArray).getDataStoreRef();
  }
  else
  {
    return MakeErrorResult(-53900, fmt::format("Mask Array is not Bool or UInt8: {}", m_InputValues->mask_array_path.toString()));
  }

  auto* gridGeomPtr = m_DataStructure.getDataAs<ImageGeom>(m_InputValues->image_geometry_path);
  SizeVec3Type dims = gridGeomPtr->getDimensions();

  std::array<int64, 3> sdims = {
      static_cast<int64>(dims[0]),
      static_cast<int64>(dims[1]),
      static_cast<int64>(dims[2]),
  };

  nx::core::FloatVec3Type spacing = gridGeomPtr->getSpacing();
  std::vector<float32> xCentroid(dims[2], 0.0f);
  std::vector<float32> yCentroid(dims[2], 0.0f);

  const usize sliceVoxels = dims[0] * dims[1];
  std::vector<uint8> maskBuf(sliceVoxels);

  // Read and reduce one mask slice at a time. The current implementation does
  // not inspect the bulk-read Result.
  for(usize iter = 0; iter < dims[2]; iter++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    usize slice = static_cast<usize>((dims[2] - 1) - iter);
    usize sliceOffset = slice * sliceVoxels;

    if(maskUInt8StorePtr != nullptr)
    {
      Result<> readResult = maskUInt8StorePtr->copyIntoBuffer(sliceOffset, nonstd::span<uint8>(maskBuf.data(), sliceVoxels));
      if(readResult.invalid())
      {
        return readResult;
      }
    }
    else if(maskBoolStorePtr != nullptr)
    {
      // NOLINTNEXTLINE(modernize-avoid-c-arrays) -- Runtime-sized buffer; std::array cannot represent this extent.
      auto boolBuf = std::make_unique<bool[]>(sliceVoxels);
      Result<> readResult = maskBoolStorePtr->copyIntoBuffer(sliceOffset, nonstd::span<bool>(boolBuf.get(), sliceVoxels));
      if(readResult.invalid())
      {
        return readResult;
      }
      for(usize idx = 0; idx < sliceVoxels; idx++)
      {
        maskBuf[idx] = boolBuf[idx] ? 1 : 0;
      }
    }

    usize count = 0;
    xCentroid[iter] = 0;
    yCentroid[iter] = 0;

    for(usize l = 0; l < dims[1]; l++)
    {
      for(usize n = 0; n < dims[0]; n++)
      {
        usize localIdx = l * dims[0] + n;
        if(maskBuf[localIdx] != 0)
        {
          xCentroid[iter] += static_cast<float32>(n) * spacing[0];
          yCentroid[iter] += static_cast<float32>(l) * spacing[1];
          count++;
        }
      }
    }
    xCentroid[iter] = xCentroid[iter] / static_cast<float32>(count);
    yCentroid[iter] = yCentroid[iter] / static_cast<float32>(count);
  }

  // Convert physical centroid differences to integer voxel offsets.
  bool xWarning = false;
  bool yWarning = false;
  for(usize iter = (m_InputValues->use_reference_slice && m_InputValues->reference_slice != 0) ? 0 : 1; iter < dims[2]; iter++)
  {
    if(m_InputValues->use_reference_slice)
    {
      xShifts[iter] = ComputeCentroidShift(xCentroid[iter], xCentroid[static_cast<usize>(m_InputValues->reference_slice)], spacing[0]);
      yShifts[iter] = ComputeCentroidShift(yCentroid[iter], yCentroid[static_cast<usize>(m_InputValues->reference_slice)], spacing[1]);
    }
    else
    {
      xShifts[iter] = xShifts[iter - 1] + ComputeCentroidShift(xCentroid[iter], xCentroid[iter - 1], spacing[0]);
      yShifts[iter] = yShifts[iter - 1] + ComputeCentroidShift(yCentroid[iter], yCentroid[iter - 1], spacing[1]);
    }

    if((xShifts[iter] < -sdims[0] || xShifts[iter] > sdims[0]) && !xWarning)
    {
      m_MessageHandler.sendMessage(nx::core::IFilter::Message::Type::Info, fmt::format("A shift was greater than the X dimension of the Image Geometry. "
                                                                                       "All subsequent slices are probably wrong. Slice={}  X Dim={}  X Shift={}  sDims[0]={}",
                                                                                       iter, dims[0], xShifts[iter], sdims[0]));
      xWarning = true;
    }
    if((yShifts[iter] < -sdims[1] || yShifts[iter] > sdims[1]) && !yWarning)
    {
      m_MessageHandler.sendMessage(nx::core::IFilter::Message::Type::Info, fmt::format("A shift was greater than the Y dimension of the Image Geometry. "
                                                                                       "All subsequent slices are probably wrong. Slice={}  Y Dim={}  Y Shift={}  sDims[1]={}",
                                                                                       iter, dims[1], yShifts[iter], sdims[1]));
      yWarning = true;
    }
    if(std::isnan(xCentroid[iter]) && !xWarning)
    {
      m_MessageHandler.sendMessage(nx::core::IFilter::Message::Type::Info, fmt::format("The X Centroid was NaN. All subsequent slices are probably wrong. Slice={}", iter));
      xWarning = true;
    }
    if(std::isnan(yCentroid[iter]) && !yWarning)
    {
      m_MessageHandler.sendMessage(nx::core::IFilter::Message::Type::Info, fmt::format("The Y Centroid was NaN. All subsequent slices are probably wrong. Slice={}", iter));
      yWarning = true;
    }
  }

  // Scanline diagnostics keep relative shifts signed.
  if(m_InputValues->store_alignment_shifts)
  {
    auto& slicesStore = m_DataStructure.getDataAs<UInt32ArrayType>(m_InputValues->slices_array_path)->getDataStoreRef();
    auto& relativeShiftsStore = m_DataStructure.getDataAs<Int64ArrayType>(m_InputValues->relative_shifts_array_path)->getDataStoreRef();
    auto& cumulativeShiftsStore = m_DataStructure.getDataAs<Int64ArrayType>(m_InputValues->cumulative_shifts_array_path)->getDataStoreRef();
    auto& centroidsStore = m_DataStructure.getDataAs<Float32ArrayType>(m_InputValues->centroids_array_path)->getDataStoreRef();

    for(usize iter = (m_InputValues->use_reference_slice && m_InputValues->reference_slice != 0) ? 0 : 1; iter < dims[2]; iter++)
    {
      usize slice = (dims[2] - 1) - iter;
      int64 relativexshift = 0;
      int64 relativeyshift = 0;
      if(m_InputValues->use_reference_slice)
      {
        relativexshift = xShifts[iter];
        relativeyshift = yShifts[iter];
      }
      else
      {
        relativexshift = ComputeCentroidShift(xCentroid[iter], xCentroid[iter - 1], spacing[0]);
        relativeyshift = ComputeCentroidShift(yCentroid[iter], yCentroid[iter - 1], spacing[1]);
      }

      usize xIndex = iter * 2;
      usize yIndex = (iter * 2) + 1;
      slicesStore[xIndex] = slice;
      slicesStore[yIndex] = iter == 0 ? dims[2] - 1 - static_cast<usize>(m_InputValues->reference_slice) : slice + 1;
      relativeShiftsStore[xIndex] = relativexshift;
      relativeShiftsStore[yIndex] = relativeyshift;
      cumulativeShiftsStore[xIndex] = xShifts[iter];
      cumulativeShiftsStore[yIndex] = yShifts[iter];
      centroidsStore[xIndex] = xCentroid[iter];
      centroidsStore[yIndex] = yCentroid[iter];
    }
  }

  return {};
}
