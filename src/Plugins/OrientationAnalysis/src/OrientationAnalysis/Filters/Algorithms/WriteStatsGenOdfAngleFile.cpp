#include "WriteStatsGenOdfAngleFile.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

class QFile;
using namespace nx::core;

namespace
{
constexpr std::array<char, 5> k_Delimiters = {',', ';', ' ', '"', '\t'};
const std::array<std::string, 5> k_DelimiterStr = {"Comma", "Semicolon", "Space", "Colon", "Tab"};
} // namespace

// -----------------------------------------------------------------------------
WriteStatsGenOdfAngleFile::WriteStatsGenOdfAngleFile(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                     WriteStatsGenOdfAngleFileInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
WriteStatsGenOdfAngleFile::~WriteStatsGenOdfAngleFile() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& WriteStatsGenOdfAngleFile::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> WriteStatsGenOdfAngleFile::operator()()
{
  Result<> results;

  // Make sure any directory path is also available as the user may have just typed
  // in a path without actually creating the full path
  Result<> createDirectoriesResult = CreateOutputDirectories(m_InputValues->OutputFile.parent_path());
  if(createDirectoriesResult.invalid())
  {
    return createDirectoriesResult;
  }

  const auto& cellPhases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellPhasesArrayPath);
  std::unique_ptr<MaskCompare> maskPtr = nullptr;
  if(m_InputValues->UseMask)
  {
    maskPtr = InstantiateMaskCompare(m_DataStructure, m_InputValues->MaskArrayPath);
  }

  // Figure out how many unique phase values we have by looping over all the phase values
  const usize totalPoints = cellPhases.getNumberOfTuples();
  std::set<int32> uniquePhases;
  for(usize i = 0; i < totalPoints; i++)
  {
    uniquePhases.insert(cellPhases[i]);
  }

  uniquePhases.erase(0); // remove Phase 0 as this is a Special Phase for DREAM3D

  const std::string absPath = std::filesystem::absolute(m_InputValues->OutputFile).parent_path().string();
  const std::string fName = m_InputValues->OutputFile.stem().string();
  const std::string suffix = m_InputValues->OutputFile.extension().string();

  for(const auto& phase : uniquePhases)
  {
    // Dry run to figure out how many lines we are going to have
    int32 const lineCount = determineOutputLineCount(cellPhases, maskPtr, totalPoints, phase);
    if(lineCount == 0)
    {
      results = MergeResults(results, MakeWarningVoidResult(-9403, fmt::format("No valid data for phase '{}'. No ODF Angle file written for phase.", phase)));
      continue;
    }

    m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Writing file for phase '{}'", phase));

    const std::string absFilePath = fmt::format("{}/{}_Phase_{}{}", absPath, fName, phase, suffix);

    std::ofstream file(absFilePath, std::ios::out | std::ios::trunc | std::ios_base::binary);
    if(!file.is_open())
    {
      return MakeErrorResult(-9404, fmt::format("Error creating output file '{}'", absFilePath));
    }

    Result<> writeResult = writeOutputFile(file, cellPhases, maskPtr, lineCount, totalPoints, phase);
    if(writeResult.invalid())
    {
      return writeResult;
    }
    file.flush();
    file.close();
  }

  return results;
}

// -----------------------------------------------------------------------------
int WriteStatsGenOdfAngleFile::determineOutputLineCount(const Int32Array& cellPhases, const std::unique_ptr<MaskCompare>& mask, usize totalPoints, int32 phase) const
{
  int32 lineCount = 0;
  for(usize i = 0; i < totalPoints; i++)
  {
    if(cellPhases[i] == phase)
    {
      if(!m_InputValues->UseMask || (m_InputValues->UseMask && mask->isTrue(i)))
      {
        lineCount++;
      }
    }
  }

  return lineCount;
}

// -----------------------------------------------------------------------------
Result<> WriteStatsGenOdfAngleFile::writeOutputFile(std::ofstream& out, const Int32Array& cellPhases, const std::unique_ptr<MaskCompare>& mask, int32 lineCount, usize totalPoints, int32 phase) const
{
  const auto& eulerAngles = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CellEulerAnglesArrayPath);

  out << "# All lines starting with '#' are comments and should come before the data.\n";
  out << "# DREAM.3D StatsGenerator ODF Angles Input File\n";
  out << "# DREAM.3D Version 7.0.0\n";

  const auto delimiter = k_Delimiters[m_InputValues->Delimiter];

  out << "# Angle Data is " << k_DelimiterStr[m_InputValues->Delimiter] << " delimited.\n";

  if(m_InputValues->ConvertToDegrees)
  {
    out << "# Euler angles are expressed in degrees\n";
  }
  else
  {
    out << "# Euler angles are expressed in radians\n";
  }
  out << "# Euler0 Euler1 Euler2 Weight Sigma\n";
  out << "Angle Count:" << lineCount << "\n";

  for(usize i = 0; i < totalPoints; i++)
  {
    bool writeLine = false;

    if(cellPhases[i] == phase)
    {
      if(!m_InputValues->UseMask || (m_InputValues->UseMask && mask->isTrue(i)))
      {
        writeLine = true;
      }
    }

    if(writeLine)
    {
      float32 euler0 = eulerAngles[i * 3 + 0];
      float32 euler1 = eulerAngles[i * 3 + 1];
      float32 euler2 = eulerAngles[i * 3 + 2];
      if(m_InputValues->ConvertToDegrees)
      {
        euler0 = euler0 * Constants::k_180OverPiF;
        euler1 = euler1 * Constants::k_180OverPiF;
        euler2 = euler2 * Constants::k_180OverPiF;
      }
      out << std::fixed << std::setprecision(8) << euler0 << delimiter << euler1 << delimiter << euler2 << delimiter << m_InputValues->Weight << delimiter << m_InputValues->Sigma << "\n";
    }
  }

  return {};
}
