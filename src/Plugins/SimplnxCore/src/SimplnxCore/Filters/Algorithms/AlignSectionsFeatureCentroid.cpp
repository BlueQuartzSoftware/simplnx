#include "AlignSectionsFeatureCentroid.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"

#include <array>

using namespace nx::core;

namespace
{
// Error and warning codes. This algorithm owns the -539xx series.
constexpr nx::core::int32 k_MissingMaskArray = -53900;
constexpr nx::core::int32 k_EmptyReferenceSlice = -53901;
constexpr nx::core::int32 k_XShiftOutOfRange = -53902;
constexpr nx::core::int32 k_YShiftOutOfRange = -53903;
constexpr nx::core::int32 k_EmptySlice = -53904;
constexpr nx::core::int32 k_ReferenceSliceOutOfRange = -53905;
} // namespace

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
Result<> AlignSectionsFeatureCentroid::findShifts(std::vector<int64_t>& xShifts, std::vector<int64_t>& yShifts)
{
  std::unique_ptr<MaskCompareUtilities::MaskCompare> maskCompare;
  try
  {
    maskCompare = MaskCompareUtilities::InstantiateMaskCompare(m_DataStructure, m_InputValues->MaskArrayPath);
  } catch(const std::out_of_range& exception)
  {
    // This really should NOT be happening as the path was verified during preflight BUT we may be calling this from
    // somewhere else that is NOT going through the normal nx::core::IFilter API of Preflight and Execute
    std::string message = fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->MaskArrayPath.toString());
    return MakeErrorResult(k_MissingMaskArray, message);
  }

  auto* gridGeom = m_DataStructure.getDataAs<ImageGeom>(m_InputValues->ImageGeometryPath);

  SizeVec3 dims = gridGeom->getDimensions();

  int64_t sdims[3] = {
      static_cast<int64_t>(dims[0]),
      static_cast<int64_t>(dims[1]),
      static_cast<int64_t>(dims[2]),
  };

  size_t slice = 0;
  size_t point = 0;
  nx::core::FloatVec3 spacing = gridGeom->getSpacing();
  std::vector<float> xCentroid(dims[2], 0.0f);
  std::vector<float> yCentroid(dims[2], 0.0f);
  // Records which iteration indices had at least one in-mask Cell. A slice with none has no
  // centroid at all, so it cannot contribute a shift and cannot serve as an alignment target.
  std::vector<bool> sliceHasMask(dims[2], false);

  Result<> result;

  ThrottledMessenger throttledMessenger = getMessageHelper().createThrottledMessenger();
  // Loop over the Z Direction. Note that 'iter' walks from the slice farthest from the Z origin back
  // toward it, so the centroid arrays are stored in the reverse of the physical slice order.
  for(size_t iter = 0; iter < dims[2]; iter++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    throttledMessenger.sendThrottledMessage([&]() { return fmt::format("Determining Shifts || {:.2f}% Complete", CalculatePercentComplete(iter, dims[2])); });

    size_t count = 0;
    xCentroid[iter] = 0;
    yCentroid[iter] = 0;

    slice = static_cast<size_t>((dims[2] - 1) - iter);
    for(size_t l = 0; l < dims[1]; l++)
    {
      for(size_t n = 0; n < dims[0]; n++)
      {
        point = ((slice)*dims[0] * dims[1]) + (l * dims[0]) + n;

        if(maskCompare->isTrue(point))
        {
          xCentroid[iter] = xCentroid[iter] + (static_cast<float>(n) * spacing[0]);
          yCentroid[iter] = yCentroid[iter] + (static_cast<float>(l) * spacing[1]);
          count++;
        }
      }
    }

    if(count == 0)
    {
      // Dividing by a zero Cell count would produce a NaN centroid, and casting a NaN to an integer
      // shift is undefined behavior. An empty slice is reported instead and left where it is.
      xCentroid[iter] = 0.0f;
      yCentroid[iter] = 0.0f;
      result.warnings().push_back(
          Warning{k_EmptySlice, fmt::format("Slice={} has no Cells that are true in the mask array '{}', so it cannot be aligned. In consecutive mode it adds no shift of its own "
                                            "and keeps the cumulative shift of its previously processed neighbor (the section farther from the Z origin), or zero if it has "
                                            "none; in reference mode its shift is zero.",
                                            slice, m_InputValues->MaskArrayPath.toString())});
      continue;
    }

    sliceHasMask[iter] = true;
    xCentroid[iter] = xCentroid[iter] / static_cast<float>(count);
    yCentroid[iter] = yCentroid[iter] / static_cast<float>(count);
  }

  // The Reference Slice parameter is a physical slice index where 0 is the slice at the Z origin, but
  // the centroid arrays above were filled starting from the far end of the stack, so the centroid of
  // physical slice k is stored at index dims[2]-1-k.
  size_t referenceIndex = 0;
  if(m_InputValues->UseReferenceSlice)
  {
    if(m_InputValues->ReferenceSlice < 0 || static_cast<size_t>(m_InputValues->ReferenceSlice) >= dims[2])
    {
      // sdims[2] rather than dims[2] so that a zero-slice geometry reports "0 to -1" instead of
      // wrapping the unsigned subtraction around.
      return MakeErrorResult(k_ReferenceSliceOutOfRange, fmt::format("Reference Slice ({}) is not a valid slice index. The Image Geometry '{}' has {} slices, so the valid range is 0 to {}.",
                                                                     m_InputValues->ReferenceSlice, m_InputValues->ImageGeometryPath.toString(), dims[2], sdims[2] - 1));
    }
    referenceIndex = static_cast<size_t>(static_cast<int64>(dims[2]) - 1 - static_cast<int64>(m_InputValues->ReferenceSlice));
    if(!sliceHasMask[referenceIndex])
    {
      return MakeErrorResult(k_EmptyReferenceSlice, fmt::format("Reference Slice={} has no Cells that are true in the mask array '{}', so there is no centroid for the other slices to align to.",
                                                                m_InputValues->ReferenceSlice, m_InputValues->MaskArrayPath.toString()));
    }
  }

  // Centroid of the most recent slice that had in-mask Cells. Carrying it forward keeps the
  // consecutive-mode chain defined across a slice that is entirely masked out.
  float lastValidXCentroid = 0.0f;
  float lastValidYCentroid = 0.0f;
  bool haveValidCentroid = false;
  if(dims[2] > 0 && sliceHasMask[0])
  {
    lastValidXCentroid = xCentroid[0];
    lastValidYCentroid = yCentroid[0];
    haveValidCentroid = true;
  }

  // In reference mode every slice is aligned against the reference slice, including the one at the
  // far end of the stack, so the loop starts at 0. In consecutive mode index 0 is the anchor: it has
  // no preceding slice and never moves.
  const size_t firstIndex = m_InputValues->UseReferenceSlice ? 0 : 1;

  // Shift of one slice relative to its alignment target, in Cells. Note that the cast truncates
  // toward zero rather than rounding. A slice with no in-mask Cells contributes no shift.
  auto takeRelativeShift = [&](size_t iterIndex) -> std::array<int64, 2> {
    if(!sliceHasMask[iterIndex])
    {
      return {0, 0};
    }
    std::array<int64, 2> relativeShift = {0, 0};
    if(m_InputValues->UseReferenceSlice)
    {
      relativeShift[0] = static_cast<int64>((xCentroid[iterIndex] - xCentroid[referenceIndex]) / spacing[0]);
      relativeShift[1] = static_cast<int64>((yCentroid[iterIndex] - yCentroid[referenceIndex]) / spacing[1]);
    }
    else
    {
      if(haveValidCentroid)
      {
        relativeShift[0] = static_cast<int64>((xCentroid[iterIndex] - lastValidXCentroid) / spacing[0]);
        relativeShift[1] = static_cast<int64>((yCentroid[iterIndex] - lastValidYCentroid) / spacing[1]);
      }
      lastValidXCentroid = xCentroid[iterIndex];
      lastValidYCentroid = yCentroid[iterIndex];
      haveValidCentroid = true;
    }
    return relativeShift;
  };

  bool xWarning = false;
  bool yWarning = false;
  if(m_InputValues->StoreAlignmentShifts)
  {
    auto& slicesStore = m_DataStructure.getDataAs<UInt32Array>(m_InputValues->SlicesArrayPath)->getDataStoreRef();
    auto& relativeShiftsStore = m_DataStructure.getDataAs<Int64Array>(m_InputValues->RelativeShiftsArrayPath)->getDataStoreRef();
    auto& cumulativeShiftsStore = m_DataStructure.getDataAs<Int64Array>(m_InputValues->CumulativeShiftsArrayPath)->getDataStoreRef();
    auto& centroidsStore = m_DataStructure.getDataAs<Float32Array>(m_InputValues->CentroidsArrayPath)->getDataStoreRef();

    // Calculate the X&Y shifts based on the centroid. Note the shifts are in real units
    for(size_t iter = firstIndex; iter < dims[2]; iter++)
    {
      slice = (dims[2] - 1) - iter;
      const std::array<int64, 2> relativeShift = takeRelativeShift(iter);
      if(m_InputValues->UseReferenceSlice)
      {
        // Cumulative and Relative are identical
        xShifts[iter] = relativeShift[0];
        yShifts[iter] = relativeShift[1];
      }
      else
      {
        // Cumulative and Relative are different
        xShifts[iter] = xShifts[iter - 1] + relativeShift[0];
        yShifts[iter] = yShifts[iter - 1] + relativeShift[1];
      }

      if((xShifts[iter] < -sdims[0] || xShifts[iter] > sdims[0]) && !xWarning)
      {
        result.warnings().push_back(Warning{k_XShiftOutOfRange, fmt::format("A shift was greater than the X dimension of the Image Geometry. "
                                                                            "All subsequent slices are probably wrong. Slice={}  X Dim={}  X Shift={}",
                                                                            slice, dims[0], xShifts[iter])});
        xWarning = true;
      }
      if((yShifts[iter] < -sdims[1] || yShifts[iter] > sdims[1]) && !yWarning)
      {
        result.warnings().push_back(Warning{k_YShiftOutOfRange, fmt::format("A shift was greater than the Y dimension of the Image Geometry. "
                                                                            "All subsequent slices are probably wrong. Slice={}  Y Dim={}  Y Shift={}",
                                                                            slice, dims[1], yShifts[iter])});
        yWarning = true;
      }

      usize xIndex = iter * 2;
      usize yIndex = (iter * 2) + 1;
      slicesStore[xIndex] = static_cast<uint32>(slice);
      slicesStore[yIndex] = static_cast<uint32>(slice + 1);
      relativeShiftsStore[xIndex] = relativeShift[0];
      relativeShiftsStore[yIndex] = relativeShift[1];
      cumulativeShiftsStore[xIndex] = xShifts[iter];
      cumulativeShiftsStore[yIndex] = yShifts[iter];
      centroidsStore[xIndex] = xCentroid[iter];
      centroidsStore[yIndex] = yCentroid[iter];
    }
  }
  else
  {
    // Calculate the X&Y shifts based on the centroid. Note the shifts are in real units
    for(size_t iter = firstIndex; iter < dims[2]; iter++)
    {
      slice = (dims[2] - 1) - iter;
      const std::array<int64, 2> relativeShift = takeRelativeShift(iter);
      if(m_InputValues->UseReferenceSlice)
      {
        xShifts[iter] = relativeShift[0];
        yShifts[iter] = relativeShift[1];
      }
      else
      {
        xShifts[iter] = xShifts[iter - 1] + relativeShift[0];
        yShifts[iter] = yShifts[iter - 1] + relativeShift[1];
      }

      if((xShifts[iter] < -sdims[0] || xShifts[iter] > sdims[0]) && !xWarning)
      {
        result.warnings().push_back(Warning{k_XShiftOutOfRange, fmt::format("A shift was greater than the X dimension of the Image Geometry. "
                                                                            "All subsequent slices are probably wrong. Slice={}  X Dim={}  X Shift={}",
                                                                            slice, dims[0], xShifts[iter])});
        xWarning = true;
      }
      if((yShifts[iter] < -sdims[1] || yShifts[iter] > sdims[1]) && !yWarning)
      {
        result.warnings().push_back(Warning{k_YShiftOutOfRange, fmt::format("A shift was greater than the Y dimension of the Image Geometry. "
                                                                            "All subsequent slices are probably wrong. Slice={}  Y Dim={}  Y Shift={}",
                                                                            slice, dims[1], yShifts[iter])});
        yWarning = true;
      }
    }
  }

  return result;
}
