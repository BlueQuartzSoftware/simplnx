#pragma once

#include "complex/Pipeline/Pipeline.hpp"
#include "complex/Utilities/Parsing/HDF5/H5.hpp"
#include "complex/complex_export.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <utility>

namespace complex::HDF5
{
class FileReader;
class FileWriter;
} // namespace complex::HDF5

namespace complex
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

/**
 * @brief Returns the DREAM3D file version.
 * @param fileReader
 * @return FileVersionType
 */
COMPLEX_EXPORT FileVersionType GetFileVersion(const complex::HDF5::FileReader& fileReader);

/**
 * @brief Returns the DREAM3D pipeline version.
 * @param fileReader
 * @return PipelineVersionType
 */
COMPLEX_EXPORT PipelineVersionType GetPipelineVersion(const complex::HDF5::FileReader& fileReader);

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
COMPLEX_EXPORT Result<FileData> ReadFile(const complex::HDF5::FileReader& fileReader, bool preflight = false);

/**
 * @brief Imports and returns the Pipeline / DataStructure pair from the target
 * .dream3d file.
 *
 * This method imports both current and legacy DataStructures but will return
 * an empty Pipeline and warning when given a legacy file.
 * @param path
 * @return Result<FileData>
 */
COMPLEX_EXPORT Result<FileData> ReadFile(const std::filesystem::path& path);

/**
 * @brief Writes a .dream3d file with the specified data.
 * @param fileWriter
 * @param fileData
 * @return complex::HDF5::ErrorType
 */
COMPLEX_EXPORT complex::HDF5::ErrorType WriteFile(complex::HDF5::FileWriter& fileWriter, const FileData& fileData);

/**
 * @brief Writes a .dream3d file with the specified data.
 * @param fileWriter
 * @param fileData
 * @return complex::HDF5::ErrorType
 */
COMPLEX_EXPORT complex::HDF5::ErrorType WriteFile(complex::HDF5::FileWriter& fileWriter, const Pipeline& pipeline, const DataStructure& dataStructure);

/**
 * @brief Writes a .dream3d file with the specified data.
 * @param path
 * @param dataStructure
 * @param writeXdmf
 * @return bool
 */
COMPLEX_EXPORT Result<> WriteFile(const std::filesystem::path& path, const DataStructure& dataStructure, const Pipeline& pipeline = {}, bool writeXdmf = false);

/**
 * @brief Imports and returns the DataStructure from the target .dream3d file.
 *
 * This method imports both current and legacy DataStructures.
 * @param fileReader
 * @param preflight = false
 * @return DataStructure
 */
COMPLEX_EXPORT Result<DataStructure> ImportDataStructureFromFile(const complex::HDF5::FileReader& fileReader, bool preflight = false);

/**
 * @brief Imports and returns the DataStructure from the target .dream3d file.
 * This method imports both current and legacy DataStructures.
 * @param filePath
 * @return DataStructure
 */
COMPLEX_EXPORT Result<DataStructure> ImportDataStructureFromFile(const std::filesystem::path& filePath);

/**
 * @brief Imports and returns a Pipeline from the target .dream3d file.
 *
 * This method does not import legacy Pipelines.
 * @param fileReader
 * @return Pipeline
 */
COMPLEX_EXPORT Result<Pipeline> ImportPipelineFromFile(const complex::HDF5::FileReader& fileReader);

/**
 * @brief Imports and returns a Pipeline from the target .dream3d file.
 * This method does not import legacy Pipelines.
 * @param fileReader
 * @return Pipeline
 */
COMPLEX_EXPORT Result<Pipeline> ImportPipelineFromFile(const std::filesystem::path& filePath);

/**
 * @brief Writes an xdmf file for the given DataStructure.
 * The hdf5 file path corresponds to an already written hdf5 file for the given DataStructure.
 * @param filePath
 * @param dataStructure
 * @param hdf5FilePath
 * @return
 */
COMPLEX_EXPORT void WriteXdmf(const std::filesystem::path& filePath, const DataStructure& dataStructure, std::string_view hdf5FilePath);
} // namespace DREAM3D
} // namespace complex
