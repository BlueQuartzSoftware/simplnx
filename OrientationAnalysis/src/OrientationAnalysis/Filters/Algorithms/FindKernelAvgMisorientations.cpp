#include "FindKernelAvgMisorientations.hpp"

#include "complex/Common/Constants.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Utilities/ParallelData3DAlgorithm.hpp"

#include "EbsdLib/LaueOps/LaueOps.h"

using namespace complex;

namespace
{
class FindKernelAvgMisorientationsImpl
{
public:
  FindKernelAvgMisorientationsImpl(DataStructure& dataStructure, const FindKernelAvgMisorientationsInputValues* inputValues, const std::atomic_bool& shouldCancel)
  : m_DataStructure(dataStructure)
  , m_InputValues(inputValues)
  , m_ShouldCancel(shouldCancel)
  {
  }

  void convert(size_t zStart, size_t zEnd, size_t yStart, size_t yEnd, size_t xStart, size_t xEnd) const
  {
    // Input Arrays / Parameter Data
    const auto& cellPhasesArray = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellPhasesArrayPath);
    const auto& cellPhases = cellPhasesArray.getDataStoreRef();
    const auto& featureIdsArray = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
    const auto& featureIds = featureIdsArray.getDataStoreRef();
    const auto& quatsArray = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->QuatsArrayPath);
    const auto& quats = quatsArray.getDataStoreRef();
    const auto& crystalStructuresArray = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);
    const auto& crystalStructures = crystalStructuresArray.getDataStoreRef();
    const auto kernelSize = m_InputValues->KernelSize;

    // Output Arrays
    auto& kernelAvgMisorientationsArray = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->KernelAverageMisorientationsArrayName);
    auto& kernelAvgMisorientations = kernelAvgMisorientationsArray.getDataStoreRef();

    std::vector<LaueOps::Pointer> m_OrientationOps = LaueOps::GetAllOrientationOps();

    auto* gridGeom = m_DataStructure.getDataAs<ImageGeom>(m_InputValues->InputImageGeometry);
    SizeVec3 udims = gridGeom->getDimensions();

    auto xPoints = static_cast<int64_t>(udims[0]);
    auto yPoints = static_cast<int64_t>(udims[1]);
    auto zPoints = static_cast<int64_t>(udims[2]);
    for(size_t plane = zStart; plane < zEnd; plane++)
    {
      if(m_ShouldCancel)
      {
        break;
      }
      for(size_t row = yStart; row < yEnd; row++)
      {
        for(size_t col = xStart; col < xEnd; col++)
        {
          size_t point = (plane * xPoints * yPoints) + (row * xPoints) + col;
          if(featureIds[point] > 0 && cellPhases[point] > 0)
          {
            float totalMisorientation = 0.0f;
            int32 numVoxel = 0;

            size_t quatIndex = point * 4;
            QuatF q1(quats[quatIndex], quats[quatIndex + 1], quats[quatIndex + 2], quats[quatIndex + 3]);

            uint32_t phase1 = crystalStructures[cellPhases[point]];
            for(int32_t j = -kernelSize[2]; j < kernelSize[2] + 1; j++)
            {
              size_t jStride = j * xPoints * yPoints;
              for(int32_t k = -kernelSize[1]; k < kernelSize[1] + 1; k++)
              {
                size_t kStride = k * xPoints;
                for(int32_t l = -kernelSize[0]; l < kernelSize[0] + 1; l++)
                {
                  bool good = true;
                  size_t neighbor = point + (jStride) + (kStride) + (l);
                  if(plane + j < 0)
                  {
                    good = false;
                  }
                  else if(plane + j > zPoints - 1)
                  {
                    good = false;
                  }
                  else if(row + k < 0)
                  {
                    good = false;
                  }
                  else if(row + k > yPoints - 1)
                  {
                    good = false;
                  }
                  else if(col + l < 0)
                  {
                    good = false;
                  }
                  else if(col + l > xPoints - 1)
                  {
                    good = false;
                  }
                  if(good && featureIds[point] == featureIds[neighbor])
                  {
                    quatIndex = neighbor * 4;
                    QuatF q2(quats[quatIndex], quats[quatIndex + 1], quats[quatIndex + 2], quats[quatIndex + 3]);
                    OrientationF axisAngle = m_OrientationOps[phase1]->calculateMisorientation(q1, q2);
                    totalMisorientation = totalMisorientation + (axisAngle[3] * complex::Constants::k_180OverPiD);
                    numVoxel++;
                  }
                }
              }
            }
            kernelAvgMisorientations[point] = totalMisorientation / static_cast<float>(numVoxel);
            if(numVoxel == 0)
            {
              kernelAvgMisorientations[point] = 0.0f;
            }
          }
          if(featureIds[point] == 0 || cellPhases[point] == 0)
          {
            kernelAvgMisorientations[point] = 0.0f;
          }
        }
      }
    }
  }

  void operator()(const ComplexRange3D& range) const
  {
    convert(range[4], range[5], range[2], range[3], range[0], range[1]);
  }

private:
  DataStructure& m_DataStructure;
  const FindKernelAvgMisorientationsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
};

} // namespace

// -----------------------------------------------------------------------------
FindKernelAvgMisorientations::FindKernelAvgMisorientations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                           FindKernelAvgMisorientationsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
FindKernelAvgMisorientations::~FindKernelAvgMisorientations() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& FindKernelAvgMisorientations::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> FindKernelAvgMisorientations::operator()()
{
  auto* gridGeom = m_DataStructure.getDataAs<ImageGeom>(m_InputValues->InputImageGeometry);
  SizeVec3 udims = gridGeom->getDimensions();

  ParallelData3DAlgorithm parallelAlgorithm;
  parallelAlgorithm.setRange(ComplexRange3D(0, udims[0], 0, udims[1], 0, udims[2]));
  parallelAlgorithm.setParallelizationEnabled(true);
  parallelAlgorithm.execute(FindKernelAvgMisorientationsImpl(m_DataStructure, m_InputValues, m_ShouldCancel));

  return {};
}
