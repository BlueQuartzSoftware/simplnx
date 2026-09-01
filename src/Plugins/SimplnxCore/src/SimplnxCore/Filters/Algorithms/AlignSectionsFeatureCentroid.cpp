#include <array>

#include "AlignSectionsFeatureCentroid.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"

using namespace nx::core;

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
  const auto& gridGeom = m_DataStructure.getDataRefAs<IGridGeometry>(m_InputValues->ImageGeometryPath);

  return execute(gridGeom.getDimensions(), m_InputValues->ImageGeometryPath);
}

Result<> AlignSectionsFeatureCentroid::findShifts(std::vector<int64>& xShifts, std::vector<int64>& yShifts)
{
  bool usesOutOfCoreStore = false;
  // Select storage-neutral bulk access before MaskCompare can perform element access.
  {
    const auto& maskCheck = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->MaskArrayPath);
    usesOutOfCoreStore = IsOutOfCore(maskCheck);
    if(!ForceInCoreAlgorithm() && (ForceOocAlgorithm() || usesOutOfCoreStore))
    {
      RecordAlgorithmPathExecution(AlgorithmPath::OutOfCore, usesOutOfCoreStore);
      return findShiftsOoc(xShifts, yShifts);
    }
  }

  RecordAlgorithmPathExecution(AlgorithmPath::InCore, usesOutOfCoreStore);

  std::unique_ptr<MaskCompareUtilities::MaskCompare> maskCompare;
  try
  {
    maskCompare = MaskCompareUtilities::InstantiateMaskCompare(m_DataStructure, m_InputValues->MaskArrayPath);
  } catch(const std::out_of_range& exception)
  {
    // Direct callers can bypass preflight, so return an invalid mask as a Result.
    std::string message = fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->MaskArrayPath.toString());
    return MakeErrorResult(-53900, message);
  }

  auto* gridGeom = m_DataStructure.getDataAs<ImageGeom>(m_InputValues->ImageGeometryPath);

  SizeVec3 dims = gridGeom->getDimensions();

  std::array<int64, 3> sdims = {
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
  // Traverse source slices from highest Z to lowest Z.
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
    // These unsigned intermediates preserve current direct-path behavior.
    // A negative relative shift can wrap before it reaches signed output storage.
    usize relativexshift = 0;
    usize relativeyshift = 0;

    auto& slicesStore = m_DataStructure.getDataAs<UInt32Array>(m_InputValues->SlicesArrayPath)->getDataStoreRef();
    auto& relativeShiftsStore = m_DataStructure.getDataAs<Int64Array>(m_InputValues->RelativeShiftsArrayPath)->getDataStoreRef();
    auto& cumulativeShiftsStore = m_DataStructure.getDataAs<Int64Array>(m_InputValues->CumulativeShiftsArrayPath)->getDataStoreRef();
    auto& centroidsStore = m_DataStructure.getDataAs<Float32Array>(m_InputValues->CentroidsArrayPath)->getDataStoreRef();

    // Convert physical centroid differences to integer voxel offsets.
    for(usize iter = 1; iter < dims[2]; iter++)
    {
      slice = (dims[2] - 1) - iter;
      if(m_InputValues->UseReferenceSlice)
      {
        // Reference-relative and cumulative shifts are identical in this mode.
        relativexshift = static_cast<int64>((xCentroid[iter] - xCentroid[static_cast<usize>(m_InputValues->ReferenceSlice)]) / spacing[0]);
        relativeyshift = static_cast<int64>((yCentroid[iter] - yCentroid[static_cast<usize>(m_InputValues->ReferenceSlice)]) / spacing[1]);
        xShifts[iter] = relativexshift;
        yShifts[iter] = relativeyshift;
      }
      else
      {
        // Accumulate consecutive-slice offsets.
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
    // Compute shifts without writing diagnostic arrays.
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

Result<> AlignSectionsFeatureCentroid::findShiftsOoc(std::vector<int64>& xShifts, std::vector<int64>& yShifts)
{
  // Resolve Bool or UInt8 storage for one-slice bulk reads.
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

  std::array<int64, 3> sdims = {
      static_cast<int64>(dims[0]),
      static_cast<int64>(dims[1]),
      static_cast<int64>(dims[2]),
  };

  nx::core::FloatVec3 spacing = gridGeom->getSpacing();
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
      maskUInt8StorePtr->copyIntoBuffer(sliceOffset, nonstd::span<uint8>(maskBuf.data(), sliceVoxels));
    }
    else if(maskBoolStorePtr != nullptr)
    {
      // NOLINTNEXTLINE(modernize-avoid-c-arrays) -- Runtime-sized buffer; std::array cannot represent this extent.
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

  // Convert physical centroid differences to integer voxel offsets.
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

  // Scanline diagnostics keep relative shifts signed.
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
