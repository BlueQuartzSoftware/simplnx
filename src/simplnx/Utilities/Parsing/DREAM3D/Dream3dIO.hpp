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

namespace DREAM3D
{
using FileData = std::pair<Pipeline, DataStructure>;
using FileVersionType = std::string;
using PipelineVersionType = int32;

inline constexpr int32 k_InvalidPipelineVersion = -404;
inline constexpr int32 k_InvalidDataStructureVersion = -405;
inline constexpr int32 k_PipelineGroupUnavailable = -406;
inline constexpr StringLiteral k_CurrentFileVersion = "8.0";
inline constexpr StringLiteral k_LegacyFileVersion = "7.0";

/**
 * @brief Returns the DREAM3D file version.
 * @param path
 * @return FileVersionType
 */
SIMPLNX_EXPORT FileVersionType GetFileVersion(const std::filesystem::path& path);

/**
 * @brief Returns the DREAM3D file version.
 * @param fileReader
 * @return FileVersionType
 */
SIMPLNX_EXPORT FileVersionType GetFileVersion(const nx::core::HDF5::FileIO& fileReader);

/**
 * @brief Returns the DREAM3D pipeline version.
 * @param fileReader
 * @return PipelineVersionType
 */
SIMPLNX_EXPORT PipelineVersionType GetPipelineVersion(const nx::core::HDF5::FileIO& fileReader);

/**
 * @brief Loads a complete DataStructure from a .dream3d file with all arrays
 * receiving real data stores (in-core or OOC).
 *
 * Supports both v8.0 and legacy v7.0 file formats. In an OOC-enabled build
 * (compiled in under SIMPLNX_USE_OOC) the OOC import path decides whether each
 * array becomes an in-core DataStore or a lazy OOC store backed by the HDF5
 * file; in a non-OOC build every array is eager-loaded in-core.
 *
 * @param path Filesystem path to the .dream3d file
 * @return Result containing the fully loaded DataStructure, or errors on failure
 */
SIMPLNX_EXPORT Result<DataStructure> LoadDataStructure(const std::filesystem::path& path);

/**
 * @brief Loads specific arrays from a .dream3d file with real data stores,
 * pruning all unrequested objects from the result.
 *
 * Only the requested arrays (and their ancestor containers) are present in
 * the returned DataStructure. No Empty placeholder stores remain — every
 * array in the result has been fully loaded or attached to an OOC store.
 *
 * @param path Filesystem path to the .dream3d file
 * @param dataPaths The specific DataPaths to load from the file
 * @return Result containing the pruned DataStructure with only requested arrays
 */
SIMPLNX_EXPORT Result<DataStructure> LoadDataStructureArrays(const std::filesystem::path& path, const std::vector<DataPath>& dataPaths);

/**
 * @brief Loads the topology (metadata skeleton) of a .dream3d file without
 * loading any array data. All DataArrays receive Empty placeholder stores.
 *
 * This is the preflight/metadata-only path: the returned DataStructure has
 * the complete hierarchy (geometries, attribute matrices, arrays) but none
 * of the arrays contain real data.
 *
 * @param path Filesystem path to the .dream3d file
 * @return Result containing the metadata-only DataStructure with Empty stores
 */
SIMPLNX_EXPORT Result<DataStructure> LoadDataStructureMetadata(const std::filesystem::path& path);

/**
 * @brief Loads the topology (metadata skeleton) for specific arrays from a
 * .dream3d file. All arrays receive Empty placeholder stores, and unrequested
 * objects are pruned from the result.
 *
 * Combines the metadata-only behavior of LoadDataStructureMetadata with the
 * path-based pruning of LoadDataStructureArrays.
 *
 * @param path Filesystem path to the .dream3d file
 * @param dataPaths The specific DataPaths whose metadata to load
 * @return Result containing the pruned metadata-only DataStructure
 */
SIMPLNX_EXPORT Result<DataStructure> LoadDataStructureArraysMetadata(const std::filesystem::path& path, const std::vector<DataPath>& dataPaths);

/**
 * @brief Writes a .dream3d file with the specified data.
 * @param fileWriter
 * @param fileData
 * @return Result<>
 */
SIMPLNX_EXPORT Result<> WriteFile(nx::core::HDF5::FileIO& fileWriter, const FileData& fileData);

/**
 * @brief Writes a .dream3d file with the specified data.
 * @param fileWriter
 * @param fileData
 * @return Result<>
 */
SIMPLNX_EXPORT Result<> WriteFile(nx::core::HDF5::FileIO& fileWriter, const Pipeline& pipeline, const DataStructure& dataStructure);

/**
 * @brief Writes a .dream3d file with the specified data and explicit write options.
 *        Equivalent to the no-options overload when a default-constructed WriteOptions is provided.
 * @param fileWriter     An open HDF5 file writer to receive the serialized content.
 * @param pipeline       Pipeline metadata to embed alongside the DataStructure.
 * @param dataStructure  DataStructure to serialize.
 * @param options        Write-time options (e.g. gzip compression level for DataArray datasets).
 * @return Result<>      Success, or an error describing the first failing write step.
 */
SIMPLNX_EXPORT Result<> WriteFile(nx::core::HDF5::FileIO& fileWriter, const Pipeline& pipeline, const DataStructure& dataStructure, const nx::core::HDF5::DataStructureWriter::WriteOptions& options);

/**
 * @brief Writes a .dream3d file with the specified data.
 * @param path
 * @param dataStructure
 * @param writeXdmf
 * @return bool
 */
SIMPLNX_EXPORT Result<> WriteFile(const std::filesystem::path& path, const DataStructure& dataStructure, const Pipeline& pipeline = {}, bool writeXdmf = false);

/**
 * @brief Writes a .dream3d file with the specified data and explicit write options.
 *        Equivalent to the no-options overload when a default-constructed WriteOptions is provided.
 * @param path           Destination filesystem path for the .dream3d file.
 * @param dataStructure  DataStructure to serialize.
 * @param pipeline       Pipeline metadata to embed alongside the DataStructure.
 * @param writeXdmf      If true, also produces a sibling .xdmf file next to the .dream3d file.
 * @param options        Write-time options (e.g. gzip compression level for DataArray datasets).
 * @return Result<>      Success, or an error describing the first failing write step.
 */
SIMPLNX_EXPORT Result<> WriteFile(const std::filesystem::path& path, const DataStructure& dataStructure, const Pipeline& pipeline, bool writeXdmf,
                                  const nx::core::HDF5::DataStructureWriter::WriteOptions& options);

/**
 * @brief Writes a recovery snapshot of @p dataStructure to @p path.
 *
 * When @p userDataFilePath is unset (default), the full recovery file is
 * written: in-core arrays get their data payload, OOC-backed arrays get a
 * placeholder plus their getRecoveryMetadata() key/value attributes so the
 * recovery loader can reconstruct the backing store on load.
 *
 * When @p userDataFilePath is set, @p dataStructure and @p pipeline are
 * ignored and a minimal HDF5 file is written containing only the file-
 * version attribute and a root-level string attribute named
 * "UserDataFilePath" whose value is the absolute path of the user's
 * authoritative `.dream3d` output. The recovery scanner uses that attribute
 * at relaunch time to redirect the load at the user's file.
 *
 * @param path Target path of the recovery file ("{uuid}.dream3d").
 * @param dataStructure Pipeline's final DataStructure (ignored when
 *                      @p userDataFilePath is set).
 * @param pipeline      Pipeline JSON to embed (ignored when
 *                      @p userDataFilePath is set).
 * @param userDataFilePath Optional absolute path to the user's own
 *                         `.dream3d` file. When set, switches the writer
 *                         to minimal redirect mode.
 * @return Result<> ok on success; error payload on HDF5-level failure
 *         (file open or version-tag write).
 */
SIMPLNX_EXPORT Result<> WriteRecoveryFile(const std::filesystem::path& path, const DataStructure& dataStructure, const Pipeline& pipeline = {},
                                          std::optional<std::filesystem::path> userDataFilePath = std::nullopt);

/**
 * @brief Reads the "UserDataFilePath" root-level HDF5 string attribute
 *        from a recovery file.
 *
 * The recovery scanner calls this on every `{uuid}.dream3d` it finds at
 * startup; when a value comes back it means the pipeline ended with a
 * WriteDREAM3DFilter and the returned path is the user's authoritative
 * output. Absent attribute is NOT an error — it just means this recovery
 * file carries its own data (the standard case).
 *
 * @param recoveryFilePath Path to the `{uuid}.dream3d` to inspect.
 * @return Result<std::optional<std::filesystem::path>>
 *         - ok + nullopt: attribute absent, this is a standard recovery file
 *         - ok + path: attribute set, caller should redirect to that path
 *         - error: HDF5 open/read failure (corrupt file, missing, etc.)
 */
SIMPLNX_EXPORT Result<std::optional<std::filesystem::path>> ReadUserDataFilePathAttribute(const std::filesystem::path& recoveryFilePath);

/**
 * @brief Appends the object at the path in the data structure to the dream3d file
 * @param path
 * @param dataStructure
 * @param dataPath
 * @return Result<>
 */
SIMPLNX_EXPORT Result<> AppendFile(const std::filesystem::path& path, const DataStructure& dataStructure, const DataPath& dataPath);

/**
 * @brief Imports and returns a Pipeline from the target .dream3d file.
 *
 * This method does not import legacy Pipelines.
 * @param fileReader
 * @return Pipeline
 */
SIMPLNX_EXPORT Result<Pipeline> ImportPipelineFromFile(const nx::core::HDF5::FileIO& fileReader);

/**
 * @brief Imports and returns a Pipeline from the target .dream3d file.
 *
 * This method does not import legacy Pipelines.
 * @param fileReader
 * @return Pipeline
 */
SIMPLNX_EXPORT Result<nlohmann::json> ImportPipelineJsonFromFile(const nx::core::HDF5::FileIO& fileReader);

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

SIMPLNX_EXPORT std::vector<nx::core::DataPath> ExpandSelectedPathsToAncestors(const std::vector<nx::core::DataPath>& selectedPaths);

SIMPLNX_EXPORT std::vector<nx::core::DataPath> ExpandSelectedPathsToDescendants(const std::vector<nx::core::DataPath>& selectedPaths, const std::vector<nx::core::DataPath>& allPaths);

} // namespace DREAM3D
} // namespace nx::core
