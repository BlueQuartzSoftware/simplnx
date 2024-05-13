#pragma once

#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5.hpp"
#include "simplnx/simplnx_export.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <utility>

namespace nx::core::HDF5
{
class FileReader;
class FileWriter;
} // namespace nx::core::HDF5

namespace nx::core
{
class DataStructure;

namespace DREAM3D
{
using FileData = std::pair<Pipeline, DataStructure>;
using FileVersionType = std::string;
using PipelineVersionType = int32;

inline constexpr int32 k_InvalidPipelineVersion = -404;
inline constexpr int32 k_InvalidDataStructureVersion = -405;
inline constexpr int32 k_PipelineGroupUnavailable = -406;
inline constexpr StringLiteral k_CurrentFileVersion = "8.0";
/**
 * @brief Returns the DREAM3D file version.
 * @param fileReader
 * @return FileVersionType
 */
SIMPLNX_EXPORT FileVersionType GetFileVersion(const nx::core::HDF5::FileReader& fileReader);

/**
 * @brief Returns the DREAM3D pipeline version.
 * @param fileReader
 * @return PipelineVersionType
 */
SIMPLNX_EXPORT PipelineVersionType GetPipelineVersion(const nx::core::HDF5::FileReader& fileReader);

/**
 * @brief Imports and returns the Pipeline / DataStructure pair from the target
 * .dream3d file.
 *
 * This method imports both current and legacy DataStructures but will return
 * an empty Pipeline when given a legacy file.
 * @param fileReader
 * @param preflight = false
 * @return FileData
 */
SIMPLNX_EXPORT Result<FileData> ReadFile(const nx::core::HDF5::FileReader& fileReader, bool preflight = false);

/**
 * @brief Imports and returns the Pipeline / DataStructure pair from the target
 * .dream3d file.
 *
 * This method imports both current and legacy DataStructures but will return
 * an empty Pipeline and warning when given a legacy file.
 * @param path
 * @return Result<FileData>
 */
SIMPLNX_EXPORT Result<FileData> ReadFile(const std::filesystem::path& path);

/**
 * @brief Writes a .dream3d file with the specified data.
 * @param fileWriter
 * @param fileData
 * @return Result<>
 */
SIMPLNX_EXPORT Result<> WriteFile(nx::core::HDF5::FileWriter& fileWriter, const FileData& fileData);

/**
 * @brief Writes a .dream3d file with the specified data.
 * @param fileWriter
 * @param fileData
 * @return Result<>
 */
SIMPLNX_EXPORT Result<> WriteFile(nx::core::HDF5::FileWriter& fileWriter, const Pipeline& pipeline, const DataStructure& dataStructure);

/**
 * @brief Writes a .dream3d file with the specified data.
 * @param path
 * @param dataStructure
 * @param writeXdmf
 * @return bool
 */
SIMPLNX_EXPORT Result<> WriteFile(const std::filesystem::path& path, const DataStructure& dataStructure, const Pipeline& pipeline = {}, bool writeXdmf = false);

/**
 * @brief Imports and returns the DataStructure from the target .dream3d file.
 *
 * This method imports both current and legacy DataStructures.
 * @param fileReader
 * @param preflight = false
 * @return DataStructure
 */
SIMPLNX_EXPORT Result<DataStructure> ImportDataStructureFromFile(const nx::core::HDF5::FileReader& fileReader, bool preflight = false);

/**
 * @brief Imports and returns the DataStructure from the target .dream3d file.
 * This method imports both current and legacy DataStructures.
 * @param filePath
 * @return DataStructure
 */
SIMPLNX_EXPORT Result<DataStructure> ImportDataStructureFromFile(const std::filesystem::path& filePath, bool preflight = false);

/**
 * @brief Imports and returns a Pipeline from the target .dream3d file.
 *
 * This method does not import legacy Pipelines.
 * @param fileReader
 * @return Pipeline
 */
SIMPLNX_EXPORT Result<Pipeline> ImportPipelineFromFile(const nx::core::HDF5::FileReader& fileReader);

/**
 * @brief Imports and returns a Pipeline from the target .dream3d file.
 *
 * This method does not import legacy Pipelines.
 * @param fileReader
 * @return Pipeline
 */
SIMPLNX_EXPORT Result<nlohmann::json> ImportPipelineJsonFromFile(const nx::core::HDF5::FileReader& fileReader);

/**
 * @brief Imports and returns a Pipeline from the target .dream3d file.
 * This method does not import legacy Pipelines.
 * @param fileReader
 * @return Pipeline
 */
SIMPLNX_EXPORT Result<nlohmann::json> ImportPipelineJsonFromFile(const std::filesystem::path& filePath);

/**
 * @brief Imports and returns a Pipeline from the target .dream3d file.
 * This method does not import legacy Pipelines.
 * @param fileReader
 * @return Pipeline
 */
SIMPLNX_EXPORT Result<Pipeline> ImportPipelineFromFile(const std::filesystem::path& filePath);

/**
 * @brief Writes an xdmf file for the given DataStructure.
 * The hdf5 file path corresponds to an already written hdf5 file for the given DataStructure.
 * @param filePath
 * @param dataStructure
 * @param hdf5FilePath
 * @return
 */
SIMPLNX_EXPORT void WriteXdmf(const std::filesystem::path& filePath, const DataStructure& dataStructure, std::string_view hdf5FilePath);
} // namespace DREAM3D
} // namespace nx::core
