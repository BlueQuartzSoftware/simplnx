#include "FindCAxisLocations.hpp"

#include "OrientationAnalysis/utilities/OrientationUtilities.hpp"

#include "simplnx/DataStructure/DataArray.hpp"

#include "EbsdLib/Core/OrientationTransformation.hpp"
#include "EbsdLib/Core/Quaternion.hpp"
#include "EbsdLib/LaueOps/LaueOps.h"

using namespace nx::core;
using namespace nx::core::OrientationUtilities;

// -----------------------------------------------------------------------------
FindCAxisLocations::FindCAxisLocations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindCAxisLocationsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
FindCAxisLocations::~FindCAxisLocations() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& FindCAxisLocations::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> FindCAxisLocations::operator()()
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
    return MakeErrorResult(-3522, "Finding the c-axis locations requires at least one phase to be Hexagonal-Low 6/m or Hexagonal-High 6/mmm type crystal structures but none were found.");
  }

  Result<> result;
  if(!allPhasesHexagonal)
  {
    result.warnings().push_back({-3523, "Finding the c-axis locations requires Hexagonal-Low 6/m or Hexagonal-High 6/mmm type crystal structures. All calculations for non Hexagonal phases will be "
                                        "skipped and a NaN value inserted."});
  }

  std::vector<LaueOps::Pointer> m_OrientationOps = LaueOps::GetAllOrientationOps();

  const auto& quaternions = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->QuatsArrayPath);
  const auto& cellPhases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellPhasesArrayPath);
  auto& cAxisLocation = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CAxisLocationsArrayName);

  const usize totalPoints = quaternions.getNumberOfTuples();

  const Eigen::Vector3f cAxis{0.0f, 0.0f, 1.0f};
  Eigen::Vector3f c1{0.0f, 0.0f, 0.0f};

  usize index = 0;
  for(size_t i = 0; i < totalPoints; i++)
  {
    index = 3 * i;
    const auto crystalStructureType = crystalStructures[cellPhases[i]];
    if(crystalStructureType == EbsdLib::CrystalStructure::Hexagonal_High || crystalStructureType == EbsdLib::CrystalStructure::Hexagonal_Low)
    {
      const usize quatIndex = i * 4;
      OrientationF oMatrix = OrientationTransformation::qu2om<QuatF, OrientationF>({quaternions[quatIndex], quaternions[quatIndex + 1], quaternions[quatIndex + 2], quaternions[quatIndex + 3]});
      // transpose the g matrices so when c-axis is multiplied by it
      // it will give the sample direction that the c-axis is along
      c1 = OrientationMatrixToGMatrixTranspose(oMatrix) * cAxis;
      // normalize so that the magnitude is 1
      c1.normalize();
      if(c1[2] < 0)
      {
        c1 *= -1.0f;
      }
      cAxisLocation[index] = c1[0];
      cAxisLocation[index + 1] = c1[1];
      cAxisLocation[index + 2] = c1[2];
    }
    else
    {
      cAxisLocation[index] = NAN;
      cAxisLocation[index + 1] = NAN;
      cAxisLocation[index + 2] = NAN;
    }
  }
  return result;
}
