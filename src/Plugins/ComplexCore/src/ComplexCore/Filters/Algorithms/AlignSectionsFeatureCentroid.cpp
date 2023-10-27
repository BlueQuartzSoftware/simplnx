#include "AlignSectionsFeatureCentroid.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"
#include "complex/Utilities/FilterUtilities.hpp"

#include <iostream>

using namespace complex;

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
  const auto& gridGeom = m_DataStructure.getDataAs<IGridGeometry>(m_InputValues->inputImageGeometry);
  Result<> result = execute(gridGeom->getDimensions());
  if(result.invalid())
  {
    return result;
  }
  if(m_Result.invalid())
  {
    return m_Result;
  }
  return {};
}

// -----------------------------------------------------------------------------
std::vector<DataPath> AlignSectionsFeatureCentroid::getSelectedDataPaths() const
{
  auto cellDataGroupPath = m_InputValues->cellDataGroupPath;
  auto& cellDataGroup = m_DataStructure.getDataRefAs<AttributeMatrix>(cellDataGroupPath);
  std::vector<DataPath> selectedCellArrays;

  // Create the vector of selected cell DataPaths
  for(const auto& child : cellDataGroup)
  {
    selectedCellArrays.push_back(m_InputValues->cellDataGroupPath.createChildPath(child.second->getName()));
  }
  return selectedCellArrays;
}

// -----------------------------------------------------------------------------
Result<> AlignSectionsFeatureCentroid::findShifts(std::vector<int64_t>& xShifts, std::vector<int64_t>& yShifts)
{
  std::unique_ptr<MaskCompare> maskCompare;
  try
  {
    maskCompare = InstantiateMaskCompare(m_DataStructure, m_InputValues->MaskArrayPath);
  } catch(const std::out_of_range& exception)
  {
    // This really should NOT be happening as the path was verified during preflight BUT we may be calling this from
    // somewhere else that is NOT going through the normal complex::IFilter API of Preflight and Execute
    std::string message = fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->MaskArrayPath.toString());
    return MakeErrorResult(-53900, message);
  }

  std::ofstream outFile;
  if(m_InputValues->WriteAlignmentShifts)
  {
    // Make sure any directory path is also available as the user may have just typed
    // in a path without actually creating the full path
    Result<> createDirectoriesResult = complex::CreateOutputDirectories(m_InputValues->AlignmentShiftFileName.parent_path());
    if(createDirectoriesResult.invalid())
    {
      return createDirectoriesResult;
    }

    outFile.open(m_InputValues->AlignmentShiftFileName, std::ios_base::out);
    if(!outFile.is_open())
    {
      std::string message = fmt::format("Error creating Input Shifts File with file path {}", m_InputValues->AlignmentShiftFileName.string());
      return MakeErrorResult(-53901, message);
    }
    outFile << "#"
            << "Slice_A,Slice_B,New X Shift,New Y Shift,X Shift, Y Shift, X Centroid, Y Centroid" << std::endl;
  }

  auto* gridGeom = m_DataStructure.getDataAs<ImageGeom>(m_InputValues->inputImageGeometry);

  SizeVec3 dims = gridGeom->getDimensions();

  int64_t sdims[3] = {
      static_cast<int64_t>(dims[0]),
      static_cast<int64_t>(dims[1]),
      static_cast<int64_t>(dims[2]),
  };

  int32_t progInt = 0;

  size_t newxshift = 0;
  size_t newyshift = 0;

  size_t slice = 0;
  size_t point = 0;
  complex::FloatVec3 spacing = gridGeom->getSpacing();
  std::vector<float> xCentroid(dims[2], 0.0f);
  std::vector<float> yCentroid(dims[2], 0.0f);

  auto start = std::chrono::steady_clock::now();
  // Loop over the Z Direction
  for(size_t iter = 0; iter < dims[2]; iter++)
  {
    progInt = static_cast<float>(iter) / static_cast<float>(dims[2]) * 100.0f;
    auto now = std::chrono::steady_clock::now();
    // Only send updates every 1 second
    if(std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() > 1000)
    {
      std::string message = fmt::format("Determining Shifts || {}% Complete", progInt);
      m_MessageHandler(complex::IFilter::ProgressMessage{complex::IFilter::Message::Type::Info, message, progInt});
      start = std::chrono::steady_clock::now();
    }
    if(getCancel())
    {
      return {};
    }

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
  // Calculate the X&Y shifts based on the centroid. Note the shifts are in real units
  for(size_t iter = 1; iter < dims[2]; iter++)
  {
    slice = (dims[2] - 1) - iter;
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
      m_MessageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, message});
      xWarning = true;
    }
    if((yShifts[iter] < -sdims[1] || yShifts[iter] > sdims[1]) && !yWarning)
    {
      std::string message = fmt::format("A shift was greater than the Y dimension of the Image Geometry. "
                                        "All subsequent slices are probably wrong. Slice={}  Y Dim={}  Y Shift={}  sDims[1]={}",
                                        iter, dims[1], yShifts[iter], sdims[1]);
      m_MessageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, message});
      yWarning = true;
    }
    if(std::isnan(xCentroid[iter]) && !xWarning)
    {
      std::string message = fmt::format("The X Centroid was NaN. All subsequent slices are probably wrong. Slice=", iter);
      m_MessageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, message});
      xWarning = true;
    }
    if(std::isnan(yCentroid[iter]) && !yWarning)
    {
      std::string message = fmt::format("The Y Centroid was NaN. All subsequent slices are probably wrong. Slice=", iter);
      m_MessageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, message});
      yWarning = true;
    }
    if(m_InputValues->WriteAlignmentShifts)
    {
      outFile << slice << "," << slice + 1 << "," << newxshift << "," << newyshift << "," << xShifts[iter] << "," << yShifts[iter] << "," << xCentroid[iter] << "," << yCentroid[iter] << std::endl;
    }
  }
  if(m_InputValues->WriteAlignmentShifts)
  {
    outFile.close();
  }

  return {};
}
