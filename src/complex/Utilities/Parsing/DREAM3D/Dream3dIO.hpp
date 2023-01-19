#pragma once

#include "complex/Pipeline/Pipeline.hpp"
#include "complex/Utilities/Parsing/HDF5/H5.hpp"
#include "complex/complex_export.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <utility>

namespace complex
{
class DataStructure;

namespace H5
{
class FileReader;
class FileWriter;
} // namespace H5

namespace DREAM3D
{
using FileData = std::pair<complex::Pipeline, complex::DataStructure>;
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
COMPLEX_EXPORT FileVersionType GetFileVersion(const H5::FileReader& fileReader);

/**
 * @brief Returns the DREAM3D pipeline version.
 * @param fileReader
 * @return PipelineVersionType
 */
COMPLEX_EXPORT PipelineVersionType GetPipelineVersion(const H5::FileReader& fileReader);

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
COMPLEX_EXPORT Result<FileData> ReadFile(const H5::FileReader& fileReader, bool preflight = false);

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
 * @return H5::ErrorType
 */
COMPLEX_EXPORT H5::ErrorType WriteFile(H5::FileWriter& fileWriter, const FileData& fileData);

/**
 * @brief Writes a .dream3d file with the specified data.
 * @param fileWriter
 * @param fileData
 * @return H5::ErrorType
 */
COMPLEX_EXPORT H5::ErrorType WriteFile(H5::FileWriter& fileWriter, const Pipeline& pipeline, const DataStructure& dataStructure);

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
 * @return complex::DataStructure
 */
COMPLEX_EXPORT Result<complex::DataStructure> ImportDataStructureFromFile(const H5::FileReader& fileReader, bool preflight = false);

/**
 * @brief Imports and returns the DataStructure from the target .dream3d file.
 * This method imports both current and legacy DataStructures.
 * @param filePath
 * @return complex::DataStructure
 */
COMPLEX_EXPORT Result<complex::DataStructure> ImportDataStructureFromFile(const std::filesystem::path& filePath);

/**
 * @brief Imports and returns a Pipeline from the target .dream3d file.
 *
 * This method does not import legacy Pipelines.
 * @param fileReader
 * @return complex::Pipeline
 */
COMPLEX_EXPORT Result<complex::Pipeline> ImportPipelineFromFile(const H5::FileReader& fileReader);

/**
 * @brief Imports and returns a Pipeline from the target .dream3d file.
 * This method does not import legacy Pipelines.
 * @param fileReader
 * @return complex::Pipeline
 */
COMPLEX_EXPORT Result<complex::Pipeline> ImportPipelineFromFile(const std::filesystem::path& filePath);

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
