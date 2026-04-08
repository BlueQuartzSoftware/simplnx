#include "AlignSectionsFeatureCentroid.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
AlignSectionsFeatureCentroid::AlignSectionsFeatureCentroid(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                           AlignSectionsFeatureCentroidInputValues* inputValues)
: AlignSections(dataStructure, shouldCancel, mesgHandler)
, m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
AlignSectionsFeatureCentroid::~AlignSectionsFeatureCentroid() noexcept = default;

// -----------------------------------------------------------------------------
Result<> AlignSectionsFeatureCentroid::operator()()
{
  if(m_ShouldCancel)
  {
    return {};
  }
  const auto& gridGeom = m_DataStructure.getDataRefAs<IGridGeometry>(m_InputValues->ImageGeometryPath);

  return execute(gridGeom.getDimensions(), m_InputValues->ImageGeometryPath);
}

// -----------------------------------------------------------------------------
Result<> AlignSectionsFeatureCentroid::findShifts(std::vector<int64>& xShifts, std::vector<int64>& yShifts)
{
  {
    const auto& maskCheck = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->MaskArrayPath);
    if(ForceOocAlgorithm() || IsOutOfCore(maskCheck))
    {
      return findShiftsOoc(xShifts, yShifts);
    }
  }

  std::unique_ptr<MaskCompareUtilities::MaskCompare> maskCompare;
  try
  {
    maskCompare = MaskCompareUtilities::InstantiateMaskCompare(m_DataStructure, m_InputValues->MaskArrayPath);
  } catch(const std::out_of_range& exception)
  {
    // This really should NOT be happening as the path was verified during preflight BUT we may be calling this from
    // somewhere else that is NOT going through the normal nx::core::IFilter API of Preflight and Execute
    std::string message = fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->MaskArrayPath.toString());
    return MakeErrorResult(-53900, message);
  }

  auto* gridGeom = m_DataStructure.getDataAs<ImageGeom>(m_InputValues->ImageGeometryPath);

  SizeVec3 dims = gridGeom->getDimensions();

  int64 sdims[3] = {
      static_cast<int64>(dims[0]),
      static_cast<int64>(dims[1]),
      static_cast<int64>(dims[2]),
  };

  int32 progInt = 0;

  usize slice = 0;
  usize point = 0;
  nx::core::FloatVec3 spacing = gridGeom->getSpacing();
  std::vector<float32> xCentroid(dims[2], 0.0f);
  std::vector<float32> yCentroid(dims[2], 0.0f);

  ThrottledMessenger throttledMessenger = getMessageHelper().createThrottledMessenger();
  // Loop over the Z Direction
  for(usize iter = 0; iter < dims[2]; iter++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    throttledMessenger.sendThrottledMessage([&]() { return fmt::format("Determining Shifts || {:.2f}% Complete", CalculatePercentComplete(iter, dims[2])); });

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
  if(m_InputValues->StoreAlignmentShifts)
  {
    usize relativexshift = 0;
    usize relativeyshift = 0;

    auto& slicesStore = m_DataStructure.getDataAs<UInt32Array>(m_InputValues->SlicesArrayPath)->getDataStoreRef();
    auto& relativeShiftsStore = m_DataStructure.getDataAs<Int64Array>(m_InputValues->RelativeShiftsArrayPath)->getDataStoreRef();
    auto& cumulativeShiftsStore = m_DataStructure.getDataAs<Int64Array>(m_InputValues->CumulativeShiftsArrayPath)->getDataStoreRef();
    auto& centroidsStore = m_DataStructure.getDataAs<Float32Array>(m_InputValues->CentroidsArrayPath)->getDataStoreRef();

    // Calculate the X&Y shifts based on the centroid. Note the shifts are in real units
    for(usize iter = 1; iter < dims[2]; iter++)
    {
      slice = (dims[2] - 1) - iter;
      if(m_InputValues->UseReferenceSlice)
      {
        // Cumulative and Relative are identical
        relativexshift = static_cast<int64>((xCentroid[iter] - xCentroid[static_cast<usize>(m_InputValues->ReferenceSlice)]) / spacing[0]);
        relativeyshift = static_cast<int64>((yCentroid[iter] - yCentroid[static_cast<usize>(m_InputValues->ReferenceSlice)]) / spacing[1]);
        xShifts[iter] = relativexshift;
        yShifts[iter] = relativeyshift;
      }
      else
      {
        // Cumulative and Relative are different
        relativexshift = static_cast<int64>((xCentroid[iter] - xCentroid[iter - 1]) / spacing[0]);
        relativeyshift = static_cast<int64>((yCentroid[iter] - yCentroid[iter - 1]) / spacing[1]);
        xShifts[iter] = xShifts[iter - 1] + relativexshift;
        yShifts[iter] = yShifts[iter - 1] + relativeyshift;
      }

      if((xShifts[iter] < -sdims[0] || xShifts[iter] > sdims[0]) && !xWarning)
      {
        std::string message = fmt::format("A shift was greater than the X dimension of the Image Geometry. "
                                          "All subsequent slices are probably wrong. Slice={}  X Dim={}  X Shift={}  sDims[0]={}",
                                          iter, dims[0], xShifts[iter], sdims[0]);
        m_MessageHandler(nx::core::IFilter::Message::Type::Info, message);
        xWarning = true;
      }
      if((yShifts[iter] < -sdims[1] || yShifts[iter] > sdims[1]) && !yWarning)
      {
        std::string message = fmt::format("A shift was greater than the Y dimension of the Image Geometry. "
                                          "All subsequent slices are probably wrong. Slice={}  Y Dim={}  Y Shift={}  sDims[1]={}",
                                          iter, dims[1], yShifts[iter], sdims[1]);
        m_MessageHandler(nx::core::IFilter::Message::Type::Info, message);
        yWarning = true;
      }
      if(std::isnan(xCentroid[iter]) && !xWarning)
      {
        std::string message = fmt::format("The X Centroid was NaN. All subsequent slices are probably wrong. Slice=", iter);
        m_MessageHandler(nx::core::IFilter::Message::Type::Info, message);
        xWarning = true;
      }
      if(std::isnan(yCentroid[iter]) && !yWarning)
      {
        std::string message = fmt::format("The Y Centroid was NaN. All subsequent slices are probably wrong. Slice=", iter);
        m_MessageHandler(nx::core::IFilter::Message::Type::Info, message);
        yWarning = true;
      }

      usize xIndex = iter * 2;
      usize yIndex = (iter * 2) + 1;
      slicesStore[xIndex] = slice;
      slicesStore[yIndex] = slice + 1;
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
    // Calculate the X&Y shifts based on the centroid. Note the shifts are in real units
    for(usize iter = 1; iter < dims[2]; iter++)
    {
      if(m_InputValues->UseReferenceSlice)
      {
        xShifts[iter] = static_cast<int64>((xCentroid[iter] - xCentroid[static_cast<usize>(m_InputValues->ReferenceSlice)]) / spacing[0]);
        yShifts[iter] = static_cast<int64>((yCentroid[iter] - yCentroid[static_cast<usize>(m_InputValues->ReferenceSlice)]) / spacing[1]);
      }
      else
      {
        xShifts[iter] = xShifts[iter - 1] + static_cast<int64>((xCentroid[iter] - xCentroid[iter - 1]) / spacing[0]);
        yShifts[iter] = yShifts[iter - 1] + static_cast<int64>((yCentroid[iter] - yCentroid[iter - 1]) / spacing[1]);
      }

      if((xShifts[iter] < -sdims[0] || xShifts[iter] > sdims[0]) && !xWarning)
      {
        std::string message = fmt::format("A shift was greater than the X dimension of the Image Geometry. "
                                          "All subsequent slices are probably wrong. Slice={}  X Dim={}  X Shift={}  sDims[0]={}",
                                          iter, dims[0], xShifts[iter], sdims[0]);
        m_MessageHandler(nx::core::IFilter::Message::Type::Info, message);
        xWarning = true;
      }
      if((yShifts[iter] < -sdims[1] || yShifts[iter] > sdims[1]) && !yWarning)
      {
        std::string message = fmt::format("A shift was greater than the Y dimension of the Image Geometry. "
                                          "All subsequent slices are probably wrong. Slice={}  Y Dim={}  Y Shift={}  sDims[1]={}",
                                          iter, dims[1], yShifts[iter], sdims[1]);
        m_MessageHandler(nx::core::IFilter::Message::Type::Info, message);
        yWarning = true;
      }
      if(std::isnan(xCentroid[iter]) && !xWarning)
      {
        std::string message = fmt::format("The X Centroid was NaN. All subsequent slices are probably wrong. Slice=", iter);
        m_MessageHandler(nx::core::IFilter::Message::Type::Info, message);
        xWarning = true;
      }
      if(std::isnan(yCentroid[iter]) && !yWarning)
      {
        std::string message = fmt::format("The Y Centroid was NaN. All subsequent slices are probably wrong. Slice=", iter);
        m_MessageHandler(nx::core::IFilter::Message::Type::Info, message);
        yWarning = true;
      }
    }
  }

  return {};
}

// -----------------------------------------------------------------------------
// OOC-optimized findShifts: bulk-reads mask per Z-slice instead of per-element
// isTrue() calls, eliminating OOC chunk thrashing in the centroid computation.
// -----------------------------------------------------------------------------
Result<> AlignSectionsFeatureCentroid::findShiftsOoc(std::vector<int64>& xShifts, std::vector<int64>& yShifts)
{
  // Get raw mask store for bulk reads
  const auto& maskArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->MaskArrayPath);
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
    return MakeErrorResult(-53900, fmt::format("Mask Array is not Bool or UInt8: {}", m_InputValues->MaskArrayPath.toString()));
  }

  auto* gridGeom = m_DataStructure.getDataAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  SizeVec3 dims = gridGeom->getDimensions();

  int64 sdims[3] = {
      static_cast<int64>(dims[0]),
      static_cast<int64>(dims[1]),
      static_cast<int64>(dims[2]),
  };

  nx::core::FloatVec3 spacing = gridGeom->getSpacing();
  std::vector<float32> xCentroid(dims[2], 0.0f);
  std::vector<float32> yCentroid(dims[2], 0.0f);

  const usize sliceVoxels = dims[0] * dims[1];
  std::vector<uint8> maskBuf(sliceVoxels);

  // Compute centroids per Z-slice using bulk mask reads
  for(usize iter = 0; iter < dims[2]; iter++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    usize slice = static_cast<usize>((dims[2] - 1) - iter);
    usize sliceOffset = slice * sliceVoxels;

    // Bulk-read mask for this slice
    if(maskUInt8StorePtr != nullptr)
    {
      maskUInt8StorePtr->copyIntoBuffer(sliceOffset, nonstd::span<uint8>(maskBuf.data(), sliceVoxels));
    }
    else if(maskBoolStorePtr != nullptr)
    {
      auto boolBuf = std::make_unique<bool[]>(sliceVoxels);
      maskBoolStorePtr->copyIntoBuffer(sliceOffset, nonstd::span<bool>(boolBuf.get(), sliceVoxels));
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

  // Calculate shifts from centroids (same logic as in-core path)
  bool xWarning = false;
  bool yWarning = false;
  for(usize iter = 1; iter < dims[2]; iter++)
  {
    if(m_InputValues->UseReferenceSlice)
    {
      xShifts[iter] = static_cast<int64>((xCentroid[iter] - xCentroid[static_cast<usize>(m_InputValues->ReferenceSlice)]) / spacing[0]);
      yShifts[iter] = static_cast<int64>((yCentroid[iter] - yCentroid[static_cast<usize>(m_InputValues->ReferenceSlice)]) / spacing[1]);
    }
    else
    {
      xShifts[iter] = xShifts[iter - 1] + static_cast<int64>((xCentroid[iter] - xCentroid[iter - 1]) / spacing[0]);
      yShifts[iter] = yShifts[iter - 1] + static_cast<int64>((yCentroid[iter] - yCentroid[iter - 1]) / spacing[1]);
    }

    if((xShifts[iter] < -sdims[0] || xShifts[iter] > sdims[0]) && !xWarning)
    {
      m_MessageHandler(nx::core::IFilter::Message::Type::Info, fmt::format("A shift was greater than the X dimension of the Image Geometry. "
                                                                           "All subsequent slices are probably wrong. Slice={}  X Dim={}  X Shift={}  sDims[0]={}",
                                                                           iter, dims[0], xShifts[iter], sdims[0]));
      xWarning = true;
    }
    if((yShifts[iter] < -sdims[1] || yShifts[iter] > sdims[1]) && !yWarning)
    {
      m_MessageHandler(nx::core::IFilter::Message::Type::Info, fmt::format("A shift was greater than the Y dimension of the Image Geometry. "
                                                                           "All subsequent slices are probably wrong. Slice={}  Y Dim={}  Y Shift={}  sDims[1]={}",
                                                                           iter, dims[1], yShifts[iter], sdims[1]));
      yWarning = true;
    }
    if(std::isnan(xCentroid[iter]) && !xWarning)
    {
      m_MessageHandler(nx::core::IFilter::Message::Type::Info, fmt::format("The X Centroid was NaN. All subsequent slices are probably wrong. Slice=", iter));
      xWarning = true;
    }
    if(std::isnan(yCentroid[iter]) && !yWarning)
    {
      m_MessageHandler(nx::core::IFilter::Message::Type::Info, fmt::format("The Y Centroid was NaN. All subsequent slices are probably wrong. Slice=", iter));
      yWarning = true;
    }
  }

  // Store alignment shifts if requested
  if(m_InputValues->StoreAlignmentShifts)
  {
    auto& slicesStore = m_DataStructure.getDataAs<UInt32Array>(m_InputValues->SlicesArrayPath)->getDataStoreRef();
    auto& relativeShiftsStore = m_DataStructure.getDataAs<Int64Array>(m_InputValues->RelativeShiftsArrayPath)->getDataStoreRef();
    auto& cumulativeShiftsStore = m_DataStructure.getDataAs<Int64Array>(m_InputValues->CumulativeShiftsArrayPath)->getDataStoreRef();
    auto& centroidsStore = m_DataStructure.getDataAs<Float32Array>(m_InputValues->CentroidsArrayPath)->getDataStoreRef();

    for(usize iter = 1; iter < dims[2]; iter++)
    {
      usize slice = (dims[2] - 1) - iter;
      int64 relativexshift = 0;
      int64 relativeyshift = 0;
      if(m_InputValues->UseReferenceSlice)
      {
        relativexshift = xShifts[iter];
        relativeyshift = yShifts[iter];
      }
      else
      {
        relativexshift = static_cast<int64>((xCentroid[iter] - xCentroid[iter - 1]) / spacing[0]);
        relativeyshift = static_cast<int64>((yCentroid[iter] - yCentroid[iter - 1]) / spacing[1]);
      }

      usize xIndex = iter * 2;
      usize yIndex = (iter * 2) + 1;
      slicesStore[xIndex] = slice;
      slicesStore[yIndex] = slice + 1;
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
