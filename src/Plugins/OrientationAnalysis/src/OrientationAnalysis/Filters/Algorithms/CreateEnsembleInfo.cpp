#include "CreateEnsembleInfo.hpp"

#include "OrientationAnalysis/utilities/PhaseType.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/StringArray.hpp"

#include "EbsdLib/Core/EbsdLibConstants.h"

using namespace nx::core;

namespace
{

// -----------------------------------------------------------------------------
uint32 CrystalStructureFromString(const std::string& structure)
{
  auto iterator = std::find(EnsembleInfoParameter::k_CrystalStructures.begin(), EnsembleInfoParameter::k_CrystalStructures.end(), structure);

  if(iterator == std::end(EnsembleInfoParameter::k_CrystalStructures))
  {
    return EbsdLib::CrystalStructure::UnknownCrystalStructure;
  }

  return static_cast<uint32>(iterator - EnsembleInfoParameter::k_CrystalStructures.begin());
}
// -----------------------------------------------------------------------------
uint32 PhaseTypeFromString(const std::string& phaseType)
{
  auto iterator = std::find(EnsembleInfoParameter::k_PhaseTypes.begin(), EnsembleInfoParameter::k_PhaseTypes.end(), phaseType);

  if(iterator == std::end(EnsembleInfoParameter::k_PhaseTypes))
  {
    return static_cast<uint32>(PhaseType::Type::Unknown);
  }

  return static_cast<uint32>(iterator - EnsembleInfoParameter::k_PhaseTypes.begin());
}
} // namespace

// -----------------------------------------------------------------------------
CreateEnsembleInfo::CreateEnsembleInfo(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CreateEnsembleInfoInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
CreateEnsembleInfo::~CreateEnsembleInfo() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& CreateEnsembleInfo::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> CreateEnsembleInfo::operator()()
{
  AttributeMatrix& cellEnsembleAttributeMatrix = m_DataStructure.getDataRefAs<AttributeMatrix>(m_InputValues->CellEnsembleAttributeMatrixName);
  UInt32Array& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CellEnsembleAttributeMatrixName.createChildPath(m_InputValues->CrystalStructuresArrayName));
  UInt32Array& phaseTypes = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CellEnsembleAttributeMatrixName.createChildPath(m_InputValues->PhaseTypesArrayName));
  StringArray& phaseNames = m_DataStructure.getDataRefAs<StringArray>(m_InputValues->CellEnsembleAttributeMatrixName.createChildPath(m_InputValues->PhaseNamesArrayName));

  usize numPhases = m_InputValues->Ensemble.size();
  if(0 == numPhases) // Either the group name "EnsembleInfo" is incorrect or 0 was entered as the Number_Phases
  {
    return MakeErrorResult(-10001, "Check the group name EnsembleInfo and that the number of phases > 0");
  }

  crystalStructures[0] = EbsdLib::CrystalStructure::UnknownCrystalStructure;
  phaseTypes[0] = static_cast<uint32>(PhaseType::Type::Unknown);

  for(int i = 0; i < numPhases; i++)
  {
    // Crystal Structure
    uint32 crystalStructure = CrystalStructureFromString(m_InputValues->Ensemble[i][0]);
    if(crystalStructure == EbsdLib::CrystalStructure::UnknownCrystalStructure)
    {
      return MakeErrorResult(-10006, fmt::format("Incorrect crystal structure name '{}'", crystalStructure));
    }

    crystalStructures[i + 1] = crystalStructure;

    // Phase Type
    uint32_t phaseType = PhaseTypeFromString(m_InputValues->Ensemble[i][1]); // static_cast<uint32_t>(phaseTypes[i]);
    if(static_cast<PhaseType::Type>(phaseType) == PhaseType::Type::Unknown)
    {
      return MakeErrorResult(-10007, fmt::format("Incorrect phase type '{}'", phaseType)); // The phase type name was not found in the lookup table
    }

    phaseTypes[i + 1] = phaseType;

    // Phase Name
    std::string phaseName = m_InputValues->Ensemble[i][2];
    if(phaseName.empty())
    {
      return MakeErrorResult(-10008, fmt::format("Phase name cannot be empty")); // The phase name was not found
    }

    phaseNames[i + 1] = phaseName;
  }

  return {};
}
