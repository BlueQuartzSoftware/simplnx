#include "FindAvgCAxes.hpp"

#include "OrientationAnalysis/utilities/OrientationUtilities.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/Utilities/ImageRotationUtilities.hpp"

#include "EbsdLib/Core/Orientation.hpp"
#include "EbsdLib/Core/OrientationTransformation.hpp"
#include "EbsdLib/Core/Quaternion.hpp"

using namespace complex;
using namespace complex::OrientationUtilities;

// -----------------------------------------------------------------------------
FindAvgCAxes::FindAvgCAxes(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindAvgCAxesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
FindAvgCAxes::~FindAvgCAxes() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& FindAvgCAxes::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> FindAvgCAxes::operator()()
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
    return MakeErrorResult(-6402, "Finding the average c-axes requires at least one phase to be Hexagonal-Low 6/m or Hexagonal-High 6/mmm type crystal structures but none were found.");
  }

  Result<> result;
  if(!allPhasesHexagonal)
  {
    result.warnings().push_back(
        {-6403,
         "Finding the average c-axes requires Hexagonal-Low 6/m or Hexagonal-High 6/mmm type crystal structures. All calculations for non Hexagonal phases will be skipped and a NaN value inserted."});
  }

  const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  const auto& quats = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->QuatsArrayPath);
  const auto& cellPhases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellPhasesArrayPath);
  auto& avgCAxes = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->AvgCAxesArrayPath);

  const usize totalPoints = featureIds.getNumberOfTuples();
  const usize totalFeatures = avgCAxes.getNumberOfTuples();

  Matrix3fR g1T;
  g1T.fill(0.0f);
  const Eigen::Vector3f cAxis{0.0f, 0.0f, 1.0f};
  Eigen::Vector3f c1{0.0f, 0.0f, 0.0f};

  std::vector<int32> counter(totalFeatures, 0);

  Eigen::Vector3f curCAxis{0.0f, 0.0f, 0.0f};
  usize cAxesIndex = 0;
  float32 w = 0.0f;

  for(usize i = 0; i < totalPoints; i++)
  {
    if(featureIds[i] > 0)
    {
      cAxesIndex = 3 * featureIds[i];
      const auto crystalStructureType = crystalStructures[cellPhases[i]];
      if(crystalStructureType == EbsdLib::CrystalStructure::Hexagonal_High || crystalStructureType == EbsdLib::CrystalStructure::Hexagonal_Low)
      {
        const usize quatIndex = i * 4;
        OrientationF oMatrix = OrientationTransformation::qu2om<QuatF, OrientationF>({quats[quatIndex], quats[quatIndex + 1], quats[quatIndex + 2], quats[quatIndex + 3]});

        // Convert the quaternion matrix to a transposed g matrix so when caxis is multiplied by it, it will give the sample direction that the caxis is along
        g1T = OrientationMatrixToGMatrixTranspose(oMatrix);

        c1 = g1T * cAxis;

        // normalize so that the magnitude is 1
        c1.normalize();

        curCAxis[0] = avgCAxes[cAxesIndex] / counter[featureIds[i]];
        curCAxis[1] = avgCAxes[cAxesIndex + 1] / counter[featureIds[i]];
        curCAxis[2] = avgCAxes[cAxesIndex + 2] / counter[featureIds[i]];

        curCAxis.normalize();
        w = ImageRotationUtilities::CosBetweenVectors(c1, curCAxis);
        if(w < 0)
        {
          c1 *= -1.0f;
        }
        counter[featureIds[i]]++;
        avgCAxes[cAxesIndex] += c1[0];
        avgCAxes[cAxesIndex + 1] += c1[1];
        avgCAxes[cAxesIndex + 2] += c1[2];
      }
      else
      {
        avgCAxes[cAxesIndex] = NAN;
        avgCAxes[cAxesIndex + 1] = NAN;
        avgCAxes[cAxesIndex + 2] = NAN;
      }
    }
  }

  for(size_t i = 1; i < totalFeatures; i++)
  {
    const usize tupleIndex = i * 3;
    if(std::isnan(avgCAxes[tupleIndex]))
    {
      continue;
    }

    if(counter[i] == 0)
    {
      avgCAxes[tupleIndex] = 0;
      avgCAxes[tupleIndex + 1] = 0;
      avgCAxes[tupleIndex + 2] = 1;
    }
    else
    {
      avgCAxes[tupleIndex] /= counter[i];
      avgCAxes[tupleIndex + 1] /= counter[i];
      avgCAxes[tupleIndex + 2] /= counter[i];

      MatrixMath::Normalize3x1(avgCAxes[tupleIndex], avgCAxes[tupleIndex + 1], avgCAxes[tupleIndex +2]);
    }
  }
  return result;
}
