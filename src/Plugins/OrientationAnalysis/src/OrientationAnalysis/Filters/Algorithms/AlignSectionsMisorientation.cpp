#include "AlignSectionsMisorientation.hpp"

#include "complex/Common/Numbers.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/Geometry/IGridGeometry.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"
#include "complex/Utilities/FilterUtilities.hpp"

#include "EbsdLib/LaueOps/LaueOps.h"

#include <iostream>

using namespace complex;

// -----------------------------------------------------------------------------
AlignSectionsMisorientation::AlignSectionsMisorientation(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                         AlignSectionsMisorientationInputValues* inputValues)
: AlignSections(dataStructure, shouldCancel, mesgHandler)
, m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
AlignSectionsMisorientation::~AlignSectionsMisorientation() noexcept = default;

// -----------------------------------------------------------------------------
Result<> AlignSectionsMisorientation::operator()()
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
std::vector<DataPath> AlignSectionsMisorientation::getSelectedDataPaths() const
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
Result<> AlignSectionsMisorientation::findShifts(std::vector<int64_t>& xShifts, std::vector<int64_t>& yShifts)
{
  std::unique_ptr<MaskCompare> maskCompare = nullptr;
  if(m_InputValues->useGoodVoxels)
  {
    try
    {
      maskCompare = InstantiateMaskCompare(m_DataStructure, m_InputValues->goodVoxelsArrayPath);
    } catch(const std::out_of_range& exception)
    {
      // This really should NOT be happening as the path was verified during preflight BUT we may be calling this from
      // somewhere else that is NOT going through the normal complex::IFilter API of Preflight and Execute
      std::string message = fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->goodVoxelsArrayPath.toString());
      return MakeErrorResult(-53900, message);
    }
  }

  std::ofstream outFile;
  if(m_InputValues->writeAlignmentShifts)
  {
    // Make sure any directory path is also available as the user may have just typed
    // in a path without actually creating the full path
    Result<> createDirectoriesResult = complex::CreateOutputDirectories(m_InputValues->alignmentShiftFileName.parent_path());
    if(createDirectoriesResult.invalid())
    {
      return createDirectoriesResult;
    }
    outFile.open(m_InputValues->alignmentShiftFileName, std::ios_base::out);
    if(!outFile.is_open())
    {
      std::string message = fmt::format("Error creating output shifts file with file path {}", m_InputValues->alignmentShiftFileName.string());
      return MakeErrorResult(-53801, message);
    }
  }

  auto* gridGeom = m_DataStructure.getDataAs<IGridGeometry>(m_InputValues->inputImageGeometry);

  const auto& cellPhases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->cellPhasesArrayPath);
  const auto& quats = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->quatsArrayPath);
  const auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->crystalStructuresArrayPath);

  SizeVec3 udims = gridGeom->getDimensions();

  std::array<int64_t, 3> dims = {
      static_cast<int64_t>(udims[0]),
      static_cast<int64_t>(udims[1]),
      static_cast<int64_t>(udims[2]),
  };

  std::vector<LaueOps::Pointer> orientationOps = LaueOps::GetAllOrientationOps();

  int32_t progInt = 0;

  // Allocate a 2D Array which will be reused from slice to slice
  std::vector<bool> misorients(dims[0] * dims[1], false);

  const auto halfDim0 = static_cast<int64_t>(dims[0] * 0.5f);
  const auto halfDim1 = static_cast<int64_t>(dims[1] * 0.5f);

  double deg2Rad = (complex::numbers::pi / 180.0);
  auto start = std::chrono::steady_clock::now();
  // Loop over the Z Direction
  for(int64_t iter = 1; iter < dims[2]; iter++)
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
    float minDisorientation = std::numeric_limits<float>::max();
    // Work from the largest Slice Value to the lowest Slice Value.
    int64_t slice = (dims[2] - 1) - iter;
    int64_t oldxshift = -1;
    int64_t oldyshift = -1;
    int64_t newxshift = 0;
    int64_t newyshift = 0;

    // Initialize everything to false
    std::fill(misorients.begin(), misorients.end(), false);

    float misorientationTolerance = static_cast<float>(m_InputValues->misorientationTolerance * deg2Rad);

    while(newxshift != oldxshift || newyshift != oldyshift)
    {
      oldxshift = newxshift;
      oldyshift = newyshift;
      for(int32_t j = -3; j < 4; j++)
      {
        for(int32_t k = -3; k < 4; k++)
        {
          float disorientation = 0.0f;
          float count = 0.0f;
          int64_t xIdx = k + oldxshift + halfDim0;
          int64_t yIdx = j + oldyshift + halfDim1;
          int64_t idx = (dims[0] * yIdx) + xIdx;
          if(!misorients[idx] && llabs(k + oldxshift) < halfDim0 && llabs(j + oldyshift) < halfDim1)
          {
            for(int64_t l = 0; l < dims[1]; l = l + 4)
            {
              for(int64_t n = 0; n < dims[0]; n = n + 4)
              {
                if((l + j + oldyshift) >= 0 && (l + j + oldyshift) < dims[1] && (n + k + oldxshift) >= 0 && (n + k + oldxshift) < dims[0])
                {
                  count++;
                  int64_t refposition = ((slice + 1) * dims[0] * dims[1]) + (l * dims[0]) + n;
                  int64_t curposition = (slice * dims[0] * dims[1]) + ((l + j + oldyshift) * dims[0]) + (n + k + oldxshift);
                  if(!m_InputValues->useGoodVoxels || maskCompare->bothTrue(refposition, curposition))
                  {
                    float angle = std::numeric_limits<float>::max();
                    if(cellPhases[refposition] > 0 && cellPhases[curposition] > 0)
                    {
                      QuatF quat1(quats[refposition * 4], quats[refposition * 4 + 1], quats[refposition * 4 + 2], quats[refposition * 4 + 3]); // Makes a copy into voxQuat!!!!
                      auto phase1 = static_cast<int32_t>(crystalStructures[cellPhases[refposition]]);
                      QuatF quat2(quats[curposition * 4], quats[curposition * 4 + 1], quats[curposition * 4 + 2], quats[curposition * 4 + 3]); // Makes a copy into voxQuat!!!!
                      auto phase2 = static_cast<int32_t>(crystalStructures[cellPhases[curposition]]);
                      if(phase1 == phase2 && phase1 < static_cast<uint32_t>(orientationOps.size()))
                      {
                        OrientationF axisAngle = orientationOps[phase1]->calculateMisorientation(quat1, quat2);
                        angle = axisAngle[3];
                      }
                    }
                    if(angle > misorientationTolerance)
                    {
                      disorientation++;
                    }
                  }
                  if(m_InputValues->useGoodVoxels)
                  {
                    if(maskCompare->isTrue(refposition) && !maskCompare->isTrue(curposition))
                    {
                      disorientation++;
                    }
                    if(!maskCompare->isTrue(refposition) && maskCompare->isTrue(curposition))
                    {
                      disorientation++;
                    }
                  }
                }
              }
            }
            disorientation = disorientation / count;
            xIdx = k + oldxshift + halfDim0;
            yIdx = j + oldyshift + halfDim1;
            idx = (dims[0] * yIdx) + xIdx;
            misorients[idx] = true;
            if(disorientation < minDisorientation || (disorientation == minDisorientation && ((llabs(k + oldxshift) < llabs(newxshift)) || (llabs(j + oldyshift) < llabs(newyshift)))))
            {
              newxshift = k + oldxshift;
              newyshift = j + oldyshift;
              minDisorientation = disorientation;
            }
          }
        }
      }
    }
    xShifts[iter] = xShifts[iter - 1] + newxshift;
    yShifts[iter] = yShifts[iter - 1] + newyshift;
    if(m_InputValues->writeAlignmentShifts)
    {
      outFile << slice << "\t" << slice + 1 << "\t" << newxshift << "\t" << newyshift << "\t" << xShifts[iter] << "\t" << yShifts[iter] << "\n";
    }
  }
  if(m_InputValues->writeAlignmentShifts)
  {
    outFile.close();
  }

  return {};
}
