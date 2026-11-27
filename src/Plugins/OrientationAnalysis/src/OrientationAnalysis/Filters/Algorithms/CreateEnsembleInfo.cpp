#include "CreateEnsembleInfo.hpp"

#include "OrientationAnalysis/utilities/PhaseType.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/StringArray.hpp"

#include <EbsdLib/Core/EbsdLibConstants.h>

using namespace nx::core;

namespace
{

// -----------------------------------------------------------------------------
uint32 CrystalStructureFromString(const std::string& structure)
{
  auto iterator = std::find(EnsembleInfoParameter::k_CrystalStructures.begin(), EnsembleInfoParameter::k_CrystalStructures.end(), structure);

  if(iterator == std::end(EnsembleInfoParameter::k_CrystalStructures))
  {
    return ebsdlib::CrystalStructure::UnknownCrystalStructure;
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

// -----------------------------------------------------------------------------
const std::atomic_bool& CreateEnsembleInfo::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> CreateEnsembleInfo::operator()()
{
  AttributeMatrix& cellEnsembleAttributeMatrix = m_DataStructure.getDataRefAs<AttributeMatrix>(m_InputValues->cell_ensemble_attribute_matrix_name);
  UInt32ArrayType& crystalStructures = m_DataStructure.getDataRefAs<UInt32ArrayType>(m_InputValues->cell_ensemble_attribute_matrix_name.createChildPath(m_InputValues->crystal_structures_array_name));
  UInt32ArrayType& phaseTypes = m_DataStructure.getDataRefAs<UInt32ArrayType>(m_InputValues->cell_ensemble_attribute_matrix_name.createChildPath(m_InputValues->phase_types_array_name));
  StringArray& phaseNames = m_DataStructure.getDataRefAs<StringArray>(m_InputValues->cell_ensemble_attribute_matrix_name.createChildPath(m_InputValues->phase_names_array_name));

  usize numPhases = m_InputValues->ensemble.size();
  if(0 == numPhases) // Either the group name "EnsembleInfo" is incorrect or 0 was entered as the Number_Phases
  {
    return MakeErrorResult(-10001, "Check the group name EnsembleInfo and that the number of phases > 0");
  }

  crystalStructures[0] = ebsdlib::CrystalStructure::UnknownCrystalStructure;
  phaseTypes[0] = static_cast<uint32>(PhaseType::Type::Unknown);

  for(int i = 0; i < numPhases; i++)
  {
    // Crystal Structure
    const uint32 crystalStructure = CrystalStructureFromString(m_InputValues->ensemble[i][0]);
    if(crystalStructure == ebsdlib::CrystalStructure::UnknownCrystalStructure)
    {
      return MakeErrorResult(-10006, fmt::format("Incorrect crystal structure name '{}' in ensemble row {}.", m_InputValues->ensemble[i][0], i + 1));
    }

    crystalStructures[i + 1] = crystalStructure;

    // Phase Type
    const uint32_t phaseType = PhaseTypeFromString(m_InputValues->ensemble[i][1]);
    if(static_cast<PhaseType::Type>(phaseType) == PhaseType::Type::Unknown)
    {
      return MakeErrorResult(-10007, fmt::format("Incorrect phase type '{}' in ensemble row {}.", m_InputValues->ensemble[i][1], i + 1));
    }

    phaseTypes[i + 1] = phaseType;

    // Phase Name
    std::string phaseName = m_InputValues->ensemble[i][2];
    if(phaseName.empty())
    {
      return MakeErrorResult(-10008, fmt::format("Phase name cannot be empty in ensemble row {}.", i + 1)); // The phase name was not found
    }

    phaseNames[i + 1] = phaseName;
  }

  return {};
}
