#include "Dream3dIO.hpp"

#include "nlohmann/json.hpp"

#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Pipeline/Pipeline.hpp"
#include "complex/Utilities/Parsing/HDF5/H5DataStructureReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5DataStructureWriter.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileWriter.hpp"

using namespace complex;

namespace
{
constexpr complex::StringLiteral k_DataStructureGroupTag = "DataStructure";
constexpr complex::StringLiteral k_LegacyDataStructureGroupTag = "DataContainers";
constexpr complex::StringLiteral k_FileVersionTag = "FileVersion";
constexpr complex::StringLiteral k_PipelineJsonTag = "Pipeline";
constexpr complex::StringLiteral k_PipelineNameTag = "Current Pipeline";
constexpr complex::StringLiteral k_PipelineVersionTag = "Pipeline Version";
constexpr complex::StringLiteral k_CurrentFileVersion = "8.0";
constexpr int32_t k_CurrentPipelineVersion = 3;
} // namespace

complex::DREAM3D::FileVersionType complex::DREAM3D::GetFileVersion(const H5::FileReader& fileReader)
{
  auto fileVersionAttribute = fileReader.getAttribute(k_FileVersionTag);
  if(!fileVersionAttribute.isValid())
  {
    return "";
  }
  return fileVersionAttribute.readAsString();
}

complex::DREAM3D::PipelineVersionType complex::DREAM3D::GetPipelineVersion(const H5::FileReader& fileReader)
{
  auto pipelineGroup = fileReader.openGroup(k_PipelineJsonTag);
  auto pipelineVersionAttribute = pipelineGroup.getAttribute(k_PipelineVersionTag);
  if(!pipelineVersionAttribute.isValid())
  {
    return 0;
  }
  return pipelineVersionAttribute.readAsValue<PipelineVersionType>();
}

DataStructure ImportDataStructureV8(const H5::FileReader& fileReader, H5::ErrorType& errorCode)
{
  auto dataStructureGroup = fileReader.openGroup(k_DataStructureGroupTag);
  complex::H5::DataStructureReader dataStructureReader;
  auto dataStructure = dataStructureReader.readH5Group(dataStructureGroup, errorCode);
  if(errorCode < 0)
  {
    return {};
  }
  return dataStructure;
}

DataStructure ImportLegacyDataStructure(const H5::FileReader& fileReader, H5::ErrorType& errorCode)
{
  //auto dataStructureGroup = fileReader.openGroup(k_LegacyDataStructureGroupTag);

  throw std::runtime_error("Not implemented: ImportLegacyDataStructure from dream3d file");
}

complex::DataStructure complex::DREAM3D::ImportDataStructureFromFile(const H5::FileReader& fileReader, H5::ErrorType& errorCode)
{
  errorCode = 0;

  const auto fileVersion = GetFileVersion(fileReader);
  if(fileVersion == k_CurrentFileVersion)
  {
    return ImportDataStructureV8(fileReader, errorCode);
  }
  else
  {
    return ImportLegacyDataStructure(fileReader, errorCode);
  }
}

complex::Pipeline complex::DREAM3D::ImportPipelineFromFile(const H5::FileReader& fileReader, H5::ErrorType& errorCode)
{
  errorCode = 0;

  if(GetPipelineVersion(fileReader) != k_CurrentPipelineVersion)
  {
    return {};
  }

  auto pipelineGroupReader = fileReader.openGroup(k_PipelineJsonTag);
  auto pipelineDatasetReader = pipelineGroupReader.openDataset(k_PipelineJsonTag);
  if(!pipelineDatasetReader.isValid())
  {
    return {};
  }

  auto pipelineJsonString = pipelineDatasetReader.readAsString();
  auto pipelineJson = nlohmann::json::parse(pipelineJsonString);
  return Pipeline::FromJson(pipelineJson);
}

complex::DREAM3D::FileData complex::DREAM3D::ReadFile(const H5::FileReader& fileReader, H5::ErrorType& errorCode)
{
  errorCode = 0;
  //Pipeline pipeline;
  auto pipeline = ImportPipelineFromFile(fileReader, errorCode);
  if(errorCode < 0)
  {
    return {};
  }

  auto dataStructure = ImportDataStructureFromFile(fileReader, errorCode);
  if(errorCode < 0)
  {
    return {};
  }

  return {pipeline, dataStructure};
}

H5::ErrorType WritePipeline(H5::FileWriter& fileWriter, const complex::Pipeline& pipeline)
{
  if(!fileWriter.isValid())
  {
    return -1;
  }
  auto pipelineGroupWriter = fileWriter.createGroupWriter(k_PipelineJsonTag);
  auto versionAttribute = pipelineGroupWriter.createAttribute(k_PipelineVersionTag);
  auto errorCode = versionAttribute.writeValue<DREAM3D::PipelineVersionType>(k_CurrentPipelineVersion);
  if(errorCode < 0)
  {
    return errorCode;
  }

  auto nameAttribute = pipelineGroupWriter.createAttribute(k_PipelineNameTag);
  errorCode = nameAttribute.writeString(pipeline.getName());
  if(errorCode < 0)
  {
    return errorCode;
  }

  auto pipelineDatasetWriter = pipelineGroupWriter.createDatasetWriter(k_PipelineJsonTag);
  std::string pipelineString = pipeline.toJson().dump();
  return pipelineDatasetWriter.writeString(pipelineString);
}

H5::ErrorType WriteDataStructure(H5::FileWriter& fileWriter, const complex::DataStructure& dataStructure)
{
  auto dataStructureGroup = fileWriter.createGroupWriter(k_DataStructureGroupTag);
  return dataStructure.writeHdf5(dataStructureGroup);
}

H5::ErrorType WriteFileVersion(H5::FileWriter& fileWriter)
{
  auto fileVersionAttribute = fileWriter.createAttribute(k_FileVersionTag);
  return fileVersionAttribute.writeString(k_CurrentFileVersion);
}

H5::ErrorType complex::DREAM3D::WriteFile(H5::FileWriter& fileWriter, const FileData& fileData)
{
  auto errorCode = WriteFileVersion(fileWriter);
  if(errorCode < 0)
  {
    return errorCode;
  }

  errorCode = WritePipeline(fileWriter, fileData.first);
  if(errorCode < 0)
  {
    return errorCode;
  }
  return WriteDataStructure(fileWriter, fileData.second);
}
