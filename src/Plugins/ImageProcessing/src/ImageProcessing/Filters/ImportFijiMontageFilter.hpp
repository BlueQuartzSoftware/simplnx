#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"
#include "ImageProcessing/utils/FijiMontageUtilities.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/ImageIO/ImageMetadata.hpp"

#include <filesystem>
#include <vector>

namespace nx::core
{
/**
 * @class ImportFijiMontageFilter
 * @brief Reads a Fiji TileConfiguration[.registered].txt and imports each tile as its own
 *        2D ImageGeometry positioned at the parsed origin. ITK-free replacement for the
 *        legacy ITKImportFijiMontageFilter (UUID 4c48ea16-...). No stitching is performed.
 */
class IMAGEPROCESSING_EXPORT ImportFijiMontageFilter : public IFilter
{
public:
  ImportFijiMontageFilter() = default;
  ~ImportFijiMontageFilter() noexcept override = default;

  ImportFijiMontageFilter(const ImportFijiMontageFilter&) = delete;
  ImportFijiMontageFilter(ImportFijiMontageFilter&&) noexcept = delete;
  ImportFijiMontageFilter& operator=(const ImportFijiMontageFilter&) = delete;
  ImportFijiMontageFilter& operator=(ImportFijiMontageFilter&&) noexcept = delete;

  // Parameter Keys — match ITKImportFijiMontageFilter verbatim so a FUTURE retirement redirect
  // (deferred to the ITK-removal phase; no getFilterReplacementMap entry exists yet) can be
  // parameter-lossless.
  static constexpr StringLiteral k_InputFile_Key = "input_file";
  static constexpr StringLiteral k_DataGroupName_Key = "data_group_name";
  static constexpr StringLiteral k_LengthUnit_Key = "length_unit_index";
  static constexpr StringLiteral k_ChangeOrigin_Key = "change_origin";
  static constexpr StringLiteral k_Origin_Key = "origin";
  static constexpr StringLiteral k_ChangeDataType_Key = "change_image_data_type";
  static constexpr StringLiteral k_ImageDataType_Key = "image_data_type_index";
  static constexpr StringLiteral k_ParentDataGroup_Key = "parent_data_group";
  static constexpr StringLiteral k_ConvertToGrayScale_Key = "convert_to_gray_scale";
  static constexpr StringLiteral k_ColorWeights_Key = "color_weights";
  static constexpr StringLiteral k_DataContainerPath_Key = "data_container_path"; // image-geometry prefix
  static constexpr StringLiteral k_CellAttributeMatrixName_Key = "cell_attribute_matrix_name";
  static constexpr StringLiteral k_ImageDataArrayName_Key = "image_data_array_name";

  std::string name() const override;
  std::string className() const override;
  Uuid uuid() const override;
  std::string humanName() const override;
  std::vector<std::string> defaultTags() const override;
  Parameters parameters() const override;
  VersionType parametersVersion() const override;
  UniquePointer clone() const override;

protected:
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                const ExecutionContext& executionContext) const override;
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                       const ExecutionContext& executionContext) const override;

private:
  /**
   * @brief Per-instance metadata cache. Keyed on the config file path + its last-write-time, it
   *        memoizes the two expensive, file-I/O-bound steps of preflight — parsing the
   *        TileConfiguration and reading each tile's ImageMetadata — so repeated preflights (GUI
   *        parameter edits, plus the framework's pre-execute preflight) do not re-open the
   *        montage's (potentially thousands of) tile files when the config file is unchanged.
   *        Everything that depends on the live parameters (tile names, origin rebasing, output
   *        data type) is recomputed on every call, so a parameter edit can never surface a stale
   *        cached value; only a change to the config file (detected by the timestamp) invalidates
   *        the cache. Modeled on the legacy ITKImportFijiMontageFilter's FijiCache, but held as a
   *        per-instance member (not a shared static map) and covering strictly the file-derived data.
   */
  struct MetadataCache
  {
    std::filesystem::path inputFile;
    std::filesystem::file_time_type timeStamp;
    std::vector<fiji::FijiTile> rawTiles; ///< Parsed straight from the config (no prefix, no rebase).
    std::vector<ImageMetadata> metadata;  ///< Parallel to rawTiles; populated only when metadataValid.
    bool tilesValid = false;
    bool metadataValid = false;
  };
  mutable MetadataCache m_Cache;

  /**
   * @brief Ensures m_Cache holds the parsed tiles (and, when @p needMetadata is true, each tile's
   *        ImageMetadata) for @p inputFile, refreshing only if the file path or its last-write-time
   *        changed. Returns a working copy of the tiles with names assigned from @p prefix. Tile
   *        origins are NOT rebased here (a preflight-only concern). On a valid return with
   *        needMetadata==true, m_Cache.metadata is populated and parallel to the returned tiles.
   */
  Result<std::vector<fiji::FijiTile>> loadTiles(const std::filesystem::path& inputFile, const std::string& prefix, bool needMetadata) const;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ImportFijiMontageFilter, "b9c66c77-5c7f-4951-bb02-981db3c297ae");
