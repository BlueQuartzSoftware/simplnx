#include "WriteAbaqusInputDeck.hpp"

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <numbers>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
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
  std::vector<usize> CellIndices;
  std::vector<usize> Offsets;
};

constexpr usize ToIndex(FileIndex fileIndex)
{
  return static_cast<usize>(fileIndex);
}

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

void SendProgress(ThrottledMessenger& messenger, std::string_view label, usize current, usize total)
{
  messenger.sendThrottledMessage([=]() { return fmt::format("{}: {:.0f}%", label, CalculatePercentComplete(current, total)); });
}

WriteStatus WriteNodes(const fs::path& filePath, const ImageGeom& imageGeom, bool writeDummyNode, const std::atomic_bool& shouldCancel, MessageHelper& messageHelper)
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
  fmt::format_to(std::back_inserter(buffer), "*NODE, NSET=ALLNODES\n");
  ThrottledMessenger progressMessenger = messageHelper.createThrottledMessenger();

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
        const float32 xCoordinate = origin[0] + static_cast<float32>(x) * spacing[0];
        const float32 yCoordinate = origin[1] + static_cast<float32>(y) * spacing[1];
        const float32 zCoordinate = origin[2] + static_cast<float32>(z) * spacing[2];
        fmt::format_to(std::back_inserter(buffer), "{}, {:.3f}, {:.3f}, {:.3f}\n", index + 1, xCoordinate, yCoordinate, zCoordinate);
      }
    }

    if(FlushBuffer(output, buffer) == WriteStatus::WriteError)
    {
      return WriteStatus::WriteError;
    }
    SendProgress(progressMessenger, "Writing Nodes (File 1/5)", z + 1, nodeDimensions[2]);
  }

  if(writeDummyNode)
  {
    const usize dummyNodeId = nodeDimensions[0] * nodeDimensions[1] * nodeDimensions[2] + 1;
    fmt::format_to(std::back_inserter(buffer), "{}, {:.3f}, {:.3f}, {:.3f}\n", dummyNodeId, 0.0F, 0.0F, 0.0F);
    if(FlushBuffer(output, buffer) == WriteStatus::WriteError)
    {
      return WriteStatus::WriteError;
    }
  }

  return CloseOutput(output);
}

WriteStatus WriteElements(const fs::path& filePath, const ImageGeom& imageGeom, bool useReducedIntegration, const std::atomic_bool& shouldCancel, MessageHelper& messageHelper)
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
  fmt::format_to(std::back_inserter(buffer), "*ELEMENT, TYPE={}, ELSET=ALLELEMENTS\n", useReducedIntegration ? "C3D8R" : "C3D8");
  ThrottledMessenger progressMessenger = messageHelper.createThrottledMessenger();

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
      }
    }

    if(FlushBuffer(output, buffer) == WriteStatus::WriteError)
    {
      return WriteStatus::WriteError;
    }
    SendProgress(progressMessenger, "Writing Elements (File 2/5)", z + 1, dimensions[2]);
  }

  return CloseOutput(output);
}

WriteStatus WriteElementSets(const fs::path& filePath, const GrainData& grainData, const std::atomic_bool& shouldCancel, MessageHelper& messageHelper)
{
  std::ofstream output(filePath, std::ios::binary);
  if(!output.is_open())
  {
    return WriteStatus::OpenError;
  }

  const usize grainCount = grainData.Phases.size() - 1;
  fmt::memory_buffer buffer;
  ThrottledMessenger progressMessenger = messageHelper.createThrottledMessenger();
  for(usize grainId = 1; grainId <= grainCount; grainId++)
  {
    if(shouldCancel)
    {
      return WriteStatus::Cancelled;
    }

    fmt::format_to(std::back_inserter(buffer), "*Elset, elset=Grain{}_Phase{}_set\n", grainId, grainData.Phases[grainId]);
    const usize beginOffset = grainData.Offsets[grainId];
    const usize endOffset = grainData.Offsets[grainId + 1];
    for(usize index = beginOffset; index < endOffset; index++)
    {
      const usize grainCellIndex = index - beginOffset;
      if(grainCellIndex != 0)
      {
        fmt::format_to(std::back_inserter(buffer), "{}", grainCellIndex % 16 == 0 ? ",\n" : ", ");
      }
      fmt::format_to(std::back_inserter(buffer), "{}", grainData.CellIndices[index]);
    }
    fmt::format_to(std::back_inserter(buffer), "\n");

    if(FlushBuffer(output, buffer) == WriteStatus::WriteError)
    {
      return WriteStatus::WriteError;
    }
    SendProgress(progressMessenger, "Writing Element Sets (File 3/5)", grainId, grainCount);
  }

  return CloseOutput(output);
}

void WriteMaterialConstants(fmt::memory_buffer& buffer, const DynamicTableParameter::ValueType& materialConstants)
{
  usize entriesPerLine = 5;
  for(const auto& row : materialConstants)
  {
    if(entriesPerLine != 0)
    {
      if(entriesPerLine % 8 != 0)
      {
        fmt::format_to(std::back_inserter(buffer), ", ");
      }
      else
      {
        fmt::format_to(std::back_inserter(buffer), "\n");
        entriesPerLine = 0;
      }
    }
    fmt::format_to(std::back_inserter(buffer), "{:.3f}", row.empty() ? 0.0 : row.front());
    entriesPerLine++;
  }
}

WriteStatus WriteMaster(const fs::path& filePath, const WriteAbaqusInputDeckInputValues& inputValues, const GrainData& grainData, const std::atomic_bool& shouldCancel, MessageHelper& messageHelper)
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
  fmt::format_to(std::back_inserter(buffer), "**\n");
  fmt::format_to(std::back_inserter(buffer), "*Include, Input = {}_nodes.inp\n", inputValues.FilePrefix);
  fmt::format_to(std::back_inserter(buffer), "*Include, Input = {}_elems.inp\n", inputValues.FilePrefix);
  fmt::format_to(std::back_inserter(buffer), "*Include, Input = {}_sects.inp\n", inputValues.FilePrefix);
  fmt::format_to(std::back_inserter(buffer), "*Include, Input = {}_elset.inp\n", inputValues.FilePrefix);
  fmt::format_to(std::back_inserter(buffer), "**\n");

  if(!inputValues.CrystalPlasticityMaterial.has_value())
  {
    if(FlushBuffer(output, buffer) == WriteStatus::WriteError)
    {
      return WriteStatus::WriteError;
    }
    return CloseOutput(output);
  }

  const auto& materialValues = inputValues.CrystalPlasticityMaterial.value();
  const usize materialConstantCount = materialValues.MaterialConstants.size();
  const usize grainCount = grainData.Phases.size() - 1;
  ThrottledMessenger progressMessenger = messageHelper.createThrottledMessenger();
  for(usize grainId = 1; grainId <= grainCount; grainId++)
  {
    if(shouldCancel)
    {
      return WriteStatus::Cancelled;
    }

    const int32 phaseId = grainData.Phases[grainId];
    const auto& orientation = grainData.Orientations[grainId];
    fmt::format_to(std::back_inserter(buffer), "*Material, name = Grain{}_Phase{}_mat\n", grainId, phaseId);
    fmt::format_to(std::back_inserter(buffer), "*Depvar\n");
    fmt::format_to(std::back_inserter(buffer), "{}\n", materialValues.NumDepvar);
    fmt::format_to(std::back_inserter(buffer), "*User Material, constants = {}\n", materialConstantCount + 5);
    fmt::format_to(std::back_inserter(buffer), "{}, {}, {:.3f}, {:.3f}, {:.3f}", grainId, phaseId, orientation[0], orientation[1], orientation[2]);
    WriteMaterialConstants(buffer, materialValues.MaterialConstants);
    fmt::format_to(std::back_inserter(buffer), "\n");
    fmt::format_to(std::back_inserter(buffer), "*User Output Variables\n");
    fmt::format_to(std::back_inserter(buffer), "{}\n", materialValues.NumUserOutVar);

    if(FlushBuffer(output, buffer) == WriteStatus::WriteError)
    {
      return WriteStatus::WriteError;
    }
    SendProgress(progressMessenger, "Writing Master File (File 4/5)", grainId, grainCount);
  }

  return CloseOutput(output);
}

WriteStatus WriteSections(const fs::path& filePath, const GrainData& grainData, bool useReducedIntegration, int32 hourglassStiffness, const std::atomic_bool& shouldCancel,
                          MessageHelper& messageHelper)
{
  std::ofstream output(filePath, std::ios::binary);
  if(!output.is_open())
  {
    return WriteStatus::OpenError;
  }

  const usize grainCount = grainData.Phases.size() - 1;
  fmt::memory_buffer buffer;
  ThrottledMessenger progressMessenger = messageHelper.createThrottledMessenger();
  for(usize grainId = 1; grainId <= grainCount; grainId++)
  {
    if(shouldCancel)
    {
      return WriteStatus::Cancelled;
    }

    const int32 phaseId = grainData.Phases[grainId];
    fmt::format_to(std::back_inserter(buffer), "*Solid Section, elset=Grain{}_Phase{}_set, material=Grain{}_Phase{}_mat\n", grainId, phaseId, grainId, phaseId);
    if(useReducedIntegration)
    {
      fmt::format_to(std::back_inserter(buffer), "*Hourglass Stiffness\n{}\n", hourglassStiffness);
    }
    if(FlushBuffer(output, buffer) == WriteStatus::WriteError)
    {
      return WriteStatus::WriteError;
    }
    SendProgress(progressMessenger, "Writing Sections (File 5/5)", grainId, grainCount);
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

Result<> nx::core::ValidateAbaqusInputCellArrays(const DataStructure& dataStructure, const DataPath& imageGeometryPath, const DataPath& featureIdsArrayPath, const DataPath& cellPhasesArrayPath)
{
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeometryPath);
  const usize cellCount = imageGeom.getNumberOfCells();
  const std::array<std::pair<DataPath, int32>, 2> arrayPaths = {{{featureIdsArrayPath, -12002}, {cellPhasesArrayPath, -12004}}};
  for(const auto& [arrayPath, errorCode] : arrayPaths)
  {
    const auto& array = dataStructure.getDataRefAs<IDataArray>(arrayPath);
    if(array.getNumberOfTuples() != cellCount)
    {
      return MakeErrorResult(
          errorCode, fmt::format("The array '{}' has {} tuples, but the Image Geometry '{}' has {} cells.", arrayPath.toString(), array.getNumberOfTuples(), imageGeometryPath.toString(), cellCount));
    }
  }
  return {};
}

WriteAbaqusInputDeck::WriteAbaqusInputDeck(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                           WriteAbaqusInputDeckInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(messageHandler)
{
}

WriteAbaqusInputDeck::~WriteAbaqusInputDeck() noexcept = default;

Result<> WriteAbaqusInputDeck::operator()()
{
  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  const auto& featureIdsRef = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath).getDataStoreRef();
  const auto& cellPhasesRef = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellPhasesArrayPath).getDataStoreRef();
  const Float32AbstractDataStore* cellEulerAngles = nullptr;
  if(m_InputValues->CrystalPlasticityMaterial.has_value())
  {
    cellEulerAngles = &m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CrystalPlasticityMaterial->CellEulerAnglesArrayPath).getDataStoreRef();
  }

  if(featureIdsRef.getNumberOfTuples() == 0)
  {
    return MakeErrorResult(-12005, fmt::format("The feature IDs array '{}' is empty and has 0 tuples.", m_InputValues->FeatureIdsArrayPath.toString()));
  }

  const int32 maxGrainId = *std::max_element(featureIdsRef.cbegin(), featureIdsRef.cend());
  if(maxGrainId <= 0)
  {
    return MakeErrorResult(-12011, fmt::format("The feature IDs array '{}' has no positive feature IDs. The Abaqus deck requires at least one grain.", m_InputValues->FeatureIdsArrayPath.toString()));
  }

  MessageHelper messageHelper(m_MessageHandler);
  const usize grainCount = static_cast<usize>(maxGrainId);
  const auto dimensions = imageGeom.getDimensions();
  const usize cellsPerSlice = dimensions[0] * dimensions[1];
  GrainData grainData;
  grainData.Phases.resize(grainCount + 1, 0);
  if(cellEulerAngles != nullptr)
  {
    grainData.Orientations.resize(grainCount + 1, {0.0F, 0.0F, 0.0F});
  }

  // The contiguous buckets use 8 bytes per positive cell; counts and offsets add 16 bytes per grain.
  std::vector<usize> counts(grainCount + 1, 0);
  constexpr float64 k_RadiansToDegrees = 180.0 / std::numbers::pi;
  ThrottledMessenger countMessenger = messageHelper.createThrottledMessenger();
  for(usize z = 0; z < dimensions[2]; z++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    const usize sliceStart = z * cellsPerSlice;
    const usize sliceEnd = sliceStart + cellsPerSlice;
    for(usize cellIndex = sliceStart; cellIndex < sliceEnd; cellIndex++)
    {
      const int32 featureId = featureIdsRef[cellIndex];
      if(featureId > 0)
      {
        const usize grainId = static_cast<usize>(featureId);
        counts[grainId]++;
        grainData.Phases[grainId] = cellPhasesRef[cellIndex];
        if(cellEulerAngles != nullptr)
        {
          grainData.Orientations[grainId][0] = static_cast<float32>((*cellEulerAngles)[cellIndex * 3] * k_RadiansToDegrees);
          grainData.Orientations[grainId][1] = static_cast<float32>((*cellEulerAngles)[cellIndex * 3 + 1] * k_RadiansToDegrees);
          grainData.Orientations[grainId][2] = static_cast<float32>((*cellEulerAngles)[cellIndex * 3 + 2] * k_RadiansToDegrees);
        }
      }
    }
    SendProgress(countMessenger, "Bucketing Elements (Pass 1/2)", z + 1, dimensions[2]);
  }

  const usize emptyGrainCount = static_cast<usize>(std::count(counts.cbegin() + 1, counts.cend(), 0));
  if(emptyGrainCount > 0)
  {
    if(m_InputValues->CrystalPlasticityMaterial.has_value())
    {
      m_MessageHandler(IFilter::Message::Type::Warning,
                       fmt::format("{} feature ids in [1, {}] have no cells. Empty element sets and materials with zero orientation were written for them.", emptyGrainCount, maxGrainId));
    }
    else
    {
      m_MessageHandler(IFilter::Message::Type::Warning,
                       fmt::format("{} feature ids in [1, {}] have no cells. Empty element sets and phase-zero sections were written for them.", emptyGrainCount, maxGrainId));
    }
  }

  grainData.Offsets.resize(grainCount + 2, 0);
  for(usize grainId = 1; grainId <= grainCount; grainId++)
  {
    grainData.Offsets[grainId + 1] = grainData.Offsets[grainId] + counts[grainId];
    counts[grainId] = grainData.Offsets[grainId];
  }
  grainData.CellIndices.resize(grainData.Offsets.back());

  ThrottledMessenger fillMessenger = messageHelper.createThrottledMessenger();
  for(usize z = 0; z < dimensions[2]; z++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    const usize sliceStart = z * cellsPerSlice;
    const usize sliceEnd = sliceStart + cellsPerSlice;
    for(usize cellIndex = sliceStart; cellIndex < sliceEnd; cellIndex++)
    {
      const int32 featureId = featureIdsRef[cellIndex];
      if(featureId > 0)
      {
        const usize grainId = static_cast<usize>(featureId);
        grainData.CellIndices[counts[grainId]] = cellIndex + 1;
        counts[grainId]++;
      }
    }
    SendProgress(fillMessenger, "Bucketing Elements (Pass 2/2)", z + 1, dimensions[2]);
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

    messageHelper.sendMessage(std::string(progressMessage));
    const usize index = ToIndex(fileIndex);
    const fs::path tempPath = files[index].value().tempFilePath();
    const WriteStatus status = writer(tempPath);
    if(status == WriteStatus::Cancelled || m_ShouldCancel)
    {
      RemoveTemporaryFiles(files);
      return {};
    }
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
    return {};
  };

  Result<> writeResult =
      writeFile(FileIndex::Nodes, "Writing Nodes (File 1/5)...", [&](const fs::path& path) { return WriteNodes(path, imageGeom, m_InputValues->WriteDummyNode, m_ShouldCancel, messageHelper); });
  if(writeResult.invalid())
  {
    return writeResult;
  }

  writeResult = writeFile(FileIndex::Elems, "Writing Elements (File 2/5)...",
                          [&](const fs::path& path) { return WriteElements(path, imageGeom, m_InputValues->UseReducedIntegration, m_ShouldCancel, messageHelper); });
  if(writeResult.invalid())
  {
    return writeResult;
  }

  writeResult = writeFile(FileIndex::Elset, "Writing Element Sets (File 3/5)...", [&](const fs::path& path) { return WriteElementSets(path, grainData, m_ShouldCancel, messageHelper); });
  if(writeResult.invalid())
  {
    return writeResult;
  }

  writeResult = writeFile(FileIndex::Master, "Writing Master File (File 4/5)...", [&](const fs::path& path) { return WriteMaster(path, *m_InputValues, grainData, m_ShouldCancel, messageHelper); });
  if(writeResult.invalid())
  {
    return writeResult;
  }

  writeResult = writeFile(FileIndex::Sects, "Writing Sections (File 5/5)...",
                          [&](const fs::path& path) { return WriteSections(path, grainData, m_InputValues->UseReducedIntegration, m_InputValues->HourglassStiffness, m_ShouldCancel, messageHelper); });
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
