#include "ComputeFeatureReferenceCAxisMisorientations.hpp"

#include "OrientationAnalysis/utilities/OrientationUtilities.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/ImageRotationUtilities.hpp"

#include "EbsdLib/Core/Orientation.hpp"
#include "EbsdLib/Core/OrientationTransformation.hpp"
#include "EbsdLib/Core/Quaternion.hpp"
#include <EbsdLib/Core/EbsdDataArray.hpp>

#include <algorithm>
#include <cmath>

using namespace nx::core;
using namespace nx::core::OrientationUtilities;

// -----------------------------------------------------------------------------
ComputeFeatureReferenceCAxisMisorientations::ComputeFeatureReferenceCAxisMisorientations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                                         ComputeFeatureReferenceCAxisMisorientationsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeFeatureReferenceCAxisMisorientations::~ComputeFeatureReferenceCAxisMisorientations() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeFeatureReferenceCAxisMisorientations::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ComputeFeatureReferenceCAxisMisorientations::operator()()
{
  const auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);
  bool allPhasesHexagonal = true;
  bool noPhasesHexagonal = true;
  for(usize i = 1; i < crystalStructures.size(); ++i)
  {
    const auto crystalStructureType = crystalStructures[i];
    const bool isHex = crystalStructureType == EbsdLib::CrystalStructure::Hexagonal_High || crystalStructureType == EbsdLib::CrystalStructure::Hexagonal_Low;
    allPhasesHexagonal = allPhasesHexagonal && isHex;
    noPhasesHexagonal = noPhasesHexagonal && !isHex;
  }

  if(noPhasesHexagonal)
  {
    return MakeErrorResult(
        -9802, "Finding the feature reference c-axis mis orientation requires at least one phase to be Hexagonal-Low 6/m or Hexagonal-High 6/mmm type crystal structures but none were found.");
  }

  Result<> result;
  if(!allPhasesHexagonal)
  {
    result.warnings().push_back(
        {-9803,
         "Finding the feature reference c-axis mis orientation requires Hexagonal-Low 6/m or Hexagonal-High 6/mmm type crystal structures. Calculations for non Hexagonal phases will be skipped."});
  }

  const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  const auto& quats = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->QuatsArrayPath);
  const auto& cellPhases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellPhasesArrayPath);
  const auto& avgCAxes = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->AvgCAxesArrayPath);

  auto& featRefCAxisMis = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->FeatureReferenceCAxisMisorientationsArrayName);
  auto& featAvgCAxisMis = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->FeatureAvgCAxisMisorientationsArrayName);
  auto& featStdevCAxisMis = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->FeatureStdevCAxisMisorientationsArrayName);

  const usize totalPoints = featureIds.getNumberOfTuples();
  const usize totalFeatures = avgCAxes.getNumberOfTuples();

  const int32 avgMisComps = 3;
  const usize numQuatComps = quats.getNumberOfComponents();
  std::vector<float32> avgMis(totalFeatures * avgMisComps, 0);

  float32 w = 0.0f;
  SizeVec3 uDims = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath).getDimensions();

  // We have more points than can be allocated on a 32 bit machine. Assert Now.
  if(totalPoints > std::numeric_limits<uint32>::max())
  {
    return MakeErrorResult(-9804, fmt::format("The volume is too large for a 32 bit machine. Try reducing the input volume size. Total Voxels: {}", totalPoints));
  }

  const auto xPoints = static_cast<int64>(uDims[0]);
  const auto yPoints = static_cast<int64>(uDims[1]);
  const auto zPoints = static_cast<int64>(uDims[2]);
  int64 point = 0;

  Matrix3fR g1T;
  g1T.fill(0.0f);
  const Eigen::Vector3f cAxis{0.0f, 0.0f, 1.0f};
  Eigen::Vector3f c1{0.0f, 0.0f, 0.0f};
  Eigen::Vector3f avgCAxisMis = {0.0f, 0.0f, 0.0f};
  usize index = 0;
  for(int64 col = 0; col < xPoints; col++)
  {
    for(int64 row = 0; row < yPoints; row++)
    {
      for(int64 plane = 0; plane < zPoints; plane++)
      {
        point = (plane * xPoints * yPoints) + (row * xPoints) + col;
        const auto quatTupleIndex = point * numQuatComps;
        const auto crystalStructureType = crystalStructures[cellPhases[point]];
        const bool isHex = crystalStructureType == EbsdLib::CrystalStructure::Hexagonal_High || crystalStructureType == EbsdLib::CrystalStructure::Hexagonal_Low;
        if(featureIds[point] > 0 && cellPhases[point] > 0 && isHex)
        {
          OrientationF oMatrix =
              OrientationTransformation::qu2om<QuatF, Orientation<float32>>({quats[quatTupleIndex], quats[quatTupleIndex + 1], quats[quatTupleIndex + 2], quats[quatTupleIndex + 3]});
          // transpose the g matrices so when caxis is multiplied by it, it will give the sample direction that the caxis is along
          g1T = OrientationMatrixToGMatrixTranspose(oMatrix);
          c1 = g1T * cAxis;
          // normalize so that the magnitude is 1
          c1.normalize();

          avgCAxisMis[0] = avgCAxes[3 * featureIds[point]];
          avgCAxisMis[1] = avgCAxes[3 * featureIds[point] + 1];
          avgCAxisMis[2] = avgCAxes[3 * featureIds[point] + 2];
          // normalize so that the magnitude is 1
          avgCAxisMis.normalize();
          w = ImageRotationUtilities::CosBetweenVectors(c1, avgCAxisMis);
          w = std::clamp(w, -1.0f, 1.0f);
          w = acosf(w);
          w *= Constants::k_180OverPiF;
          if(w > 90.0f)
          {
            w = 180.0f - w;
          }

          featRefCAxisMis[point] = w;
          index = featureIds[point] * avgMisComps;
          avgMis[index]++;
          avgMis[index + 1] += w;
        }
        else
        {
          featRefCAxisMis[point] = 0.0f;
        }
      }
    }
  }

  for(usize i = 1; i < totalFeatures; i++)
  {
    if(i % 1000 == 0)
    {
      m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Working On Feature {} of {}", i, totalFeatures));
    }
    index = i * avgMisComps;
    if(avgMis[index] == 0.0f)
    {
      featAvgCAxisMis[i] = 0.0f;
    }
    else
    {
      featAvgCAxisMis[i] = avgMis[index + 1] / avgMis[index];
    }
  }

  int32 gNum = 0;
  for(usize j = 0; j < totalPoints; j++)
  {
    gNum = featureIds[j];
    avgMis[(gNum * avgMisComps) + 2] += ((featRefCAxisMis[j] - featAvgCAxisMis[gNum]) * (featRefCAxisMis[j] - featAvgCAxisMis[gNum]));
  }

  for(usize i = 1; i < totalFeatures; i++)
  {
    index = i * avgMisComps;
    if(avgMis[index] == 0.0f)
    {
      featStdevCAxisMis[i] = 0.0f;
    }
    else
    {
      featStdevCAxisMis[i] = std::sqrt((1.0f / avgMis[index]) * avgMis[index + 2]);
    }
  }

  return {};
}
