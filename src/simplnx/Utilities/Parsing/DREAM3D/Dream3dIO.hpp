#pragma once

#include "simplnx/DataStructure/IO/HDF5/DataStructureWriter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5.hpp"
#include "simplnx/simplnx_export.hpp"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace nx::core::HDF5
{
class FileIO;
} // namespace nx::core::HDF5

namespace nx::core
{
class DataStructure;
class IDataStoreFormatResolver;

/**
 * @namespace DREAM3D
 * @brief Reads, writes, and incrementally materializes DREAM3D files.
 */
namespace DREAM3D
{
/**
 * @typedef FileData
 * @brief Defines a Pipeline and DataStructure pair.
 */
using FileData = std::pair<Pipeline, DataStructure>;
/**
 * @typedef FileVersionType
 * @brief Defines the stored file-version text type.
 */
using FileVersionType = std::string;
/**
 * @typedef PipelineVersionType
 * @brief Defines the stored pipeline-version integer type.
 */
using PipelineVersionType = int32;

/**
 * @brief Identifies an invalid pipeline version.
 */
inline constexpr int32 k_InvalidPipelineVersion = -404;
/**
 * @brief Identifies an invalid DataStructure version.
 */
inline constexpr int32 k_InvalidDataStructureVersion = -405;
/**
 * @brief Identifies a missing pipeline group.
 */
inline constexpr int32 k_PipelineGroupUnavailable = -406;
/**
 * @brief Specifies the current DREAM3D file version.
 */
inline constexpr StringLiteral k_CurrentFileVersion = "8.0";
/**
 * @brief Specifies the supported legacy DREAM3D file version.
 */
inline constexpr StringLiteral k_LegacyFileVersion = "7.0";

/**
 * @brief Returns the DREAM3D file version.
 * @param path Identifies the file.
 * @return Stored file version or first read-error message.
 */
SIMPLNX_EXPORT FileVersionType GetFileVersion(const std::filesystem::path& path);

/**
 * @brief Returns the DREAM3D file version.
 * @param fileReader Provides an open file.
 * @return Stored file version or first read-error message.
 */
SIMPLNX_EXPORT FileVersionType GetFileVersion(const nx::core::HDF5::FileIO& fileReader);

/**
 * @brief Returns the DREAM3D pipeline version.
 * @param fileReader Provides an open file.
 * @return Stored pipeline version, or k_InvalidPipelineVersion on failure.
 */
SIMPLNX_EXPORT PipelineVersionType GetPipelineVersion(const nx::core::HDF5::FileIO& fileReader);

/**
 * @brief Imports a Pipeline and DataStructure from an open DREAM3D file.
 *
 * Legacy files return an empty Pipeline.
 * @param fileReader Provides an open file.
 * @param preflight Creates placeholder array stores when true.
 * @return Imported pair or read error.
 */
SIMPLNX_EXPORT Result<FileData> ReadFile(const nx::core::HDF5::FileIO& fileReader, bool preflight = false);

/**
 * @brief Imports a Pipeline and DataStructure from a DREAM3D path.
 *
 * Legacy files return an empty Pipeline and a warning.
 * @param path Identifies the file.
 * @return Imported pair or read error.
 */
SIMPLNX_EXPORT Result<FileData> ReadFile(const std::filesystem::path& path);

/**
 * @brief Loads a complete DataStructure with materialized array stores.
 *
 * A registered import finalizer can attach disk-backed stores. Core then eagerly
 * loads any Empty placeholders that remain. Without a finalizer, core loads all arrays.
 *
 * @param path Identifies a current or legacy DREAM3D file.
 * @return Materialized DataStructure or import error.
 */
SIMPLNX_EXPORT Result<DataStructure> LoadDataStructure(const std::filesystem::path& path);

/**
 * @brief Loads a complete DataStructure with an optional format resolver.
 *
 * The resolver is installed before import finalization. A finalizer can therefore
 * choose disk-backed stores per array. Null keeps the process-level default resolver.
 *
 * @param path Identifies a current or legacy DREAM3D file.
 * @param resolver Specifies per-DataStructure storage policy, or null for process default.
 * @return Materialized DataStructure or import error.
 */
SIMPLNX_EXPORT Result<DataStructure> LoadDataStructure(const std::filesystem::path& path, std::shared_ptr<const IDataStoreFormatResolver> resolver);

/**
 * @brief Loads selected arrays and their ancestor objects.
 *
 * The returned structure removes unrequested objects. Every retained array has
 * an in-core or disk-backed store, not an Empty placeholder.
 *
 * @param path Identifies a current or legacy DREAM3D file.
 * @param dataPaths Specifies arrays to retain.
 * @return Pruned materialized DataStructure or import error.
 */
SIMPLNX_EXPORT Result<DataStructure> LoadDataStructureArrays(const std::filesystem::path& path, const std::vector<DataPath>& dataPaths);

/**
 * @brief Loads selected arrays with an optional format resolver.
 *
 * The resolver is installed before import finalization. Unrequested objects are
 * pruned after retained arrays receive materialized stores.
 *
 * @param path Identifies a current or legacy DREAM3D file.
 * @param dataPaths Specifies arrays to retain.
 * @param resolver Specifies per-DataStructure storage policy, or null for process default.
 * @return Pruned materialized DataStructure or import error.
 */
SIMPLNX_EXPORT Result<DataStructure> LoadDataStructureArrays(const std::filesystem::path& path, const std::vector<DataPath>& dataPaths, std::shared_ptr<const IDataStoreFormatResolver> resolver);

/**
 * @brief Loads a metadata-only DataStructure.
 *
 * The complete hierarchy is present. DataArrays receive Empty placeholder stores.
 *
 * @param path Identifies a current or legacy DREAM3D file.
 * @return Metadata structure or import error.
 */
SIMPLNX_EXPORT Result<DataStructure> LoadDataStructureMetadata(const std::filesystem::path& path);

/**
 * @brief Loads metadata for selected arrays and their ancestors.
 *
 * Retained arrays use Empty placeholder stores. Unrequested objects are removed.
 *
 * @param path Identifies a current or legacy DREAM3D file.
 * @param dataPaths Specifies arrays to retain.
 * @return Pruned metadata structure or import error.
 */
SIMPLNX_EXPORT Result<DataStructure> LoadDataStructureArraysMetadata(const std::filesystem::path& path, const std::vector<DataPath>& dataPaths);

/**
 * @brief Writes Pipeline and DataStructure data to an open DREAM3D file.
 * @param fileWriter Receives serialized content.
 * @param fileData Provides Pipeline and DataStructure data.
 * @return First HDF5 write error, or success.
 */
SIMPLNX_EXPORT Result<> WriteFile(nx::core::HDF5::FileIO& fileWriter, const FileData& fileData);

/**
 * @brief Writes a Pipeline and DataStructure to an open DREAM3D file.
 * @param fileWriter Receives serialized content.
 * @param pipeline Provides pipeline metadata.
 * @param dataStructure Provides hierarchy and arrays.
 * @return First HDF5 write error, or success.
 */
SIMPLNX_EXPORT Result<> WriteFile(nx::core::HDF5::FileIO& fileWriter, const Pipeline& pipeline, const DataStructure& dataStructure);

/**
 * @brief Writes a Pipeline and DataStructure with explicit options.
 * @param fileWriter Receives serialized content.
 * @param pipeline Provides pipeline metadata.
 * @param dataStructure Provides hierarchy and arrays.
 * @param options Specifies write-time compression and related policies.
 * @return First HDF5 write error, or success.
 */
SIMPLNX_EXPORT Result<> WriteFile(nx::core::HDF5::FileIO& fileWriter, const Pipeline& pipeline, const DataStructure& dataStructure, const nx::core::HDF5::DataStructureWriter::WriteOptions& options);

/**
 * @brief Writes a DataStructure and optional Pipeline to a DREAM3D path.
 * @param path Specifies the destination file.
 * @param dataStructure Provides hierarchy and arrays.
 * @param pipeline Provides optional pipeline metadata.
 * @param writeXdmf Writes a sibling XDMF file when true.
 * @return File or HDF5 write error, or success.
 */
SIMPLNX_EXPORT Result<> WriteFile(const std::filesystem::path& path, const DataStructure& dataStructure, const Pipeline& pipeline = {}, bool writeXdmf = false);

/**
 * @brief Writes a DataStructure and Pipeline with explicit options.
 * @param path Specifies the destination file.
 * @param dataStructure Provides hierarchy and arrays.
 * @param pipeline Provides pipeline metadata.
 * @param writeXdmf Writes a sibling XDMF file when true.
 * @param options Specifies write-time compression and related policies.
 * @return File or HDF5 write error, or success.
 */
SIMPLNX_EXPORT Result<> WriteFile(const std::filesystem::path& path, const DataStructure& dataStructure, const Pipeline& pipeline, bool writeXdmf,
                                  const nx::core::HDF5::DataStructureWriter::WriteOptions& options);

/**
 * @brief Writes a full recovery snapshot or a redirect file.
 *
 * Without userDataFilePath, in-core arrays write payloads. OOC arrays write
 * placeholders and recovery metadata for backing-store reconstruction.
 *
 * With userDataFilePath, the writer ignores dataStructure and pipeline. It writes
 * only file version and an absolute UserDataFilePath attribute.
 *
 * @param path Specifies the recovery destination.
 * @param dataStructure Provides final pipeline data for a full snapshot.
 * @param pipeline Provides embedded pipeline metadata for a full snapshot.
 * @param userDataFilePath Selects redirect mode and its authoritative file.
 * @return File or HDF5 write error, or success.
 */
SIMPLNX_EXPORT Result<> WriteRecoveryFile(const std::filesystem::path& path, const DataStructure& dataStructure, const Pipeline& pipeline = {},
                                          std::optional<std::filesystem::path> userDataFilePath = std::nullopt);

/**
 * @brief Reads an optional recovery UserDataFilePath attribute.
 *
 * An absent attribute identifies a data-carrying recovery file and is not an error.
 *
 * @param recoveryFilePath Identifies the recovery file.
 * @return Null optional, redirect path, or HDF5 read error.
 */
SIMPLNX_EXPORT Result<std::optional<std::filesystem::path>> ReadUserDataFilePathAttribute(const std::filesystem::path& recoveryFilePath);

/**
 * @brief Appends one DataObject to an existing DREAM3D file.
 * @param path Identifies the destination file.
 * @param dataStructure Provides the object and its dependencies.
 * @param dataPath Identifies the object to append.
 * @return File, lookup, or HDF5 write error, or success.
 */
SIMPLNX_EXPORT Result<> AppendFile(const std::filesystem::path& path, const DataStructure& dataStructure, const DataPath& dataPath);

/**
 * @brief Imports a current or legacy DataStructure from an open file.
 * @param fileReader Provides an open file.
 * @param preflight Creates placeholder stores when true.
 * @return Imported DataStructure or read error.
 */
SIMPLNX_EXPORT Result<DataStructure> ImportDataStructureFromFile(const nx::core::HDF5::FileIO& fileReader, bool preflight);

/**
 * @brief Imports one DataObject and its stored data from an open file.
 * @param fileReader Provides an open file.
 * @param dataPath Identifies the object.
 * @return Imported object or read error.
 */
SIMPLNX_EXPORT Result<std::shared_ptr<DataObject>> ImportDataObjectFromFile(const nx::core::HDF5::FileIO& fileReader, const DataPath& dataPath);

/**
 * @brief Imports selected DataObjects from an open file.
 * @param fileReader Provides an open file.
 * @param dataPaths Identifies requested objects.
 * @return Imported objects or first read error.
 */
SIMPLNX_EXPORT Result<std::vector<std::shared_ptr<DataObject>>> ImportSelectDataObjectsFromFile(const nx::core::HDF5::FileIO& fileReader, const std::vector<DataPath>& dataPaths);

/**
 * @brief Inserts one metadata-only object without reopening its source file.
 *
 * Preflight does not read array contents. Avoiding an open file removes one
 * network-storage round trip.
 * @param importStructure Provides the metadata object.
 * @param dataStructure Receives a shallow copy.
 * @param dataPath Identifies the object.
 * @return Lookup or insertion error, or success.
 */
SIMPLNX_EXPORT Result<> FinishImportingObjectPreflight(DataStructure& importStructure, DataStructure& dataStructure, const DataPath& dataPath);

/**
 * @brief Inserts and optionally materializes one imported object.
 * @param importStructure Provides imported metadata.
 * @param dataStructure Receives the object.
 * @param dataPath Identifies the object.
 * @param fileReader Provides stored data during execution.
 * @param preflight Inserts only metadata when true.
 * @return Lookup, insertion, or HDF5 read error, or success.
 */
SIMPLNX_EXPORT Result<> FinishImportingObject(DataStructure& importStructure, DataStructure& dataStructure, const DataPath& dataPath, const nx::core::HDF5::FileIO& fileReader, bool preflight);

/**
 * @brief Imports a current or legacy DataStructure from a file path.
 * @param filePath Identifies the file.
 * @param preflight Creates placeholder stores when true.
 * @return Imported DataStructure or read error.
 */
SIMPLNX_EXPORT Result<DataStructure> ImportDataStructureFromFile(const std::filesystem::path& filePath, bool preflight);

/**
 * @brief Imports a Pipeline from an open DREAM3D file.
 *
 * Legacy pipelines are not supported.
 * @param fileReader Provides an open file.
 * @return Imported Pipeline or read error.
 */
SIMPLNX_EXPORT Result<Pipeline> ImportPipelineFromFile(const nx::core::HDF5::FileIO& fileReader);

/**
 * @brief Imports Pipeline JSON from an open DREAM3D file.
 *
 * Legacy pipelines are not supported.
 * @param fileReader Provides an open file.
 * @return Imported JSON or read error.
 */
SIMPLNX_EXPORT Result<nlohmann::json> ImportPipelineJsonFromFile(const nx::core::HDF5::FileIO& fileReader);

/**
 * @brief Imports Pipeline JSON from a DREAM3D path.
 *
 * Legacy pipelines are not supported.
 * @param filePath Identifies the file.
 * @return Imported JSON or read error.
 */
SIMPLNX_EXPORT Result<nlohmann::json> ImportPipelineJsonFromFile(const std::filesystem::path& filePath);

/**
 * @brief Imports a Pipeline from a DREAM3D path.
 *
 * Legacy pipelines are not supported.
 * @param filePath Identifies the file.
 * @return Imported Pipeline or read error.
 */
SIMPLNX_EXPORT Result<Pipeline> ImportPipelineFromFile(const std::filesystem::path& filePath);

/**
 * @brief Writes XDMF metadata for an existing DREAM3D HDF5 file.
 * @param filePath Specifies the XDMF destination.
 * @param dataStructure Provides geometry and array metadata.
 * @param hdf5FilePath Specifies the referenced HDF5 path.
 */
SIMPLNX_EXPORT void WriteXdmf(const std::filesystem::path& filePath, const DataStructure& dataStructure, std::string_view hdf5FilePath);

/**
 * @brief Adds every ancestor of selected paths.
 * @param selectedPaths Specifies source paths.
 * @return Unique selected paths and ancestors.
 */
SIMPLNX_EXPORT std::vector<nx::core::DataPath> ExpandSelectedPathsToAncestors(const std::vector<nx::core::DataPath>& selectedPaths);

/**
 * @brief Adds every descendant of selected paths.
 * @param selectedPaths Specifies source paths.
 * @param allPaths Provides candidates.
 * @return Unique selected paths and descendants.
 */
SIMPLNX_EXPORT std::vector<nx::core::DataPath> ExpandSelectedPathsToDescendants(const std::vector<nx::core::DataPath>& selectedPaths, const std::vector<nx::core::DataPath>& allPaths);

} // namespace DREAM3D
} // namespace nx::core
