#include "WriteStatsGenOdfAngleFile.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/SIMPLNXVersion.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>

using namespace nx::core;

namespace
{
constexpr usize k_ChunkTuples = 65536;
constexpr usize k_EulerComponents = 3;
// Choice three is labeled Colon but currently writes a double quote. Preserve
// this mismatch until output-format behavior is changed explicitly.
constexpr std::array<char, 5> k_Delimiters = {',', ';', ' ', '"', '\t'};
const std::array<std::string, 5> k_DelimiterStr = {"Comma", "Semicolon", "Space", "Colon", "Tab"};

void WriteHeader(std::ofstream& out, const WriteStatsGenOdfAngleFileInputValues& inputValues, int32 lineCount)
{
  out << "# All lines starting with '#' are comments and should come before the data.\n";
  out << "# DREAM3D-NX StatsGenerator ODF Angles Input File\n";
  out << "# " << nx::core::Version::PackageComplete() << "\n";

  out << "# Angle Data is " << k_DelimiterStr[inputValues.Delimiter] << " delimited.\n";
  if(inputValues.ConvertToDegrees)
  {
    out << "# Euler angles are expressed in degrees\n";
  }
  else
  {
    out << "# Euler angles are expressed in radians\n";
  }
  out << "# Euler0 Euler1 Euler2 Weight Sigma\n";
  out << "Angle Count:" << lineCount << "\n";
}

void WriteEulerLine(std::ofstream& out, const WriteStatsGenOdfAngleFileInputValues& inputValues, float32 euler0, float32 euler1, float32 euler2)
{
  if(inputValues.ConvertToDegrees)
  {
    euler0 = euler0 * Constants::k_180OverPiF;
    euler1 = euler1 * Constants::k_180OverPiF;
    euler2 = euler2 * Constants::k_180OverPiF;
  }

  const char delimiter = k_Delimiters[inputValues.Delimiter];
  out << std::fixed << std::setprecision(8) << euler0 << delimiter << euler1 << delimiter << euler2 << delimiter << inputValues.Weight << delimiter << inputValues.Sigma << "\n";
}

Result<> MakeInvalidMaskTypeError(const IDataArray& maskArray, const DataPath& maskPath)
{
  return MakeErrorResult(-9405, fmt::format("Mask array '{}' has data type '{}'. Select a Bool or UInt8 mask array.", maskPath.toString(), DataTypeToString(maskArray.getDataType())));
}

constexpr StringLiteral k_InvalidMaskCompareMessage = "Mask comparator must be a BoolMaskCompare or UInt8MaskCompare.";

template <typename MaskT>
Result<int32> DetermineOutputLineCount(const Int32AbstractDataStore& phasesStore, const AbstractDataStore<MaskT>* maskStore, usize totalPoints, int32 phase)
{
  auto phasesBuffer = std::make_unique<int32[]>(k_ChunkTuples);
  std::unique_ptr<MaskT[]> maskBuffer;
  if(maskStore != nullptr)
  {
    maskBuffer = std::make_unique<MaskT[]>(k_ChunkTuples);
  }

  int32 lineCount = 0;
  for(usize tupleOffset = 0; tupleOffset < totalPoints; tupleOffset += k_ChunkTuples)
  {
    const usize tupleCount = std::min(k_ChunkTuples, totalPoints - tupleOffset);
    Result<> readResult = phasesStore.copyIntoBuffer(tupleOffset, nonstd::span<int32>(phasesBuffer.get(), tupleCount));
    if(readResult.invalid())
    {
      return ConvertResultTo<int32>(std::move(readResult), int32{0});
    }
    if(maskStore != nullptr)
    {
      readResult = maskStore->copyIntoBuffer(tupleOffset, nonstd::span<MaskT>(maskBuffer.get(), tupleCount));
      if(readResult.invalid())
      {
        return ConvertResultTo<int32>(std::move(readResult), int32{0});
      }
    }

    for(usize chunkIndex = 0; chunkIndex < tupleCount; chunkIndex++)
    {
      if(phasesBuffer[chunkIndex] == phase && (maskStore == nullptr || static_cast<bool>(maskBuffer[chunkIndex])))
      {
        lineCount++;
      }
    }
  }

  return {lineCount};
}

template <typename MaskT>
Result<> WriteOutputFile(std::ofstream& out, const Int32AbstractDataStore& phasesStore, const Float32AbstractDataStore& eulersStore, const AbstractDataStore<MaskT>* maskStore,
                         const WriteStatsGenOdfAngleFileInputValues& inputValues, int32 lineCount, usize totalPoints, int32 phase)
{
  WriteHeader(out, inputValues, lineCount);

  auto phasesBuffer = std::make_unique<int32[]>(k_ChunkTuples);
  auto eulersBuffer = std::make_unique<float32[]>(k_ChunkTuples * k_EulerComponents);
  std::unique_ptr<MaskT[]> maskBuffer;
  if(maskStore != nullptr)
  {
    maskBuffer = std::make_unique<MaskT[]>(k_ChunkTuples);
  }

  for(usize tupleOffset = 0; tupleOffset < totalPoints; tupleOffset += k_ChunkTuples)
  {
    const usize tupleCount = std::min(k_ChunkTuples, totalPoints - tupleOffset);
    Result<> readResult = phasesStore.copyIntoBuffer(tupleOffset, nonstd::span<int32>(phasesBuffer.get(), tupleCount));
    if(readResult.invalid())
    {
      return readResult;
    }
    if(maskStore != nullptr)
    {
      readResult = maskStore->copyIntoBuffer(tupleOffset, nonstd::span<MaskT>(maskBuffer.get(), tupleCount));
      if(readResult.invalid())
      {
        return readResult;
      }
    }

    bool hasOutput = false;
    for(usize chunkIndex = 0; chunkIndex < tupleCount; chunkIndex++)
    {
      if(phasesBuffer[chunkIndex] == phase && (maskStore == nullptr || static_cast<bool>(maskBuffer[chunkIndex])))
      {
        hasOutput = true;
        break;
      }
    }

    if(!hasOutput)
    {
      continue;
    }

    const usize eulerOffset = tupleOffset * k_EulerComponents;
    const usize eulerCount = tupleCount * k_EulerComponents;
    readResult = eulersStore.copyIntoBuffer(eulerOffset, nonstd::span<float32>(eulersBuffer.get(), eulerCount));
    if(readResult.invalid())
    {
      return readResult;
    }

    for(usize chunkIndex = 0; chunkIndex < tupleCount; chunkIndex++)
    {
      if(phasesBuffer[chunkIndex] != phase || (maskStore != nullptr && !static_cast<bool>(maskBuffer[chunkIndex])))
      {
        continue;
      }

      const usize localEulerOffset = chunkIndex * k_EulerComponents;
      WriteEulerLine(out, inputValues, eulersBuffer[localEulerOffset], eulersBuffer[localEulerOffset + 1], eulersBuffer[localEulerOffset + 2]);
    }
  }

  return {};
}

template <typename WritePhaseFunction>
Result<> WritePhaseFiles(const std::map<int32, int32>& phaseLineCounts, const WriteStatsGenOdfAngleFileInputValues& inputValues, const IFilter::MessageHandler& messageHandler,
                         const std::atomic_bool& shouldCancel, WritePhaseFunction&& writePhase)
{
  Result<> results;
  const std::string absPath = std::filesystem::absolute(inputValues.OutputFile).parent_path().string();
  const std::string fileName = inputValues.OutputFile.stem().string();
  const std::string suffix = inputValues.OutputFile.extension().string();

  for(const auto& [phase, lineCount] : phaseLineCounts)
  {
    if(shouldCancel)
    {
      return results;
    }

    if(lineCount == 0)
    {
      results = MergeResults(results, MakeWarningVoidResult(-9403, fmt::format("No valid data for phase '{}'. No ODF Angle file written for phase.", phase)));
      continue;
    }

    messageHandler(IFilter::Message::Type::Info, fmt::format("Writing file for phase '{}'", phase));
    const std::string absFilePath = fmt::format("{}/{}_Phase_{}{}", absPath, fileName, phase, suffix);
    std::ofstream file(absFilePath, std::ios::out | std::ios::trunc | std::ios_base::binary);
    if(!file.is_open())
    {
      return MakeErrorResult(-9404, fmt::format("Error creating output file '{}'", absFilePath));
    }

    WriteHeader(file, inputValues, lineCount);
    Result<> writeResult = writePhase(file, phase);
    if(writeResult.invalid())
    {
      return writeResult;
    }
    file.flush();
    file.close();
  }

  return results;
}

template <typename MaskT>
Result<> ExecuteDirect(const Int32DataStore& phasesStore, const Float32DataStore& eulersStore, const DataStore<MaskT>* maskStore, const WriteStatsGenOdfAngleFileInputValues& inputValues,
                       const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel)
{
  const usize totalPoints = phasesStore.getNumberOfTuples();
  const int32* phases = phasesStore.data();
  const float32* eulers = eulersStore.data();
  const MaskT* mask = maskStore == nullptr ? nullptr : maskStore->data();

  std::map<int32, int32> phaseLineCounts;
  for(usize chunkOffset = 0; chunkOffset < totalPoints; chunkOffset += k_ChunkTuples)
  {
    if(shouldCancel)
    {
      return {};
    }

    const usize chunkEnd = std::min(chunkOffset + k_ChunkTuples, totalPoints);
    for(usize tupleIndex = chunkOffset; tupleIndex < chunkEnd; tupleIndex++)
    {
      const int32 phase = phases[tupleIndex];
      if(phase == 0)
      {
        continue;
      }

      auto phaseIter = phaseLineCounts.try_emplace(phase, 0).first;
      if(mask == nullptr || static_cast<bool>(mask[tupleIndex]))
      {
        phaseIter->second++;
      }
    }
  }

  return WritePhaseFiles(phaseLineCounts, inputValues, messageHandler, shouldCancel, [&](std::ofstream& file, int32 phase) -> Result<> {
    for(usize chunkOffset = 0; chunkOffset < totalPoints; chunkOffset += k_ChunkTuples)
    {
      if(shouldCancel)
      {
        return {};
      }

      const usize chunkEnd = std::min(chunkOffset + k_ChunkTuples, totalPoints);
      for(usize tupleIndex = chunkOffset; tupleIndex < chunkEnd; tupleIndex++)
      {
        if(phases[tupleIndex] != phase || (mask != nullptr && !static_cast<bool>(mask[tupleIndex])))
        {
          continue;
        }

        const usize eulerOffset = tupleIndex * k_EulerComponents;
        WriteEulerLine(file, inputValues, eulers[eulerOffset], eulers[eulerOffset + 1], eulers[eulerOffset + 2]);
      }
    }
    return {};
  });
}

template <typename MaskT>
Result<> ExecuteScanline(const Int32AbstractDataStore& phasesStore, const Float32AbstractDataStore& eulersStore, const AbstractDataStore<MaskT>* maskStore,
                         const WriteStatsGenOdfAngleFileInputValues& inputValues, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel)
{
  const usize totalPoints = phasesStore.getNumberOfTuples();
  auto phasesBuffer = std::make_unique<int32[]>(k_ChunkTuples);
  auto eulersBuffer = std::make_unique<float32[]>(k_ChunkTuples * k_EulerComponents);
  std::unique_ptr<MaskT[]> maskBuffer;
  if(maskStore != nullptr)
  {
    maskBuffer = std::make_unique<MaskT[]>(k_ChunkTuples);
  }

  std::map<int32, int32> phaseLineCounts;
  for(usize tupleOffset = 0; tupleOffset < totalPoints; tupleOffset += k_ChunkTuples)
  {
    if(shouldCancel)
    {
      return {};
    }

    const usize tupleCount = std::min(k_ChunkTuples, totalPoints - tupleOffset);
    Result<> readResult = phasesStore.copyIntoBuffer(tupleOffset, nonstd::span<int32>(phasesBuffer.get(), tupleCount));
    if(readResult.invalid())
    {
      return readResult;
    }
    if(maskStore != nullptr)
    {
      readResult = maskStore->copyIntoBuffer(tupleOffset, nonstd::span<MaskT>(maskBuffer.get(), tupleCount));
      if(readResult.invalid())
      {
        return readResult;
      }
    }

    for(usize chunkIndex = 0; chunkIndex < tupleCount; chunkIndex++)
    {
      const int32 phase = phasesBuffer[chunkIndex];
      if(phase == 0)
      {
        continue;
      }

      auto phaseIter = phaseLineCounts.try_emplace(phase, 0).first;
      if(maskStore == nullptr || static_cast<bool>(maskBuffer[chunkIndex]))
      {
        phaseIter->second++;
      }
    }
  }

  return WritePhaseFiles(phaseLineCounts, inputValues, messageHandler, shouldCancel, [&](std::ofstream& file, int32 phase) -> Result<> {
    for(usize tupleOffset = 0; tupleOffset < totalPoints; tupleOffset += k_ChunkTuples)
    {
      if(shouldCancel)
      {
        return {};
      }

      const usize tupleCount = std::min(k_ChunkTuples, totalPoints - tupleOffset);
      Result<> readResult = phasesStore.copyIntoBuffer(tupleOffset, nonstd::span<int32>(phasesBuffer.get(), tupleCount));
      if(readResult.invalid())
      {
        return readResult;
      }
      if(maskStore != nullptr)
      {
        readResult = maskStore->copyIntoBuffer(tupleOffset, nonstd::span<MaskT>(maskBuffer.get(), tupleCount));
        if(readResult.invalid())
        {
          return readResult;
        }
      }

      bool hasOutput = false;
      for(usize chunkIndex = 0; chunkIndex < tupleCount; chunkIndex++)
      {
        if(phasesBuffer[chunkIndex] == phase && (maskStore == nullptr || static_cast<bool>(maskBuffer[chunkIndex])))
        {
          hasOutput = true;
          break;
        }
      }

      if(hasOutput)
      {
        const usize eulerOffset = tupleOffset * k_EulerComponents;
        const usize eulerCount = tupleCount * k_EulerComponents;
        readResult = eulersStore.copyIntoBuffer(eulerOffset, nonstd::span<float32>(eulersBuffer.get(), eulerCount));
        if(readResult.invalid())
        {
          return readResult;
        }

        for(usize chunkIndex = 0; chunkIndex < tupleCount; chunkIndex++)
        {
          if(phasesBuffer[chunkIndex] != phase || (maskStore != nullptr && !static_cast<bool>(maskBuffer[chunkIndex])))
          {
            continue;
          }

          const usize localEulerOffset = chunkIndex * k_EulerComponents;
          WriteEulerLine(file, inputValues, eulersBuffer[localEulerOffset], eulersBuffer[localEulerOffset + 1], eulersBuffer[localEulerOffset + 2]);
        }
      }
    }
    return {};
  });
}

/**
 * @class WriteStatsGenOdfAngleFileScanline
 * @brief Streams phase, mask, and Euler stores through fixed-size buffers.
 *
 * This sequential path propagates source bulk-read failures. Cancellation is
 * checked between pages and phase files.
 */
class WriteStatsGenOdfAngleFileScanline
{
public:
  WriteStatsGenOdfAngleFileScanline(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                    const WriteStatsGenOdfAngleFileInputValues* inputValues)
  : m_DataStructure(dataStructure)
  , m_InputValues(inputValues)
  , m_ShouldCancel(shouldCancel)
  , m_MessageHandler(messageHandler)
  {
  }

  Result<> operator()()
  {
    const auto& phasesStore = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellPhasesArrayPath).getDataStoreRef();
    const auto& eulersStore = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CellEulerAnglesArrayPath).getDataStoreRef();
    if(!m_InputValues->UseMask)
    {
      return ExecuteScanline<uint8>(phasesStore, eulersStore, nullptr, *m_InputValues, m_MessageHandler, m_ShouldCancel);
    }

    const auto& maskArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->MaskArrayPath);
    switch(maskArray.getDataType())
    {
    case DataType::boolean: {
      const auto& maskStore = maskArray.getIDataStoreRefAs<AbstractDataStore<bool>>();
      return ExecuteScanline(phasesStore, eulersStore, &maskStore, *m_InputValues, m_MessageHandler, m_ShouldCancel);
    }
    case DataType::uint8: {
      const auto& maskStore = maskArray.getIDataStoreRefAs<AbstractDataStore<uint8>>();
      return ExecuteScanline(phasesStore, eulersStore, &maskStore, *m_InputValues, m_MessageHandler, m_ShouldCancel);
    }
    default:
      return MakeInvalidMaskTypeError(maskArray, m_InputValues->MaskArrayPath);
    }
  }

private:
  DataStructure& m_DataStructure;
  const WriteStatsGenOdfAngleFileInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

/**
 * @class WriteStatsGenOdfAngleFileDirect
 * @brief Uses contiguous in-memory pointers and phase-sized state for the fast path.
 *
 * A non-DataStore input delegates the complete operation to the scanline path.
 */
class WriteStatsGenOdfAngleFileDirect
{
public:
  WriteStatsGenOdfAngleFileDirect(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                  const WriteStatsGenOdfAngleFileInputValues* inputValues)
  : m_DataStructure(dataStructure)
  , m_InputValues(inputValues)
  , m_ShouldCancel(shouldCancel)
  , m_MessageHandler(messageHandler)
  {
  }

  Result<> operator()()
  {
    const auto& phasesStore = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellPhasesArrayPath).getDataStoreRef();
    const auto& eulersStore = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CellEulerAnglesArrayPath).getDataStoreRef();
    const auto* directPhasesStore = dynamic_cast<const Int32DataStore*>(&phasesStore);
    const auto* directEulersStore = dynamic_cast<const Float32DataStore*>(&eulersStore);
    if(directPhasesStore == nullptr || directEulersStore == nullptr)
    {
      return WriteStatsGenOdfAngleFileScanline(m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues)();
    }

    if(!m_InputValues->UseMask)
    {
      return ExecuteDirect<uint8>(*directPhasesStore, *directEulersStore, nullptr, *m_InputValues, m_MessageHandler, m_ShouldCancel);
    }

    const auto& maskArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->MaskArrayPath);
    switch(maskArray.getDataType())
    {
    case DataType::boolean: {
      const auto& maskStore = maskArray.getIDataStoreRefAs<AbstractDataStore<bool>>();
      const auto* directMaskStore = dynamic_cast<const BoolDataStore*>(&maskStore);
      if(directMaskStore == nullptr)
      {
        return WriteStatsGenOdfAngleFileScanline(m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues)();
      }
      return ExecuteDirect(*directPhasesStore, *directEulersStore, directMaskStore, *m_InputValues, m_MessageHandler, m_ShouldCancel);
    }
    case DataType::uint8: {
      const auto& maskStore = maskArray.getIDataStoreRefAs<AbstractDataStore<uint8>>();
      const auto* directMaskStore = dynamic_cast<const UInt8DataStore*>(&maskStore);
      if(directMaskStore == nullptr)
      {
        return WriteStatsGenOdfAngleFileScanline(m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues)();
      }
      return ExecuteDirect(*directPhasesStore, *directEulersStore, directMaskStore, *m_InputValues, m_MessageHandler, m_ShouldCancel);
    }
    default:
      return MakeInvalidMaskTypeError(maskArray, m_InputValues->MaskArrayPath);
    }
  }

private:
  DataStructure& m_DataStructure;
  const WriteStatsGenOdfAngleFileInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace

WriteStatsGenOdfAngleFile::WriteStatsGenOdfAngleFile(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                     WriteStatsGenOdfAngleFileInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

WriteStatsGenOdfAngleFile::~WriteStatsGenOdfAngleFile() noexcept = default;

const std::atomic_bool& WriteStatsGenOdfAngleFile::getCancel()
{
  return m_ShouldCancel;
}

int WriteStatsGenOdfAngleFile::determineOutputLineCount(const Int32Array& cellPhases, const std::unique_ptr<MaskCompareUtilities::MaskCompare>& mask, usize totalPoints, int32 phase) const
{
  const auto& phasesStore = cellPhases.getDataStoreRef();
  Result<int32> countResult;
  if(!m_InputValues->UseMask)
  {
    countResult = DetermineOutputLineCount<uint8>(phasesStore, nullptr, totalPoints, phase);
  }
  else if(mask == nullptr)
  {
    throw std::invalid_argument(k_InvalidMaskCompareMessage);
  }
  else if(const auto* boolMask = dynamic_cast<const MaskCompareUtilities::BoolMaskCompare*>(mask.get()); boolMask != nullptr)
  {
    countResult = DetermineOutputLineCount(phasesStore, &boolMask->m_DataStore, totalPoints, phase);
  }
  else if(const auto* uint8Mask = dynamic_cast<const MaskCompareUtilities::UInt8MaskCompare*>(mask.get()); uint8Mask != nullptr)
  {
    countResult = DetermineOutputLineCount(phasesStore, &uint8Mask->m_DataStore, totalPoints, phase);
  }
  else
  {
    throw std::invalid_argument(k_InvalidMaskCompareMessage);
  }

  if(countResult.invalid())
  {
    const auto& errors = countResult.errors();
    throw std::runtime_error(errors.empty() ? "Failed to read phase or mask data." : errors.front().message);
  }
  return countResult.value();
}

Result<> WriteStatsGenOdfAngleFile::writeOutputFile(std::ofstream& out, const Int32Array& cellPhases, const std::unique_ptr<MaskCompareUtilities::MaskCompare>& mask, int32 lineCount,
                                                    usize totalPoints, int32 phase) const
{
  const auto& phasesStore = cellPhases.getDataStoreRef();
  const auto& eulersStore = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CellEulerAnglesArrayPath).getDataStoreRef();
  if(!m_InputValues->UseMask)
  {
    return WriteOutputFile<uint8>(out, phasesStore, eulersStore, nullptr, *m_InputValues, lineCount, totalPoints, phase);
  }
  if(mask == nullptr)
  {
    return MakeErrorResult(-9405, k_InvalidMaskCompareMessage);
  }
  if(const auto* boolMask = dynamic_cast<const MaskCompareUtilities::BoolMaskCompare*>(mask.get()); boolMask != nullptr)
  {
    return WriteOutputFile(out, phasesStore, eulersStore, &boolMask->m_DataStore, *m_InputValues, lineCount, totalPoints, phase);
  }
  if(const auto* uint8Mask = dynamic_cast<const MaskCompareUtilities::UInt8MaskCompare*>(mask.get()); uint8Mask != nullptr)
  {
    return WriteOutputFile(out, phasesStore, eulersStore, &uint8Mask->m_DataStore, *m_InputValues, lineCount, totalPoints, phase);
  }
  return MakeErrorResult(-9405, k_InvalidMaskCompareMessage);
}

Result<> WriteStatsGenOdfAngleFile::operator()()
{
  Result<> createDirectoriesResult = CreateOutputDirectories(m_InputValues->OutputFile.parent_path());
  if(createDirectoriesResult.invalid())
  {
    return createDirectoriesResult;
  }

  const auto& cellPhases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellPhasesArrayPath);
  const auto& eulerAngles = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CellEulerAnglesArrayPath);
  if(m_InputValues->UseMask)
  {
    const auto& maskArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->MaskArrayPath);
    return DispatchAlgorithm<WriteStatsGenOdfAngleFileDirect, WriteStatsGenOdfAngleFileScanline>({&cellPhases, &eulerAngles, &maskArray}, m_DataStructure, m_MessageHandler, m_ShouldCancel,
                                                                                                 m_InputValues);
  }

  return DispatchAlgorithm<WriteStatsGenOdfAngleFileDirect, WriteStatsGenOdfAngleFileScanline>({&cellPhases, &eulerAngles}, m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
}
