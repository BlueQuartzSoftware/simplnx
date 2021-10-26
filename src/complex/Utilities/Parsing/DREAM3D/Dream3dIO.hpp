#pragma once

#include <memory>
#include <string>
#include <utility>

#include "complex/Utilities/Parsing/HDF5/H5.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
class DataStructure;
class Pipeline;

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
 * .dream3d file. If an error occurs while reading the file, the provided error
 * code is updated accordingly.
 *
 * This method imports both current and legacy DataStructures but will return
 * an empty Pipeline when given a legacy file.
 * @param fileReader
 * @param errorType
 * @return FileData
 */
COMPLEX_EXPORT FileData ReadFile(const H5::FileReader& fileReader, H5::ErrorType& errorType);

/**
 * @brief Writes a .dream3d file with the specified data.
 * @param fileWriter
 * @param fileData
 * @return H5::ErrorType
 */
COMPLEX_EXPORT H5::ErrorType WriteFile(H5::FileWriter& fileWriter, const FileData& fileData);

/**
 * @brief Imports and returns the DataStructure from the target .dream3d file.
 * If an error occurs while reading the file, the provided error code is updated
 * accordingly.
 *
 * This method imports both current and legacy DataStructures.
 * @param fileReader
 * @param errorCode
 * @return complex::DataStructure
 */
COMPLEX_EXPORT complex::DataStructure ImportDataStructureFromFile(const H5::FileReader& fileReader, H5::ErrorType& errorCode);

/**
 * @brief Imports and returns a Pipeline from the target .dream3d file.
 * If an error occurs while reading the file, the provided error code is updated accordingly.
 *
 * This method does not import legacy Pipelines.
 * @param fileReader
 * @param errorCode
 * @return complex::Pipeline
 */
COMPLEX_EXPORT complex::Pipeline ImportPipelineFromFile(const H5::FileReader& fileReader, H5::ErrorType& errorCode);
} // namespace DREAM3D
} // namespace complex
