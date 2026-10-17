#include "WriteAbaqusCrystalPlasticity.hpp"
#include "SimplnxCore/utils/AbaqusElementSets.hpp"

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <numbers>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
constexpr std::string_view k_SeparatorLine = "** ----------------------------------------------------------------\n";

enum class WriteStatus
{
  Success,
  Cancelled,
  OpenError,
  WriteError
};

enum class FileIndex : usize
{
  Nodes = 0,
  Elems = 1,
  Sects = 2,
  Elset = 3,
  Master = 4
};

struct GrainData
{
  std::vector<int32> Phases;
  std::vector<std::array<float32, 3>> Orientations;
  std::vector<uint8> HasCells;
};

constexpr usize k_TextBufferSize = 64 * 1024;

WriteStatus FlushBuffer(std::ostream& output, fmt::memory_buffer& buffer)
{
  output.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
  buffer.clear();
  return output ? WriteStatus::Success : WriteStatus::WriteError;
}

WriteStatus CloseOutput(std::ofstream& output)
{
  output.close();
  return output ? WriteStatus::Success : WriteStatus::WriteError;
}

WriteStatus WriteNodes(const fs::path& filePath, const ImageGeom& imageGeom, const std::atomic_bool& shouldCancel, ThrottledMessageHandler& progressThrottle)
{
  std::ofstream output(filePath, std::ios::binary);
  if(!output.is_open())
  {
    return WriteStatus::OpenError;
  }

  const auto dimensions = imageGeom.getDimensions();
  const auto spacing = imageGeom.getSpacing();
  const auto origin = imageGeom.getOrigin();
  const std::array<usize, 3> nodeDimensions = {dimensions[0] + 1, dimensions[1] + 1, dimensions[2] + 1};
  fmt::memory_buffer buffer;
  fmt::format_to(std::back_inserter(buffer), "{}**\n*Node\n", k_SeparatorLine);
  progressThrottle.reset(nodeDimensions[2], "Writing Nodes (File 1/5)");

  for(usize z = 0; z < nodeDimensions[2]; z++)
  {
    if(shouldCancel)
    {
      return WriteStatus::Cancelled;
    }

    for(usize y = 0; y < nodeDimensions[1]; y++)
    {
      for(usize x = 0; x < nodeDimensions[0]; x++)
      {
        const usize index = z * nodeDimensions[0] * nodeDimensions[1] + y * nodeDimensions[0] + x;
        const float64 xCoordinate = static_cast<float64>(origin[0]) + static_cast<float64>(x) * static_cast<float64>(spacing[0]);
        const float64 yCoordinate = static_cast<float64>(origin[1]) + static_cast<float64>(y) * static_cast<float64>(spacing[1]);
        const float64 zCoordinate = static_cast<float64>(origin[2]) + static_cast<float64>(z) * static_cast<float64>(spacing[2]);
        fmt::format_to(std::back_inserter(buffer), "{}, {:.9g}, {:.9g}, {:.9g}\n", index + 1, xCoordinate, yCoordinate, zCoordinate);
        if(buffer.size() >= k_TextBufferSize)
        {
          if(FlushBuffer(output, buffer) == WriteStatus::WriteError)
          {
            return WriteStatus::WriteError;
          }
          if(shouldCancel)
          {
            return WriteStatus::Cancelled;
          }
        }
      }
    }

    if(FlushBuffer(output, buffer) == WriteStatus::WriteError)
    {
      return WriteStatus::WriteError;
    }
    progressThrottle.updateCount(z + 1);
  }

  fmt::format_to(std::back_inserter(buffer), "**\n{}**\n", k_SeparatorLine);
  if(FlushBuffer(output, buffer) == WriteStatus::WriteError)
  {
    return WriteStatus::WriteError;
  }
  return CloseOutput(output);
}

WriteStatus WriteElements(const fs::path& filePath, const ImageGeom& imageGeom, bool useReducedIntegration, const std::atomic_bool& shouldCancel, ThrottledMessageHandler& progressThrottle)
{
  std::ofstream output(filePath, std::ios::binary);
  if(!output.is_open())
  {
    return WriteStatus::OpenError;
  }

  const auto dimensions = imageGeom.getDimensions();
  const usize nodesX = dimensions[0] + 1;
  const usize nodesY = dimensions[1] + 1;
  fmt::memory_buffer buffer;
  fmt::format_to(std::back_inserter(buffer), "{}**\n*Element, type={}\n", k_SeparatorLine, useReducedIntegration ? "C3D8R" : "C3D8");
  progressThrottle.reset(dimensions[2], "Writing Elements (File 2/5)");

  for(usize z = 0; z < dimensions[2]; z++)
  {
    if(shouldCancel)
    {
      return WriteStatus::Cancelled;
    }

    for(usize y = 0; y < dimensions[1]; y++)
    {
      for(usize x = 0; x < dimensions[0]; x++)
      {
        const usize elementIndex = z * dimensions[0] * dimensions[1] + y * dimensions[0] + x;
        const usize nodeIndex = z * nodesX * nodesY + y * nodesX + x + 1;
        const usize node1 = nodeIndex;
        const usize node2 = nodeIndex + 1;
        const usize node3 = nodeIndex + nodesX + 1;
        const usize node4 = nodeIndex + nodesX;
        const usize node5 = nodeIndex + nodesX * nodesY;
        const usize node6 = nodeIndex + 1 + nodesX * nodesY;
        const usize node7 = nodeIndex + nodesX + nodesX * nodesY + 1;
        const usize node8 = nodeIndex + nodesX + nodesX * nodesY;
        fmt::format_to(std::back_inserter(buffer), "{}, {}, {}, {}, {}, {}, {}, {}, {}\n", elementIndex + 1, node1, node2, node3, node4, node5, node6, node7, node8);
        if(buffer.size() >= k_TextBufferSize)
        {
          if(FlushBuffer(output, buffer) == WriteStatus::WriteError)
          {
            return WriteStatus::WriteError;
          }
          if(shouldCancel)
          {
            return WriteStatus::Cancelled;
          }
        }
      }
    }

    if(FlushBuffer(output, buffer) == WriteStatus::WriteError)
    {
      return WriteStatus::WriteError;
    }
    progressThrottle.updateCount(z + 1);
  }

  fmt::format_to(std::back_inserter(buffer), "**\n{}**\n", k_SeparatorLine);
  if(FlushBuffer(output, buffer) == WriteStatus::WriteError)
  {
    return WriteStatus::WriteError;
  }
  return CloseOutput(output);
}

/**
 * @brief Streams phase-named element sets with bounded output buffering.
 * @param filePath Temporary element-set path.
 * @param grainData Supplies each grain's last-cell phase.
 * @param featureIdsRef Supplies scalar cell Feature IDs.
 * @param useOocAlgorithm Selects bounded sorting or the resident-store fallback.
 * @param shouldCancel Signals cancellation between chunks.
 * @param progressThrottle Receives grouping progress messages.
 * @return Source, sorter, open, or write error; success on completion or cancellation.
 * @pre grainData covers all IDs through the validated maximum.
 */
Result<> WriteElementSets(const fs::path& filePath, const GrainData& grainData, const Int32AbstractDataStore& featureIdsRef, bool useOocAlgorithm, const std::atomic_bool& shouldCancel,
                          ThrottledMessageHandler& progressThrottle)
{
  std::ofstream output(filePath, std::ios::binary);
  if(!output.is_open())
  {
    return MakeErrorResult(-12012, fmt::format("Could not open '{}' for writing.", filePath.string()));
  }
  output << fmt::format("{}**\n** The element sets\n*Elset, elset=cube, generate\n1, {}, 1\n**\n** Each Grain is made up of multiple elements\n**", k_SeparatorLine, featureIdsRef.getNumberOfTuples());
  Result<> result = AbaqusElementSets::write(
      progressThrottle, output, [&grainData](int32 grainId) { return fmt::format("\n*Elset, elset=Grain{}_Phase{}_set\n", grainId, grainData.Phases[grainId]); }, featureIdsRef,
      static_cast<int32>(grainData.Phases.size() - 1), useOocAlgorithm, shouldCancel);
  if(result.valid())
  {
    output << fmt::format("\n**\n{}**\n", k_SeparatorLine);
  }
  output.close();
  if(!output)
  {
    return MakeErrorResult(-12013, fmt::format("Writing to '{}' failed. Check available disk space.", filePath.string()));
  }
  return result;
}

std::vector<std::string> BuildUserMaterialConstants(usize grainId, int32 phaseId, const std::array<float32, 3>& orientation, const WriteAbaqusCrystalPlasticityInputValues& inputValues)
{
  std::vector<std::string> values;
  if(inputValues.IncludeGrainAndPhaseIds)
  {
    values.push_back(fmt::format("{}", grainId));
    values.push_back(fmt::format("{}", phaseId));
  }
  // Preflight guarantees the start index lands inside or directly after the user constants.
  const usize eulerInsertPosition = static_cast<usize>(inputValues.EulerAnglesStartIndex) - 1;
  for(const auto& row : inputValues.MaterialConstants)
  {
    if(values.size() == eulerInsertPosition)
    {
      std::ranges::transform(orientation, std::back_inserter(values), [](float32 angle) { return fmt::format("{}", angle); });
    }
    values.push_back(fmt::format("{}", row.empty() ? 0.0 : row.front()));
  }
  if(values.size() == eulerInsertPosition)
  {
    std::ranges::transform(orientation, std::back_inserter(values), [](float32 angle) { return fmt::format("{}", angle); });
  }
  return values;
}

WriteStatus WriteDataLines(std::ostream& output, fmt::memory_buffer& buffer, const std::vector<std::string>& values)
{
  constexpr usize k_ValuesPerLine = 8;
  for(usize index = 0; index < values.size(); index++)
  {
    if(index != 0)
    {
      fmt::format_to(std::back_inserter(buffer), "{}", index % k_ValuesPerLine == 0 ? "\n" : ", ");
    }
    fmt::format_to(std::back_inserter(buffer), "{}", values[index]);
    if(buffer.size() >= k_TextBufferSize && FlushBuffer(output, buffer) == WriteStatus::WriteError)
    {
      return WriteStatus::WriteError;
    }
  }
  fmt::format_to(std::back_inserter(buffer), "\n");
  return WriteStatus::Success;
}

WriteStatus WriteMaster(const fs::path& filePath, const WriteAbaqusCrystalPlasticityInputValues& inputValues, const GrainData& grainData, const std::atomic_bool& shouldCancel,
                        ThrottledMessageHandler& progressThrottle)
{
  std::ofstream output(filePath, std::ios::binary);
  if(!output.is_open())
  {
    return WriteStatus::OpenError;
  }

  fmt::memory_buffer buffer;
  fmt::format_to(std::back_inserter(buffer), "*Heading\n");
  fmt::format_to(std::back_inserter(buffer), "{}\n", inputValues.JobName);
  fmt::format_to(std::back_inserter(buffer), "** Job name : {}\n", inputValues.JobName);
  fmt::format_to(std::back_inserter(buffer), "*Preprint, echo = NO, model = NO, history = NO, contact = NO\n");
  fmt::format_to(std::back_inserter(buffer), "**\n** ----------------------------Geometry----------------------------\n**\n");
  fmt::format_to(std::back_inserter(buffer), "*Include, Input = {}_nodes.inp\n", inputValues.FilePrefix);
  fmt::format_to(std::back_inserter(buffer), "*Include, Input = {}_elems.inp\n", inputValues.FilePrefix);
  fmt::format_to(std::back_inserter(buffer), "*Include, Input = {}_elset.inp\n", inputValues.FilePrefix);
  fmt::format_to(std::back_inserter(buffer), "*Include, Input = {}_sects.inp\n", inputValues.FilePrefix);
  fmt::format_to(std::back_inserter(buffer), "**\n** ----------------------------Materials---------------------------\n**\n");

  const usize grainCount = grainData.Phases.size() - 1;
  progressThrottle.reset(grainCount, "Writing Master File (File 4/5)");
  for(usize grainId = 1; grainId <= grainCount; grainId++)
  {
    if(shouldCancel)
    {
      return WriteStatus::Cancelled;
    }

    const int32 phaseId = grainData.Phases[grainId];
    const auto& orientation = grainData.Orientations[grainId];
    const std::vector<std::string> constants = BuildUserMaterialConstants(grainId, phaseId, orientation, inputValues);
    fmt::format_to(std::back_inserter(buffer), "*Material, name=Grain{}_Phase{}_mat\n", grainId, phaseId);
    fmt::format_to(std::back_inserter(buffer), "*Depvar\n{}\n", inputValues.NumDepvar);
    fmt::format_to(std::back_inserter(buffer), "*User Material, constants={}{}\n", constants.size(), inputValues.UseUnsymmetricSolver ? ", unsymm" : "");
    if(WriteDataLines(output, buffer, constants) == WriteStatus::WriteError)
    {
      return WriteStatus::WriteError;
    }
    fmt::format_to(std::back_inserter(buffer), "*User Output Variables\n{}\n", inputValues.NumUserOutVar);

    if(FlushBuffer(output, buffer) == WriteStatus::WriteError)
    {
      return WriteStatus::WriteError;
    }
    progressThrottle.updateCount(grainId);
  }

  fmt::format_to(std::back_inserter(buffer), "**\n{}**\n", k_SeparatorLine);
  if(FlushBuffer(output, buffer) == WriteStatus::WriteError)
  {
    return WriteStatus::WriteError;
  }
  return CloseOutput(output);
}

WriteStatus WriteSections(const fs::path& filePath, const GrainData& grainData, bool useReducedIntegration, int32 hourglassStiffness, const std::atomic_bool& shouldCancel,
                          ThrottledMessageHandler& progressThrottle)
{
  std::ofstream output(filePath, std::ios::binary);
  if(!output.is_open())
  {
    return WriteStatus::OpenError;
  }

  const usize grainCount = grainData.Phases.size() - 1;
  fmt::memory_buffer buffer;
  fmt::format_to(std::back_inserter(buffer), "{}**\n** Each section is a separate grain\n", k_SeparatorLine);
  progressThrottle.reset(grainCount, "Writing Sections (File 5/5)");
  for(usize grainId = 1; grainId <= grainCount; grainId++)
  {
    if(shouldCancel)
    {
      return WriteStatus::Cancelled;
    }

    const int32 phaseId = grainData.Phases[grainId];
    fmt::format_to(std::back_inserter(buffer), "** Section: Grain{}_Phase{}\n", grainId, phaseId);
    fmt::format_to(std::back_inserter(buffer), "*Solid Section, elset=Grain{}_Phase{}_set, material=Grain{}_Phase{}_mat\n", grainId, phaseId, grainId, phaseId);
    if(useReducedIntegration)
    {
      fmt::format_to(std::back_inserter(buffer), "*Hourglass Stiffness\n{}\n", hourglassStiffness);
    }
    fmt::format_to(std::back_inserter(buffer), "** --------------------------------------\n");
    if(FlushBuffer(output, buffer) == WriteStatus::WriteError)
    {
      return WriteStatus::WriteError;
    }
    progressThrottle.updateCount(grainId);
  }

  fmt::format_to(std::back_inserter(buffer), "**\n{}**\n", k_SeparatorLine);
  if(FlushBuffer(output, buffer) == WriteStatus::WriteError)
  {
    return WriteStatus::WriteError;
  }
  return CloseOutput(output);
}

void RemoveTemporaryFiles(const std::vector<Result<AtomicFile>>& files)
{
  for(const auto& file : files)
  {
    if(file.valid())
    {
      file.value().removeTempFile();
    }
  }
}
} // namespace

WriteAbaqusCrystalPlasticity::WriteAbaqusCrystalPlasticity(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                                           WriteAbaqusCrystalPlasticityInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(messageHandler)
{
}

WriteAbaqusCrystalPlasticity::~WriteAbaqusCrystalPlasticity() noexcept = default;

Result<> WriteAbaqusCrystalPlasticity::operator()()
{
  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  const auto& featureIdsRef = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath).getDataStoreRef();
  const auto& cellPhasesRef = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellPhasesArrayPath).getDataStoreRef();
  const auto& cellEulerAnglesRef = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CellEulerAnglesArrayPath).getDataStoreRef();

  if(featureIdsRef.getNumberOfTuples() == 0)
  {
    return MakeErrorResult(-12005, fmt::format("The feature IDs array '{}' is empty and has 0 tuples.", m_InputValues->FeatureIdsArrayPath.toString()));
  }

  auto maximumResult = AbaqusElementSets::findMaximumGrainId(featureIdsRef, m_ShouldCancel);
  if(maximumResult.invalid())
  {
    return ConvertResult(std::move(maximumResult));
  }
  if(m_ShouldCancel)
  {
    return {};
  }
  const int32 maxGrainId = maximumResult.value();
  if(maxGrainId <= 0)
  {
    return MakeErrorResult(-12011, fmt::format("The feature IDs array '{}' has no positive feature IDs. The Abaqus deck requires at least one grain.", m_InputValues->FeatureIdsArrayPath.toString()));
  }

  ThrottledMessageHandler progressThrottle(m_MessageHandler);
  const usize grainCount = static_cast<usize>(maxGrainId);
  const bool usesOutOfCoreStore = featureIdsRef.getStoreType() == IDataStore::StoreType::OutOfCore || cellPhasesRef.getStoreType() == IDataStore::StoreType::OutOfCore ||
                                  cellEulerAnglesRef.getStoreType() == IDataStore::StoreType::OutOfCore;
  const bool useOocAlgorithm = !ForceInCoreAlgorithm() && (usesOutOfCoreStore || ForceOocAlgorithm());
  RecordAlgorithmPathExecution(useOocAlgorithm ? AlgorithmPath::OutOfCore : AlgorithmPath::InCore, usesOutOfCoreStore);

  GrainData grainData;
  grainData.Phases.resize(grainCount + 1, 0);
  grainData.Orientations.resize(grainCount + 1, {0.0F, 0.0F, 0.0F});
  grainData.HasCells.resize(grainCount + 1, 0);
  const float64 angleScale = m_InputValues->WriteEulerAnglesInDegrees ? 180.0 / std::numbers::pi : 1.0;
  const usize cellCount = featureIdsRef.getNumberOfTuples();
  const usize chunkSize = std::min(AbaqusElementSets::k_ChunkSize, cellCount);
  std::vector<int32> featurePage(chunkSize);
  std::vector<int32> phasePage(chunkSize);
  std::vector<float32> eulerPage(chunkSize * 3);
  progressThrottle.reset(cellCount, "Reading Grain Data");
  for(usize offset = 0; offset < cellCount; offset += chunkSize)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    const usize count = std::min(chunkSize, cellCount - offset);
    if(auto result = featureIdsRef.copyIntoBuffer(offset, nonstd::span<int32>(featurePage.data(), count)); result.invalid())
    {
      return result;
    }
    if(auto result = cellPhasesRef.copyIntoBuffer(offset, nonstd::span<int32>(phasePage.data(), count)); result.invalid())
    {
      return result;
    }
    if(auto result = cellEulerAnglesRef.copyIntoBuffer(offset * 3, nonstd::span<float32>(eulerPage.data(), count * 3)); result.invalid())
    {
      return result;
    }
    // Linear cell order preserves the documented last-cell phase and orientation for each grain.
    for(usize index = 0; index < count; index++)
    {
      if(featurePage[index] > 0)
      {
        const usize grainId = static_cast<usize>(featurePage[index]);
        grainData.HasCells[grainId] = 1;
        grainData.Phases[grainId] = phasePage[index];
        for(usize component = 0; component < 3; component++)
        {
          grainData.Orientations[grainId][component] = static_cast<float32>(eulerPage[index * 3 + component] * angleScale);
        }
      }
    }
    progressThrottle.updateCount(offset + count);
  }
  const usize emptyGrainCount = static_cast<usize>(std::count(grainData.HasCells.cbegin() + 1, grainData.HasCells.cend(), 0));
  if(emptyGrainCount > 0)
  {
    m_MessageHandler.sendWarningMessage(
        fmt::format("{} feature ids in [1, {}] have no cells. Empty element sets and materials with zero orientation were written for them.", emptyGrainCount, maxGrainId));
  }

  const std::array<fs::path, 5> fileList = {
      m_InputValues->OutputPath / fmt::format("{}_nodes.inp", m_InputValues->FilePrefix), m_InputValues->OutputPath / fmt::format("{}_elems.inp", m_InputValues->FilePrefix),
      m_InputValues->OutputPath / fmt::format("{}_sects.inp", m_InputValues->FilePrefix), m_InputValues->OutputPath / fmt::format("{}_elset.inp", m_InputValues->FilePrefix),
      m_InputValues->OutputPath / fmt::format("{}.inp", m_InputValues->FilePrefix)};

  std::vector<Result<AtomicFile>> files;
  files.reserve(fileList.size());
  for(const auto& outputPath : fileList)
  {
    files.push_back(AtomicFile::Create(outputPath));
    if(files.back().invalid())
    {
      RemoveTemporaryFiles(files);
      return ConvertResult(std::move(files.back()));
    }
  }

  const auto writeFile = [&](FileIndex fileIndex, std::string_view progressMessage, auto&& writer) -> Result<> {
    if(m_ShouldCancel)
    {
      RemoveTemporaryFiles(files);
      return {};
    }

    m_MessageHandler.sendInfoMessage(std::string(progressMessage));
    const usize index = nx::core::to_underlying(fileIndex);
    const fs::path tempPath = files[index].value().tempFilePath();
    const WriteStatus status = writer(tempPath);
    if(status == WriteStatus::OpenError)
    {
      RemoveTemporaryFiles(files);
      return MakeErrorResult(-12012, fmt::format("Could not open '{}' for writing (target '{}').", tempPath.string(), fileList[index].string()));
    }
    if(status == WriteStatus::WriteError)
    {
      RemoveTemporaryFiles(files);
      return MakeErrorResult(-12013, fmt::format("Writing to '{}' failed (target '{}'). Check available disk space.", tempPath.string(), fileList[index].string()));
    }
    if(status == WriteStatus::Cancelled || m_ShouldCancel)
    {
      RemoveTemporaryFiles(files);
      return {};
    }
    return {};
  };

  Result<> writeResult = writeFile(FileIndex::Nodes, "Writing Nodes (File 1/5)...", [&](const fs::path& path) { return WriteNodes(path, imageGeom, m_ShouldCancel, progressThrottle); });
  if(writeResult.invalid())
  {
    return writeResult;
  }

  writeResult = writeFile(FileIndex::Elems, "Writing Elements (File 2/5)...",
                          [&](const fs::path& path) { return WriteElements(path, imageGeom, m_InputValues->UseReducedIntegration, m_ShouldCancel, progressThrottle); });
  if(writeResult.invalid())
  {
    return writeResult;
  }

  if(m_ShouldCancel)
  {
    RemoveTemporaryFiles(files);
    return {};
  }
  m_MessageHandler.sendInfoMessage("Writing Element Sets (File 3/5)...");
  writeResult = WriteElementSets(files[nx::core::to_underlying(FileIndex::Elset)].value().tempFilePath(), grainData, featureIdsRef, useOocAlgorithm, m_ShouldCancel, progressThrottle);
  if(writeResult.invalid())
  {
    RemoveTemporaryFiles(files);
    return writeResult;
  }

  writeResult = writeFile(FileIndex::Master, "Writing Master File (File 4/5)...", [&](const fs::path& path) { return WriteMaster(path, *m_InputValues, grainData, m_ShouldCancel, progressThrottle); });
  if(writeResult.invalid())
  {
    return writeResult;
  }

  writeResult = writeFile(FileIndex::Sects, "Writing Sections (File 5/5)...", [&](const fs::path& path) {
    return WriteSections(path, grainData, m_InputValues->UseReducedIntegration, m_InputValues->HourglassStiffness, m_ShouldCancel, progressThrottle);
  });
  if(writeResult.invalid())
  {
    return writeResult;
  }

  if(m_ShouldCancel)
  {
    RemoveTemporaryFiles(files);
    return {};
  }

  for(auto& file : files)
  {
    Result<> commitResult = file.value().commit();
    if(commitResult.invalid())
    {
      RemoveTemporaryFiles(files);
      return commitResult;
    }
  }

  return {};
}
