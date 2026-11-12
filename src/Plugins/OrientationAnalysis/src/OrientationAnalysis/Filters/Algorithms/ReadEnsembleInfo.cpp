#include "ReadEnsembleInfo.hpp"

#include "OrientationAnalysis/utilities/PhaseType.hpp"
#include "OrientationAnalysis/utilities/inicpp.h"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <EbsdLib/Core/EbsdLibConstants.h>

using namespace nx::core;

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
    {k_HexagonalHigh, ebsdlib::CrystalStructure::Hexagonal_High}, {k_CubicHigh, ebsdlib::CrystalStructure::Cubic_High},         {k_HexagonalLow, ebsdlib::CrystalStructure::Hexagonal_Low},
    {k_CubicLow, ebsdlib::CrystalStructure::Cubic_Low},           {k_TriClinic, ebsdlib::CrystalStructure::Triclinic},          {k_MonoClinic, ebsdlib::CrystalStructure::Monoclinic},
    {k_OrthoRhombic, ebsdlib::CrystalStructure::OrthoRhombic},    {k_TetragonalLow, ebsdlib::CrystalStructure::Tetragonal_Low}, {k_TetragonalHigh, ebsdlib::CrystalStructure::Tetragonal_High},
    {k_TrigonalLow, ebsdlib::CrystalStructure::Trigonal_Low},     {k_TrigonalHigh, ebsdlib::CrystalStructure::Trigonal_High}};

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
  auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayName);
  auto& phaseTypes = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->PhaseTypesArrayName);

  auto phases = ReadEnsembleFile(m_InputValues->InputFile);
  if(phases.invalid())
  {
    return ConvertResult(std::move(phases));
  }
  crystalStructures.fill(ebsdlib::CrystalStructure::UnknownCrystalStructure);
  phaseTypes.fill(static_cast<uint32>(PhaseType::Type::Unknown));
  for(usize index = 0; index < phases.value().size(); ++index)
  {
    crystalStructures[index] = phases.value()[index][0];
    phaseTypes[index] = phases.value()[index][1];
  }
  return {};
}

Result<std::vector<std::array<uint32, 2>>> ReadEnsembleInfo::ReadEnsembleFile(const std::string& file)
{
  using PhaseValuesType = std::vector<std::array<uint32, 2>>;
  ini::IniFile inputIni;
  inputIni.load(file);
  if(inputIni.count(k_EnsembleInfo) == 0)
  {
    return MakeErrorResult<PhaseValuesType>(-13001, fmt::format("File '{}' has no group '{}'.", file, k_EnsembleInfo));
  }
  if(inputIni[k_EnsembleInfo].count(k_NumberPhases) == 0)
  {
    return MakeErrorResult<PhaseValuesType>(-13002, fmt::format("File '{}' group '{}' has no key '{}'.", file, k_EnsembleInfo, k_NumberPhases));
  }
  const int32 numPhases = inputIni[k_EnsembleInfo][k_NumberPhases].as<int32>();
  if(numPhases <= 0)
  {
    return MakeErrorResult<PhaseValuesType>(-13003, fmt::format("File '{}' has Number_Phases = {}. Use a positive phase count.", file, numPhases));
  }
  PhaseValuesType values{{ebsdlib::CrystalStructure::UnknownCrystalStructure, static_cast<uint32>(PhaseType::Type::Unknown)}};
  for(int64 index = 1; index <= numPhases; ++index)
  {
    const std::string group = std::to_string(index);
    if(inputIni.count(group) == 0)
    {
      return MakeErrorResult<PhaseValuesType>(-13004, fmt::format("File '{}' has no phase group '{}'. Number phases consecutively from 1.", file, group));
    }
    if(inputIni[group].count(k_CrystalStructure) == 0)
    {
      return MakeErrorResult<PhaseValuesType>(-13005, fmt::format("File '{}' phase group '{}' has no key '{}'.", file, group, k_CrystalStructure));
    }
    if(inputIni[group].count(k_PhaseType) == 0)
    {
      return MakeErrorResult<PhaseValuesType>(-13006, fmt::format("File '{}' phase group '{}' has no key '{}'.", file, group, k_PhaseType));
    }
    const auto crystalString = inputIni[group][k_CrystalStructure].as<std::string>();
    const auto phaseString = inputIni[group][k_PhaseType].as<std::string>();
    const auto crystal = k_EnsembleCrystalInfoLookup.find(crystalString);
    const auto phase = k_EnsemblePhaseTypeInfoLookup.find(phaseString);
    if(crystal == k_EnsembleCrystalInfoLookup.end())
    {
      return MakeErrorResult<PhaseValuesType>(
          -13007, fmt::format("File '{}' phase '{}' has invalid CrystalStructure '{}'. Use a crystal structure listed in the filter documentation.", file, group, crystalString));
    }
    if(phase == k_EnsemblePhaseTypeInfoLookup.end() || phase->second == static_cast<uint32>(PhaseType::Type::Unknown))
    {
      return MakeErrorResult<PhaseValuesType>(-13008, fmt::format("File '{}' phase '{}' has invalid PhaseType '{}'. Use a phase type listed in the filter documentation.", file, group, phaseString));
    }
    values.push_back({crystal->second, phase->second});
  }
  return {std::move(values)};
}
