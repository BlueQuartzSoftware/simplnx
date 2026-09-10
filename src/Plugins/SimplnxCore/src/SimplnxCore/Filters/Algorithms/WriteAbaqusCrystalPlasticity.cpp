#include "WriteAbaqusCrystalPlasticity.hpp"

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
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
  Error
};

struct GrainData
{
  std::vector<int32> phases;
  std::vector<std::array<float32, 3>> orientations;
  std::vector<std::vector<usize>> elementIds;
};

WriteStatus FinishWrite(std::ofstream& output)
{
  output.close();
  return output ? WriteStatus::Success : WriteStatus::Error;
}

WriteStatus WriteNodes(const fs::path& filePath, const ImageGeom& imageGeometry, const std::atomic_bool& shouldCancel)
{
  std::ofstream output(filePath, std::ios::binary);
  if(!output.is_open())
  {
    return WriteStatus::Error;
  }

  const auto dimensions = imageGeometry.getDimensions();
  const auto spacing = imageGeometry.getSpacing();
  const auto origin = imageGeometry.getOrigin();
  const std::array<usize, 3> nodeDimensions = {dimensions[0] + 1, dimensions[1] + 1, dimensions[2] + 1};

  output << "*NODE, NSET=ALLNODES\n";
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
        output << fmt::format("{}, {:.3f}, {:.3f}, {:.3f}\n", index + 1, xCoordinate, yCoordinate, zCoordinate);
      }
    }
  }

  return FinishWrite(output);
}

WriteStatus WriteElements(const fs::path& filePath, const ImageGeom& imageGeometry, const std::atomic_bool& shouldCancel)
{
  std::ofstream output(filePath, std::ios::binary);
  if(!output.is_open())
  {
    return WriteStatus::Error;
  }

  const auto dimensions = imageGeometry.getDimensions();
  const usize nodesX = dimensions[0] + 1;
  const usize nodesY = dimensions[1] + 1;

  output << "*ELEMENT, TYPE=C3D8R, ELSET=ALLELEMENTS\n";
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
        output << fmt::format("{}, {}, {}, {}, {}, {}, {}, {}, {}\n", elementIndex + 1, node1, node2, node3, node4, node5, node6, node7, node8);
      }
    }
  }

  return FinishWrite(output);
}

WriteStatus WriteElementSets(const fs::path& filePath, const GrainData& grainData, const std::atomic_bool& shouldCancel)
{
  std::ofstream output(filePath, std::ios::binary);
  if(!output.is_open())
  {
    return WriteStatus::Error;
  }

  for(usize grainId = 1; grainId < grainData.elementIds.size(); grainId++)
  {
    if(shouldCancel)
    {
      return WriteStatus::Cancelled;
    }

    output << fmt::format("*Elset, elset=Grain{}_Phase{}_set\n", grainId, grainData.phases[grainId]);
    const auto& elementIds = grainData.elementIds[grainId];
    for(usize index = 0; index < elementIds.size(); index++)
    {
      if(index != 0)
      {
        output << (index % 16 == 0 ? ",\n" : ", ");
      }
      output << elementIds[index];
    }
    output << '\n';
  }

  return FinishWrite(output);
}

void WriteMaterialConstants(std::ofstream& output, const DynamicTableParameter::ValueType& materialConstants)
{
  usize entriesPerLine = 5;
  for(const auto& row : materialConstants)
  {
    if(entriesPerLine != 0)
    {
      if(entriesPerLine % 8 != 0)
      {
        output << ", ";
      }
      else
      {
        output << '\n';
        entriesPerLine = 0;
      }
    }
    output << fmt::format("{:.3f}", row[0]);
    entriesPerLine++;
  }
}

WriteStatus WriteMaster(const fs::path& filePath, const WriteAbaqusCrystalPlasticityInputValues& inputValues, const GrainData& grainData, const std::atomic_bool& shouldCancel)
{
  std::ofstream output(filePath, std::ios::binary);
  if(!output.is_open())
  {
    return WriteStatus::Error;
  }

  output << "*Heading\n";
  output << inputValues.JobName << '\n';
  output << fmt::format("** Job name : {}\n", inputValues.JobName);
  output << "*Preprint, echo = NO, model = NO, history = NO, contact = NO\n";
  output << "**\n";
  output << fmt::format("*Include, Input = {}_nodes.inp\n", inputValues.FilePrefix);
  output << fmt::format("*Include, Input = {}_elems.inp\n", inputValues.FilePrefix);
  output << fmt::format("*Include, Input = {}_sects.inp\n", inputValues.FilePrefix);
  output << fmt::format("*Include, Input = {}_elset.inp\n", inputValues.FilePrefix);
  output << "**\n";

  const usize materialConstantCount = inputValues.MaterialConstants.size();
  for(usize grainId = 1; grainId < grainData.phases.size(); grainId++)
  {
    if(shouldCancel)
    {
      return WriteStatus::Cancelled;
    }

    const int32 phaseId = grainData.phases[grainId];
    const auto& orientation = grainData.orientations[grainId];
    output << fmt::format("*Material, name = Grain{}_Phase{}_mat\n", grainId, phaseId);
    output << "*Depvar\n";
    output << fmt::format("{}\n", inputValues.NumDepvar);
    output << fmt::format("*User Material, constants = {}\n", materialConstantCount + 5);
    output << fmt::format("{}, {}, {:.3f}, {:.3f}, {:.3f}", grainId, phaseId, orientation[0], orientation[1], orientation[2]);
    WriteMaterialConstants(output, inputValues.MaterialConstants);
    output << '\n';
    output << "*User Output Variables\n";
    output << fmt::format("{}\n", inputValues.NumUserOutVar);
  }

  return FinishWrite(output);
}

WriteStatus WriteSections(const fs::path& filePath, const GrainData& grainData, const std::atomic_bool& shouldCancel)
{
  std::ofstream output(filePath, std::ios::binary);
  if(!output.is_open())
  {
    return WriteStatus::Error;
  }

  for(usize grainId = 1; grainId < grainData.phases.size(); grainId++)
  {
    if(shouldCancel)
    {
      return WriteStatus::Cancelled;
    }
    const int32 phaseId = grainData.phases[grainId];
    output << fmt::format("*Solid Section, elset=Grain{}_Phase{}_set, material=Grain{}_Phase{}_mat\n", grainId, phaseId, grainId, phaseId);
  }

  return FinishWrite(output);
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
  const auto& imageGeometry = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath).getDataStoreRef();
  const auto& cellPhases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellPhasesArrayPath).getDataStoreRef();
  const auto& cellEulerAngles = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CellEulerAnglesArrayPath).getDataStoreRef();

  if(featureIds.getNumberOfTuples() == 0)
  {
    return MakeErrorResult(-12005, fmt::format("The feature IDs array '{}' is empty and has 0 tuples.", m_InputValues->FeatureIdsArrayPath.toString()));
  }

  const int32 maxGrainId = *std::max_element(featureIds.cbegin(), featureIds.cend());
  const usize grainCount = maxGrainId > 0 ? static_cast<usize>(maxGrainId) : 0;
  GrainData grainData;
  grainData.phases.resize(grainCount + 1, 0);
  grainData.orientations.resize(grainCount + 1, {0.0F, 0.0F, 0.0F});
  grainData.elementIds.resize(grainCount + 1);

  constexpr float64 k_RadiansToDegrees = 180.0 / std::numbers::pi;
  for(usize cellIndex = 0; cellIndex < featureIds.getNumberOfTuples(); cellIndex++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    const int32 featureId = featureIds[cellIndex];
    if(featureId > 0)
    {
      const usize grainId = static_cast<usize>(featureId);
      grainData.phases[grainId] = cellPhases[cellIndex];
      grainData.orientations[grainId][0] = static_cast<float32>(cellEulerAngles[cellIndex * 3] * k_RadiansToDegrees);
      grainData.orientations[grainId][1] = static_cast<float32>(cellEulerAngles[cellIndex * 3 + 1] * k_RadiansToDegrees);
      grainData.orientations[grainId][2] = static_cast<float32>(cellEulerAngles[cellIndex * 3 + 2] * k_RadiansToDegrees);
      grainData.elementIds[grainId].push_back(cellIndex + 1);
    }
  }

  const std::array<fs::path, 5> outputPaths = {
      m_InputValues->OutputPath / fmt::format("{}_nodes.inp", m_InputValues->FilePrefix), m_InputValues->OutputPath / fmt::format("{}_elems.inp", m_InputValues->FilePrefix),
      m_InputValues->OutputPath / fmt::format("{}_sects.inp", m_InputValues->FilePrefix), m_InputValues->OutputPath / fmt::format("{}_elset.inp", m_InputValues->FilePrefix),
      m_InputValues->OutputPath / fmt::format("{}.inp", m_InputValues->FilePrefix)};

  std::vector<Result<AtomicFile>> files;
  files.reserve(outputPaths.size());
  for(const auto& outputPath : outputPaths)
  {
    files.push_back(AtomicFile::Create(outputPath));
    if(files.back().invalid())
    {
      RemoveTemporaryFiles(files);
      return ConvertResult(std::move(files.back()));
    }
  }

  MessageHelper messageHelper(m_MessageHandler);
  const auto writeFile = [&](usize fileIndex, std::string_view progressMessage, int32 errorCode, auto&& writer) -> Result<> {
    if(m_ShouldCancel)
    {
      RemoveTemporaryFiles(files);
      return {};
    }

    messageHelper.sendMessage(std::string(progressMessage));
    const WriteStatus status = writer(files[fileIndex].value().tempFilePath());
    if(status == WriteStatus::Cancelled || m_ShouldCancel)
    {
      RemoveTemporaryFiles(files);
      return {};
    }
    if(status == WriteStatus::Error)
    {
      RemoveTemporaryFiles(files);
      return MakeErrorResult(errorCode,
                             fmt::format("Failed to write output file '{}'. The temporary file path is '{}'.", outputPaths[fileIndex].string(), files[fileIndex].value().tempFilePath().string()));
    }
    return {};
  };

  Result<> writeResult = writeFile(0, "Writing Nodes (File 1/5)...", -12006, [&](const fs::path& path) { return WriteNodes(path, imageGeometry, m_ShouldCancel); });
  if(writeResult.invalid())
  {
    return writeResult;
  }

  writeResult = writeFile(1, "Writing Elements (File 2/5)...", -12007, [&](const fs::path& path) { return WriteElements(path, imageGeometry, m_ShouldCancel); });
  if(writeResult.invalid())
  {
    return writeResult;
  }

  writeResult = writeFile(3, "Writing Element Sets (File 3/5)...", -12008, [&](const fs::path& path) { return WriteElementSets(path, grainData, m_ShouldCancel); });
  if(writeResult.invalid())
  {
    return writeResult;
  }

  writeResult = writeFile(4, "Writing Master File (File 4/5)...", -12009, [&](const fs::path& path) { return WriteMaster(path, *m_InputValues, grainData, m_ShouldCancel); });
  if(writeResult.invalid())
  {
    return writeResult;
  }

  writeResult = writeFile(2, "Writing Sections (File 5/5)...", -12010, [&](const fs::path& path) { return WriteSections(path, grainData, m_ShouldCancel); });
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
