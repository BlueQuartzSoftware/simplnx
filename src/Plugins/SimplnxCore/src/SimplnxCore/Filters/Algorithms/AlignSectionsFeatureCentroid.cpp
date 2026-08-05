#include "AlignSectionsFeatureCentroid.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"

#include <iostream>

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
    return MakeErrorResult(-53900, message);
  }

  auto* gridGeom = m_DataStructure.getDataAs<ImageGeom>(m_InputValues->ImageGeometryPath);

  SizeVec3 dims = gridGeom->getDimensions();

  int64_t sdims[3] = {
      static_cast<int64_t>(dims[0]),
      static_cast<int64_t>(dims[1]),
      static_cast<int64_t>(dims[2]),
  };

  int32_t progInt = 0;

  size_t slice = 0;
  size_t point = 0;
  nx::core::FloatVec3 spacing = gridGeom->getSpacing();
  std::vector<float> xCentroid(dims[2], 0.0f);
  std::vector<float> yCentroid(dims[2], 0.0f);

  ThrottledMessenger throttledMessenger = getMessageHelper().createThrottledMessenger();
  // Loop over the Z Direction
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
    xCentroid[iter] = xCentroid[iter] / static_cast<float>(count);
    yCentroid[iter] = yCentroid[iter] / static_cast<float>(count);
  }

  bool xWarning = false;
  bool yWarning = false;
  if(m_InputValues->StoreAlignmentShifts)
  {
    size_t relativexshift = 0;
    size_t relativeyshift = 0;

    auto& slicesStore = m_DataStructure.getDataAs<UInt32Array>(m_InputValues->SlicesArrayPath)->getDataStoreRef();
    auto& relativeShiftsStore = m_DataStructure.getDataAs<Int64Array>(m_InputValues->RelativeShiftsArrayPath)->getDataStoreRef();
    auto& cumulativeShiftsStore = m_DataStructure.getDataAs<Int64Array>(m_InputValues->CumulativeShiftsArrayPath)->getDataStoreRef();
    auto& centroidsStore = m_DataStructure.getDataAs<Float32Array>(m_InputValues->CentroidsArrayPath)->getDataStoreRef();

    // Calculate the X&Y shifts based on the centroid. Note the shifts are in real units
    for(size_t iter = 1; iter < dims[2]; iter++)
    {
      slice = (dims[2] - 1) - iter;
      if(m_InputValues->UseReferenceSlice)
      {
        // Cumulative and Relative are identical
        relativexshift = static_cast<int64_t>((xCentroid[iter] - xCentroid[static_cast<size_t>(m_InputValues->ReferenceSlice)]) / spacing[0]);
        relativeyshift = static_cast<int64_t>((yCentroid[iter] - yCentroid[static_cast<size_t>(m_InputValues->ReferenceSlice)]) / spacing[1]);
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
        std::string message = fmt::format("The X Centroid was NaN. All subsequent slices are probably wrong. Slice=", iter);
        m_MessageHandler.sendInfoMessage(message);
        xWarning = true;
      }
      if(std::isnan(yCentroid[iter]) && !yWarning)
      {
        std::string message = fmt::format("The Y Centroid was NaN. All subsequent slices are probably wrong. Slice=", iter);
        m_MessageHandler.sendInfoMessage(message);
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
    for(size_t iter = 1; iter < dims[2]; iter++)
    {
      if(m_InputValues->UseReferenceSlice)
      {
        xShifts[iter] = static_cast<int64_t>((xCentroid[iter] - xCentroid[static_cast<size_t>(m_InputValues->ReferenceSlice)]) / spacing[0]);
        yShifts[iter] = static_cast<int64_t>((yCentroid[iter] - yCentroid[static_cast<size_t>(m_InputValues->ReferenceSlice)]) / spacing[1]);
      }
      else
      {
        xShifts[iter] = xShifts[iter - 1] + static_cast<int64_t>((xCentroid[iter] - xCentroid[iter - 1]) / spacing[0]);
        yShifts[iter] = yShifts[iter - 1] + static_cast<int64_t>((yCentroid[iter] - yCentroid[iter - 1]) / spacing[1]);
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
        std::string message = fmt::format("The X Centroid was NaN. All subsequent slices are probably wrong. Slice=", iter);
        m_MessageHandler.sendInfoMessage(message);
        xWarning = true;
      }
      if(std::isnan(yCentroid[iter]) && !yWarning)
      {
        std::string message = fmt::format("The Y Centroid was NaN. All subsequent slices are probably wrong. Slice=", iter);
        m_MessageHandler.sendInfoMessage(message);
        yWarning = true;
      }
    }
  }

  return {};
}
