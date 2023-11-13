#include "ReadEnsembleInfo.hpp"

#include "OrientationAnalysis/utilities/PhaseType.hpp"
#include "OrientationAnalysis/utilities/inicpp.h"

#include "complex/DataStructure/AttributeMatrix.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/Utilities/StringUtilities.hpp"

#include "EbsdLib/Core/EbsdLibConstants.h"

using namespace complex;

namespace
{
const std::string k_CrystalStructure("CrystalStructure");
const std::string k_PhaseType("PhaseType");

const std::string k_HexagonalHigh("Hexagonal_High");
const std::string k_CubicHigh("Cubic_High");
const std::string k_HexagonalLow("Hexagonal_Low");
const std::string k_CubicLow("Cubic_Low");
const std::string k_TriClinic("Triclinic");
const std::string k_MonoClinic("Monoclinic");
const std::string k_OrthoRhombic("OrthoRhombic");
const std::string k_TetragonalLow("Tetragonal_Low");
const std::string k_TetragonalHigh("Tetragonal_High");
const std::string k_TrigonalLow("Trigonal_Low");
const std::string k_TrigonalHigh("Trigonal_High");

const std::string k_PrimaryPhase("PrimaryPhase");
const std::string k_PrecipitatePhase("PrecipitatePhase");
const std::string k_TransformationPhase("TransformationPhase");
const std::string k_MatrixPhase("MatrixPhase");
const std::string k_BoundaryPhase("BoundaryPhase");
const std::string k_Unknown("Unknown");

std::map<std::string, uint32> k_EnsembleCrystalInfoLookup = {
    {k_HexagonalHigh, EbsdLib::CrystalStructure::Hexagonal_High}, {k_CubicHigh, EbsdLib::CrystalStructure::Cubic_High},         {k_HexagonalLow, EbsdLib::CrystalStructure::Hexagonal_Low},
    {k_CubicLow, EbsdLib::CrystalStructure::Cubic_Low},           {k_TriClinic, EbsdLib::CrystalStructure::Triclinic},          {k_MonoClinic, EbsdLib::CrystalStructure::Monoclinic},
    {k_OrthoRhombic, EbsdLib::CrystalStructure::OrthoRhombic},    {k_TetragonalLow, EbsdLib::CrystalStructure::Tetragonal_Low}, {k_TetragonalHigh, EbsdLib::CrystalStructure::Tetragonal_High},
    {k_TrigonalLow, EbsdLib::CrystalStructure::Trigonal_Low},     {k_TrigonalHigh, EbsdLib::CrystalStructure::Trigonal_High}};

std::map<std::string, uint32> k_EnsemblePhaseTypeInfoLookup = {{k_PrimaryPhase, static_cast<PhaseType::EnumType>(PhaseType::Type::Primary)},
                                                               {k_PrecipitatePhase, static_cast<PhaseType::EnumType>(PhaseType::Type::Precipitate)},
                                                               {k_TransformationPhase, static_cast<PhaseType::EnumType>(PhaseType::Type::Transformation)},
                                                               {k_MatrixPhase, static_cast<PhaseType::EnumType>(PhaseType::Type::Matrix)},
                                                               {k_BoundaryPhase, static_cast<PhaseType::EnumType>(PhaseType::Type::Boundary)},
                                                               {k_Unknown, static_cast<PhaseType::EnumType>(PhaseType::Type::Unknown)}};
} // namespace

// -----------------------------------------------------------------------------
ReadEnsembleInfo::ReadEnsembleInfo(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadEnsembleInfoInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ReadEnsembleInfo::~ReadEnsembleInfo() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ReadEnsembleInfo::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ReadEnsembleInfo::operator()()
{
  return readFile();
}

// -----------------------------------------------------------------------------
Result<> ReadEnsembleInfo::readFile()
{
  auto& cellEnsembleAttributeMatrix = m_DataStructure.getDataRefAs<AttributeMatrix>(m_InputValues->CellEnsembleAttributeMatrixName);
  auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayName);
  auto& phaseTypes = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->PhaseTypesArrayName);

  int32 numPhases = 0;
  ini::IniFile inputIni;
  inputIni.load(m_InputValues->InputFile);
  numPhases = inputIni[k_EnsembleInfo][k_NumberPhases].as<int32>(); // read number of phases from input file

  // Figure out if we are reading contiguous groups
  std::vector<bool> visited(numPhases + 1, false);
  visited[0] = true; // this is DREAM3D's internal, which is always visited.

  crystalStructures.fill(EbsdLib::CrystalStructure::UnknownCrystalStructure);
  phaseTypes.fill(static_cast<PhaseType::EnumType>(PhaseType::Type::Unknown));
  for(int32 index = 1; index < numPhases + 1; index++)
  {
    std::string group = StringUtilities::number(index);

    // Check to make sure the user has something for each of the Crystal Structure and Phase Type
    if(inputIni.count(group) == 0)
    {
      return MakeErrorResult(-13004, fmt::format("Could not find the group name {} from the input file '{}'", group, m_InputValues->InputFile));
    }
    if(inputIni[group].count(k_CrystalStructure) == 0)
    {
      return MakeErrorResult(-13005, fmt::format("Could not find the key {} in phase group {} from the input file '{}'", group, k_CrystalStructure, m_InputValues->InputFile));
    }
    if(inputIni[group].count(k_PhaseType) == 0)
    {
      return MakeErrorResult(-13006, fmt::format("Could not find the key {} in phase group {} from the input file '{}'", group, k_PhaseType, m_InputValues->InputFile));
    }

    auto crystalString = inputIni[group][k_CrystalStructure].as<std::string>();
    auto phaseTypeString = inputIni[group][k_PhaseType].as<std::string>();

    uint32 crystalStruct = EbsdLib::CrystalStructure::UnknownCrystalStructure;
    if(k_EnsembleCrystalInfoLookup.count(crystalString) > 0)
    {
      crystalStruct = k_EnsembleCrystalInfoLookup[crystalString];
    }
    auto pType = static_cast<PhaseType::EnumType>(PhaseType::Type::Unknown);
    if(k_EnsemblePhaseTypeInfoLookup.count(phaseTypeString) > 0)
    {
      pType = k_EnsemblePhaseTypeInfoLookup[phaseTypeString];
    }

    // Check to see if the Crystal Structure string was valid
    if(crystalStruct == EbsdLib::CrystalStructure::UnknownCrystalStructure) // The crystal structure name read from the file was not found in the lookup table
    {
      return MakeErrorResult(-13007, fmt::format("Incorrect crystal structure name '{}'", crystalString));
    }

    crystalStructures[index] = crystalStruct;

    // now check to see if the Phase type string was valid.
    if(pType == static_cast<PhaseType::EnumType>(PhaseType::Type::Unknown))
    {
      return MakeErrorResult(-13008, fmt::format("Incorrect phase type name '{}'", phaseTypeString));
    }

    phaseTypes[index] = pType;

    visited[index] = true;
  }

  // Make sure we visited all the groups.
  for(std::vector<bool>::size_type i = 0; i < visited.size(); i++)
  {
    if(!visited[i])
    {
      return MakeErrorResult(-13009, fmt::format("Phase '{}' did not have entries in the file. Phase numbering must start at 1 and no phases may be skipped",
                                                 i)); // The phase type name read from the file was not found in the lookup table
    }
  }

  return {};
}