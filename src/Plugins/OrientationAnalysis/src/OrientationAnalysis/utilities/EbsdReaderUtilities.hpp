#pragma once

#include "OrientationAnalysis/utilities/GrainMapper3DUtilities.hpp"

#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <H5Support/H5Utilities.h>
#include <algorithm>

#include <EbsdLib/IO/BrukerNano/H5EspritReader.h>
#include <EbsdLib/IO/HKL/CprReader.h>
#include <EbsdLib/IO/HKL/CtfReader.h>
#include <EbsdLib/IO/HKL/H5CtfVolumeReader.h>
#include <EbsdLib/IO/HKL/H5OINAReader.h>
#include <EbsdLib/IO/TSL/AngReader.h>
#include <EbsdLib/IO/TSL/H5AngVolumeReader.h>
#include <EbsdLib/IO/TSL/H5OIMReader.h>
#include <EbsdLib/LaueOps/LaueOps.h>
#include <EbsdLib/Math/EbsdLibMath.h>

#include <cmath>
#include <concepts>
#include <filesystem>
#include <functional>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

using namespace nx::core;

namespace EbsdReaderUtilities
{
/**
 * @brief Return the lowercase file suffix for EBSD dispatch.
 * @param path Input file path.
 * @return Lowercase suffix with its leading dot, or an empty string if absent.
 */
inline std::string NormalizedExtension(const std::filesystem::path& path)
{
  return StringUtilities::toLower(path.extension().string());
}

/** @brief Error for an output file that resolves to its input. */
inline constexpr int32 k_SameHexOutputFile = -44604;

/**
 * @brief Reject a final output that resolves to its input before creating a temporary file.
 * @param input Source ANG file path.
 * @param output Final output path, before AtomicFile creates a temporary file.
 * @return Error if normalized paths match or existing files are equivalent.
 */
inline Result<> ValidateHexOutputPath(const std::filesystem::path& input, const std::filesystem::path& output)
{
  std::error_code inputError;
  std::error_code outputError;
  const auto normalizedInput = std::filesystem::weakly_canonical(input, inputError);
  const auto normalizedOutput = std::filesystem::weakly_canonical(output, outputError);
  std::error_code equivalentError;
  if((!inputError && !outputError && normalizedInput == normalizedOutput) || std::filesystem::equivalent(input, output, equivalentError))
  {
    return MakeErrorResult(k_SameHexOutputFile, fmt::format("Output file '{}' is the input file '{}'. Choose another output directory or a nonempty output prefix.", output.string(), input.string()));
  }
  return {};
}

/**
 * @brief Gives the rotation about the crystal c-axis from the Oxford/HKL hexagonal frame to EDAX/TSL, in radians.
 * @tparam T Floating-point type for the offset.
 *
 * Oxford/HKL uses x || a*. EDAX/TSL uses x || a.
 */
template <typename T>
inline constexpr T k_HexagonalAlignmentRadians = static_cast<T>(30.0) * ebsdlib::constants::k_PiOver180<T>;
/**
 * @brief Offset in radians for float32 calculations.
 */
inline constexpr float32 k_HexagonalAlignmentRadiansF = k_HexagonalAlignmentRadians<float32>;
/**
 * @brief Offset in radians for float64 addition before conversion to float32.
 */
inline constexpr float64 k_HexagonalAlignmentRadiansD = k_HexagonalAlignmentRadians<float64>;
/**
 * @brief Offset in degrees for CTF data before optional conversion to radians.
 */
inline constexpr float64 k_HexagonalAlignmentDegrees = 30.0;

/**
 * @brief Tests whether a crystal structure uses a hexagonal Laue class.
 * @param crystalStructure EbsdLib crystal-structure value.
 * @return True for 6/m or 6/mmm; false for other structures, including unknown.
 */
inline constexpr bool IsHexagonalCrystalStructure(uint32 crystalStructure)
{
  return crystalStructure == ebsdlib::CrystalStructure::Hexagonal_Low || crystalStructure == ebsdlib::CrystalStructure::Hexagonal_High;
}

/** Error codes shared by EBSD header validation in preflight and execution. */
inline constexpr int32 k_OimMissingPatternDimensions = -9583;
inline constexpr int32 k_EspritMissingPatternDimensions = -9683;
inline constexpr int32 k_InvalidPhaseSymmetry = -67600;
inline constexpr int32 k_InvalidHeaderDimension = -67601;
inline constexpr int32 k_InvalidHeaderSpacing = -67602;
inline constexpr int32 k_InvalidSampleTransform = -67603;
inline constexpr int32 k_HeaderCountExceedsReader = -67604;
inline constexpr int32 k_HeaderCountProductExceedsReader = -67605;

/**
 * @brief Reject non-positive header dimensions before conversion to an unsigned shape.
 * @tparam T Integral header dimension type.
 * @param file Input file path for the diagnostic.
 * @param key Header key for the dimension.
 * @param value Parsed header dimension.
 * @return Error if the dimension is non-positive or cannot fit usize.
 */
template <typename T>
Result<> ValidateHeaderDimension(const std::string& file, const std::string& key, T value)
{
  if(value <= 0 || static_cast<uint64>(value) > std::numeric_limits<usize>::max())
  {
    return MakeErrorResult(k_InvalidHeaderDimension, fmt::format("File '{}' has invalid header '{}' = {}. Use a positive dimension that fits the output shape.", file, key, value));
  }
  return {};
}

/**
 * @brief Check spacing in both the header type and the Image Geometry's float32 type.
 * @tparam T Floating-point header spacing type.
 * @param file Input file path for the diagnostic.
 * @param key Header key for the spacing.
 * @param value Parsed header spacing.
 * @param normalizeZero Permit exact zero when the format defines a positive default.
 * @return Error if the spacing is nonfinite, non-positive, or cannot fit float32.
 */
template <typename T>
Result<> ValidateHeaderSpacing(const std::string& file, const std::string& key, T value, bool normalizeZero = false)
{
  if(normalizeZero && value == 0)
  {
    return {};
  }
  if(!std::isfinite(value) || value <= 0 || value > std::numeric_limits<float32>::max() || static_cast<float32>(value) <= 0)
  {
    return MakeErrorResult(k_InvalidHeaderSpacing, fmt::format("File '{}' has invalid header '{}' = {}. Use a finite, positive spacing that fits the Image Geometry.", file, key, value));
  }
  return {};
}

// 1. Base template (empty)
template <typename ReaderType>
struct ReaderTraits;

// 2. Specialization for AngReader
template <>
struct ReaderTraits<ebsdlib::AngReader>
{
  static auto getCols()
  {
    return std::mem_fn(&ebsdlib::AngReader::getNumEvenCols);
  }
  static auto getRows()
  {
    return std::mem_fn(&ebsdlib::AngReader::getNumRows);
  }
  static auto getXSpacing()
  {
    return std::mem_fn(&ebsdlib::AngReader::getXStep);
  }
  static auto getYSpacing()
  {
    return std::mem_fn(&ebsdlib::AngReader::getYStep);
  }
};

template <>
struct ReaderTraits<ebsdlib::H5OIMReader> : public ReaderTraits<ebsdlib::AngReader>
{
};

// 2. Specialization for CtfReader
template <>
struct ReaderTraits<ebsdlib::CtfReader>
{
  static auto getCols()
  {
    return std::mem_fn(&ebsdlib::CtfReader::getXCells);
  }
  static auto getRows()
  {
    return std::mem_fn(&ebsdlib::CtfReader::getYCells);
  }
  static auto getXSpacing()
  {
    return std::mem_fn(&ebsdlib::CtfReader::getXStep);
  }
  static auto getYSpacing()
  {
    return std::mem_fn(&ebsdlib::CtfReader::getYStep);
  }
};

template <>
struct ReaderTraits<ebsdlib::H5OINAReader>
{
  static auto getCols()
  {
    return std::mem_fn(&ebsdlib::H5OINAReader::getXCells);
  }
  static auto getRows()
  {
    return std::mem_fn(&ebsdlib::H5OINAReader::getYCells);
  }
  static auto getXSpacing()
  {
    return std::mem_fn(&ebsdlib::H5OINAReader::getXStep);
  }
  static auto getYSpacing()
  {
    return std::mem_fn(&ebsdlib::H5OINAReader::getYStep);
  }
};

template <>
struct ReaderTraits<ebsdlib::CprReader> : public ReaderTraits<ebsdlib::CtfReader>
{
  static auto getCols()
  {
    return std::mem_fn(&ebsdlib::CprReader::getXCells);
  }
  static auto getRows()
  {
    return std::mem_fn(&ebsdlib::CprReader::getYCells);
  }
  static auto getXSpacing()
  {
    return std::mem_fn(&ebsdlib::CprReader::getXStep);
  }
  static auto getYSpacing()
  {
    return std::mem_fn(&ebsdlib::CprReader::getYStep);
  }
};

template <>
struct ReaderTraits<ebsdlib::H5EspritReader>
{
  static auto getCols()
  {
    return std::mem_fn(&ebsdlib::H5EspritReader::getNumColumns);
  }
  static auto getRows()
  {
    return std::mem_fn(&ebsdlib::H5EspritReader::getNumRows);
  }
  static auto getXSpacing()
  {
    return std::mem_fn(&ebsdlib::H5EspritReader::getXStep);
  }
  static auto getYSpacing()
  {
    return std::mem_fn(&ebsdlib::H5EspritReader::getYStep);
  }
};

template <>
struct ReaderTraits<GrainMapper3DUtilities::GrainMapperReader>
{
  static auto getCols()
  {
    return std::mem_fn(&GrainMapper3DUtilities::GrainMapperReader::getNumColumns);
  }
  static auto getRows()
  {
    return std::mem_fn(&GrainMapper3DUtilities::GrainMapperReader::getNumRows);
  }
  static auto getXSpacing()
  {
    return std::mem_fn(&GrainMapper3DUtilities::GrainMapperReader::getXStep);
  }
  static auto getYSpacing()
  {
    return std::mem_fn(&GrainMapper3DUtilities::GrainMapperReader::getYStep);
  }
};

template <>
struct ReaderTraits<ebsdlib::H5AngVolumeReader>
{
  static auto getCols()
  {
    return std::mem_fn(&ebsdlib::H5AngVolumeReader::getNumEvenCols);
  }
  static auto getRows()
  {
    return std::mem_fn(&ebsdlib::H5AngVolumeReader::getNumRows);
  }
  static auto getXSpacing()
  {
    return std::mem_fn(&ebsdlib::H5AngVolumeReader::getXStep);
  }
  static auto getYSpacing()
  {
    return std::mem_fn(&ebsdlib::H5AngVolumeReader::getYStep);
  }
};

template <>
struct ReaderTraits<ebsdlib::H5CtfVolumeReader>
{
  static auto getCols()
  {
    return std::mem_fn(&ebsdlib::H5CtfVolumeReader::getXCells);
  }
  static auto getRows()
  {
    return std::mem_fn(&ebsdlib::H5CtfVolumeReader::getYCells);
  }
  static auto getXSpacing()
  {
    return std::mem_fn(&ebsdlib::H5CtfVolumeReader::getXStep);
  }
  static auto getYSpacing()
  {
    return std::mem_fn(&ebsdlib::H5CtfVolumeReader::getYStep);
  }
};

/**
 * @brief Check header dimensions and steps before preview arithmetic or unsigned shape conversion.
 * @tparam ReaderType EBSD reader type.
 * @param reader Reader with a successfully parsed header.
 * @return The first invalid dimension or spacing, with file and header key context.
 */
template <typename ReaderType>
Result<> ValidateHeaderGrid(ReaderType& reader)
{
  std::array<std::string, 4> keys;
  if constexpr(std::is_same_v<ReaderType, ebsdlib::CprReader>)
  {
    keys = {"xCells", "yCells", "GridDistX", "GridDistY"};
  }
  else if constexpr(std::is_same_v<ReaderType, ebsdlib::CtfReader>)
  {
    keys = {"XCells", "YCells", "XStep", "YStep"};
  }
  else if constexpr(std::is_same_v<ReaderType, ebsdlib::H5OINAReader>)
  {
    keys = {"X Cells", "Y Cells", "X Step", "Y Step"};
  }
  else if constexpr(std::is_same_v<ReaderType, ebsdlib::H5OIMReader>)
  {
    keys = {"nColumns", "nRows", "Step X", "Step Y"};
  }
  else if constexpr(std::is_same_v<ReaderType, ebsdlib::AngReader>)
  {
    keys = {"NCOLS_EVEN", "NROWS", "XSTEP", "YSTEP"};
  }
  else
  {
    keys = {"NCOLS", "NROWS", "XSTEP", "YSTEP"};
  }
  const auto file = reader.getFileName();
  if(auto result = ValidateHeaderDimension(file, keys[0], ReaderTraits<ReaderType>::getCols()(reader)); result.invalid())
  {
    return result;
  }
  if(auto result = ValidateHeaderDimension(file, keys[1], ReaderTraits<ReaderType>::getRows()(reader)); result.invalid())
  {
    return result;
  }
  constexpr bool normalizeZero = std::is_same_v<ReaderType, ebsdlib::H5EspritReader>;
  if(auto result = ValidateHeaderSpacing(file, keys[2], ReaderTraits<ReaderType>::getXSpacing()(reader), normalizeZero); result.invalid())
  {
    return result;
  }
  return ValidateHeaderSpacing(file, keys[3], ReaderTraits<ReaderType>::getYSpacing()(reader), normalizeZero);
}

template <typename ReaderType>
void GeneratePreflightScanInformation(ReaderType& reader, std::vector<IFilter::PreflightValue>& preflightUpdatedValues)
{
  auto colCount = ReaderTraits<ReaderType>::getCols()(reader);
  auto rowCount = ReaderTraits<ReaderType>::getRows()(reader);
  auto xStep = ReaderTraits<ReaderType>::getXSpacing()(reader);
  auto yStep = ReaderTraits<ReaderType>::getYSpacing()(reader);

  std::stringstream ss;

  if constexpr(std::is_same_v<ReaderType, ebsdlib::AngReader>)
  {
    ss << "Grid: " << reader.getGrid() << "\n";
  }

  ss << "X Step: " << xStep << "    Y Step: " << yStep << "\n";
  if constexpr(std::is_same_v<ReaderType, ebsdlib::AngReader>)
  {
    ss << "Num Odd Cols: " << reader.getNumOddCols() << "    ";
    ss << "Num Even Cols: " << reader.getNumEvenCols() << "    ";
  }
  else
  {
    ss << "Num Cols: " << colCount << "    ";
  }

  ss << "Num Rows: " << rowCount << "\n"
     << "Sample Physical Dimensions: " << (xStep * colCount) << " (W) x " << (yStep * rowCount) << " (H) microns" << "\n";

  if constexpr(std::is_same_v<ReaderType, ebsdlib::CtfReader>)
  {
    if(reader.getZCells() > 1)
    {
      ss << "Num Slices (Z): " << reader.getZCells() << "    Z Step: " << (reader.getZStep() > 0.0F ? reader.getZStep() : 1.0F) << "\n";
    }
  }

  preflightUpdatedValues.push_back({"Scan Information", ss.str()});
}

/**
 * @brief Append phase previews after checking each symmetry lookup.
 * @tparam ReaderType EBSD or GrainMapper reader type.
 * @param reader Reader with a successfully parsed header.
 * @param preflightUpdatedValues Preview destination. Callers discard these values on error.
 * @param filePath Diagnostic file path for GrainMapperReader, which has no file path accessor.
 * @return Error for an unsupported phase symmetry, before indexing orientation operations.
 */
template <typename ReaderType>
Result<> GeneratePreflightPhaseInformation(ReaderType& reader, std::vector<IFilter::PreflightValue>& preflightUpdatedValues, const std::string& filePath = {})
{
  auto phaseInfos = reader.getPhaseVector();
  if(!phaseInfos.empty())
  {
    preflightUpdatedValues.push_back({"Phase Information", ""});
  }
  auto laueOps = ebsdlib::LaueOps::GetAllOrientationOps();
  // int phaseIndex = 1;
  for(const auto& phaseInfo : phaseInfos)
  {

    if constexpr(!std::is_same_v<ReaderType, GrainMapper3DUtilities::GrainMapperReader>)
    {
      if(phaseInfo == nullptr)
      {
        continue;
      }
      std::string key;
      int64 value = 0;
      if constexpr(std::is_same_v<ReaderType, ebsdlib::AngReader> || std::is_same_v<ReaderType, ebsdlib::H5OIMReader> || std::is_same_v<ReaderType, ebsdlib::H5AngVolumeReader>)
      {
        key = "Symmetry";
        value = phaseInfo->getSymmetry();
      }
      else if constexpr(std::is_same_v<ReaderType, ebsdlib::H5EspritReader>)
      {
        key = "IT";
        value = phaseInfo->getIT();
      }
      else
      {
        key = "Laue Group";
        value = static_cast<int64>(phaseInfo->getLaueGroup());
      }
      const auto index = phaseInfo->determineOrientationOpsIndex();
      if(index >= laueOps.size() || laueOps[index] == nullptr)
      {
        return MakeErrorResult(k_InvalidPhaseSymmetry, fmt::format("File '{}' phase {} has unsupported header '{}' = {}. Correct the phase symmetry in the source file.", reader.getFileName(),
                                                                   phaseInfo->getPhaseIndex(), key, value));
      }
    }

    if constexpr(std::is_same_v<ReaderType, ebsdlib::AngReader> || std::is_same_v<ReaderType, ebsdlib::H5OIMReader> || std::is_same_v<ReaderType, ebsdlib::H5AngVolumeReader>)
    {
      if(phaseInfo == nullptr)
      {
        continue;
      }
      preflightUpdatedValues.push_back({fmt::format("{}: ", phaseInfo->getPhaseIndex()), fmt::format("Material Name: {}    |    Formula: {}    |    Crystal Symmetry: {}", phaseInfo->getMaterialName(),
                                                                                                     phaseInfo->getFormula(), laueOps[phaseInfo->determineOrientationOpsIndex()]->getSymmetryName())});
    }

    if constexpr(std::is_same_v<ReaderType, ebsdlib::CtfReader> || std::is_same_v<ReaderType, ebsdlib::H5OINAReader> || std::is_same_v<ReaderType, ebsdlib::CprReader> ||
                 std::is_same_v<ReaderType, ebsdlib::H5CtfVolumeReader>)
    {
      if(phaseInfo == nullptr)
      {
        continue;
      }
      preflightUpdatedValues.push_back({fmt::format("{}: ", phaseInfo->getPhaseIndex()), fmt::format("Material Name: {}    |    Crystal Symmetry: {}    |    Comment: {}", phaseInfo->getMaterialName(),
                                                                                                     laueOps[phaseInfo->determineOrientationOpsIndex()]->getSymmetryName(), phaseInfo->getComment())});
    }

    if constexpr(std::is_same_v<ReaderType, ebsdlib::H5EspritReader>)
    {
      if(phaseInfo == nullptr)
      {
        continue;
      }
      preflightUpdatedValues.push_back(
          {fmt::format("{}: ", phaseInfo->getPhaseIndex()), fmt::format("Material Name: {}    |    Crystal Symmetry: {}    |    Space Group: {}", phaseInfo->getMaterialName(),
                                                                        laueOps[phaseInfo->determineOrientationOpsIndex()]->getSymmetryName(), phaseInfo->getSpaceGroup())});
    }

    if constexpr(std::is_same_v<ReaderType, GrainMapper3DUtilities::GrainMapperReader>)
    {
      // Same check, code (-67600) and message as the import (ReadGrainMapper3D) uses.
      if(auto result = GrainMapper3DUtilities::ValidateSpaceGroups({phaseInfo}, filePath); result.invalid())
      {
        return result;
      }
      const auto ops = ebsdlib::LaueOps::GetLaueOpsFromSpaceGroupNumber(static_cast<usize>(phaseInfo.SpaceGroup));
      preflightUpdatedValues.push_back({fmt::format("{}: ", phaseInfo.PhaseIndex),
                                        fmt::format("Material Name: {}    |    Crystal Symmetry: {}    |    Space Group: {}", phaseInfo.Name, ops->getSymmetryName(), phaseInfo.SpaceGroup)});
    }
  }
  return {};
}

/**
 * @brief Validate each selected OEM scan before creating a shared Image Geometry.
 * @tparam ReaderType OEM reader type.
 * @param reader Reader with the selected scan header loaded.
 * @param readPatterns Whether pattern dimensions are active.
 * @param missingPatternCode Existing filter error code for zero pattern dimensions.
 * @return The first invalid grid, phase symmetry, or active pattern dimension.
 */
template <typename ReaderType>
Result<> ValidateOemHeader(ReaderType& reader, bool readPatterns, int32 missingPatternCode)
{
  if(auto result = ValidateHeaderGrid(reader); result.invalid())
  {
    return result;
  }
  std::vector<IFilter::PreflightValue> preview;
  if(auto result = GeneratePreflightPhaseInformation(reader, preview); result.invalid())
  {
    return result;
  }
  if(readPatterns)
  {
    std::array<int32, 2> patternDims = {0, 0};
    reader.getPatternDims(patternDims);
    for(usize axis = 0; axis < patternDims.size(); ++axis)
    {
      if(auto result = ValidateHeaderDimension(reader.getFileName(), axis == 0 ? "Pattern Height" : "Pattern Width", patternDims[axis]); result.invalid())
      {
        // Zero keeps the filter's established missing-pattern code.
        if(patternDims[axis] == 0)
        {
          return MakeErrorResult(missingPatternCode, result.errors().front().message);
        }
        return result;
      }
    }
  }
  return {};
}

/** Error codes for scan structure and requested pattern metadata. */
inline constexpr int32 k_ScanGridMismatch = -67606;
inline constexpr int32 k_ScanSpacingMismatch = -67607;
inline constexpr int32 k_ScanPhaseCountMismatch = -67608;
inline constexpr int32 k_PatternDatasetMissing = -67609;
inline constexpr int32 k_PatternDatasetType = -67610;
inline constexpr int32 k_PatternDatasetRank = -67611;
inline constexpr int32 k_PatternDatasetExtent = -67612;
inline constexpr int32 k_SliceRangeOutsideFile = -67614;
inline constexpr int32 k_RequiredChannel5Field = -67615;
inline constexpr int32 k_OemDestinationMismatch = -67616;
inline constexpr int32 k_OemPatternDestinationMismatch = -67617;
inline constexpr int32 k_OemPhaseDestinationMismatch = -67618;
inline constexpr int32 k_ScanPhaseMismatch = -67619;
inline constexpr int32 k_ScanPatternMismatch = -67620;

/**
 * @brief Compare scan properties that share one geometry and ensemble table.
 * @tparam ReaderType Esprit or OIM reader type.
 * @param first Reference scan with a validated header.
 * @param scan Scan to compare with the reference.
 * @param readPatterns Whether the output includes patterns.
 * @return Error if the grids, spacing, phase tables, or active pattern shapes differ.
 */
template <typename ReaderType>
Result<> ValidateOemScanCompatibility(ReaderType& first, ReaderType& scan, bool readPatterns)
{
  const auto context = fmt::format("File '{}' scans '{}' and '{}'", scan.getFileName(), first.getHDF5Path(), scan.getHDF5Path());
  if(first.getXDimension() != scan.getXDimension() || first.getYDimension() != scan.getYDimension())
  {
    return MakeErrorResult(k_ScanGridMismatch,
                           fmt::format("Scan '{}' in file '{}' has {} columns and {} rows, but the first selected scan '{}' has {} columns and {} rows. "
                                       "All selected scans must have the same number of columns and rows. Select scans with the same dimensions.",
                                       scan.getHDF5Path(), scan.getFileName(), scan.getXDimension(), scan.getYDimension(), first.getHDF5Path(), first.getXDimension(), first.getYDimension()));
  }
  const auto normalizedStep = [](auto value) {
    if constexpr(std::is_same_v<ReaderType, ebsdlib::H5EspritReader>)
    {
      return value == 0 ? decltype(value){1} : value;
    }
    return value;
  };
  if(normalizedStep(first.getXStep()) != normalizedStep(scan.getXStep()) || normalizedStep(first.getYStep()) != normalizedStep(scan.getYStep()))
  {
    return MakeErrorResult(k_ScanSpacingMismatch, fmt::format("{} have different spacing: X={}, Y={} and X={}, Y={}. Select scans with the same spacing.", context, first.getXStep(), first.getYStep(),
                                                              scan.getXStep(), scan.getYStep()));
  }
  const auto firstPhases = first.getPhaseVector();
  const auto scanPhases = scan.getPhaseVector();
  if(firstPhases.size() != scanPhases.size())
  {
    return MakeErrorResult(k_ScanPhaseCountMismatch, fmt::format("{} have different phase counts: {} and {}. Select scans with the same phase table.", context, firstPhases.size(), scanPhases.size()));
  }
  for(usize i = 0; i < firstPhases.size(); ++i)
  {
    const auto& a = firstPhases[i];
    const auto& b = scanPhases[i];
    if(a == nullptr || b == nullptr || a->getPhaseIndex() != b->getPhaseIndex() || a->determineOrientationOpsIndex() != b->determineOrientationOpsIndex() ||
       a->getMaterialName() != b->getMaterialName() || a->getLatticeConstants() != b->getLatticeConstants())
    {
      return MakeErrorResult(k_ScanPhaseMismatch,
                             fmt::format("{} have different phase table entries at position {} (IDs {} and {}, materials '{}' and '{}', orientation indices {} and {}, lattice constants [{}] and "
                                         "[{}]). Select scans with the same phase table.",
                                         context, i, a == nullptr ? -1 : a->getPhaseIndex(), b == nullptr ? -1 : b->getPhaseIndex(), a == nullptr ? "<missing>" : a->getMaterialName(),
                                         b == nullptr ? "<missing>" : b->getMaterialName(), a == nullptr ? -1 : static_cast<int32>(a->determineOrientationOpsIndex()),
                                         b == nullptr ? -1 : static_cast<int32>(b->determineOrientationOpsIndex()),
                                         a == nullptr ? "<missing>" : fmt::format("{}", fmt::join(a->getLatticeConstants(), ", ")),
                                         b == nullptr ? "<missing>" : fmt::format("{}", fmt::join(b->getLatticeConstants(), ", "))));
    }
  }
  if(readPatterns)
  {
    std::array<int32, 2> a = {}, b = {};
    first.getPatternDims(a);
    scan.getPatternDims(b);
    if(a != b)
    {
      return MakeErrorResult(k_ScanPatternMismatch,
                             fmt::format("{} have different pattern shapes: height={}, width={} and height={}, width={}. Select scans with the same pattern shape.", context, a[0], a[1], b[0], b[1]));
    }
  }
  return {};
}

/**
 * @brief Check pattern metadata before EbsdLib reads or reorders the pattern buffer.
 * @tparam ReaderType Esprit or OIM reader type.
 * @param reader Reader with a validated scan header.
 * @return Error if the dataset is unavailable or its type or shape does not match the header.
 *
 * This check reads metadata only. It does not allocate or read pattern values.
 */
template <typename ReaderType>
Result<> ValidateOemPatternDataset(ReaderType& reader)
{
  const std::string name = std::is_same_v<ReaderType, ebsdlib::H5EspritReader> ? ebsdlib::H5Esprit::RawPatterns : ebsdlib::Ang::PatternData;
  const std::string path = reader.getHDF5Path() + "/EBSD/Data/" + name;
  const auto context = fmt::format("File '{}' scan '{}' dataset '{}'", reader.getFileName(), reader.getHDF5Path(), path);
  const hid_t file = H5Support::H5Utilities::openFile(reader.getFileName(), true);
  const auto closeFile = MakeScopeGuard([&]() noexcept {
    if(file >= 0)
      H5Fclose(file);
  });
  const hid_t dataset = file < 0 ? -1 : H5Dopen2(file, path.c_str(), H5P_DEFAULT);
  if(dataset < 0)
  {
    return MakeErrorResult(k_PatternDatasetMissing, fmt::format("{} cannot be opened. Supply pattern data for every selected scan or turn off Import Pattern Data.", context));
  }
  const auto closeDataset = MakeScopeGuard([&]() noexcept { H5Dclose(dataset); });
  const hid_t type = H5Dget_type(dataset);
  const auto closeType = MakeScopeGuard([&]() noexcept {
    if(type >= 0)
      H5Tclose(type);
  });
  if(type < 0 || H5Tget_class(type) != H5T_INTEGER || H5Tget_size(type) != 1 || H5Tget_sign(type) != H5T_SGN_NONE)
  {
    return MakeErrorResult(k_PatternDatasetType, fmt::format("{} has type '{}', size {} bytes, sign '{}'. Expected unsigned 8-bit integer patterns.", context,
                                                             type < 0                          ? "unavailable" :
                                                             H5Tget_class(type) == H5T_INTEGER ? "integer" :
                                                             H5Tget_class(type) == H5T_FLOAT   ? "floating point" :
                                                                                                 "non-integer",
                                                             type < 0 ? 0 : H5Tget_size(type),
                                                             type >= 0 && H5Tget_class(type) == H5T_INTEGER ? (H5Tget_sign(type) == H5T_SGN_NONE ? "unsigned" : "signed") : "not applicable"));
  }
  const hid_t space = H5Dget_space(dataset);
  const auto closeSpace = MakeScopeGuard([&]() noexcept {
    if(space >= 0)
      H5Sclose(space);
  });
  const int32 rank = space < 0 ? -1 : H5Sget_simple_extent_ndims(space);
  if(rank != 3)
  {
    return MakeErrorResult(k_PatternDatasetRank, fmt::format("{} has rank {}. Expected rank 3: points, height, width.", context, rank));
  }
  std::array<hsize_t, 3> dims = {};
  std::array<int32, 2> patternDims = {};
  reader.getPatternDims(patternDims);
  const uint64 points = static_cast<uint64>(reader.getXDimension()) * static_cast<uint64>(reader.getYDimension());
  if(H5Sget_simple_extent_dims(space, dims.data(), nullptr) < 0 || dims[0] != points || dims[1] != static_cast<hsize_t>(patternDims[0]) || dims[2] != static_cast<hsize_t>(patternDims[1]))
  {
    return MakeErrorResult(k_PatternDatasetExtent, fmt::format("{} has points={}, height={}, width={}; expected points={}, height={}, width={}. Correct the pattern dataset to match its scan header.",
                                                               context, dims[0], dims[1], dims[2], points, patternDims[0], patternDims[1]));
  }
  return {};
}

/**
 * @brief Require Euler fields used by Channel5 conversion or hexagonal alignment.
 * @param reader Reader for the selected CPR file.
 * @param compatible Whether to create compatible arrays.
 * @param align Whether to apply hexagonal alignment.
 * @return Error if an active Euler field is missing or has the wrong type.
 */
inline Result<> ValidateChannel5Fields(ebsdlib::CprReader& reader, bool compatible, bool align)
{
  const auto parsers = reader.createFieldParsers(reader.getFileName());
  for(const auto& name : {ebsdlib::Ctf::phi1, ebsdlib::Ctf::Phi, ebsdlib::Ctf::phi2})
  {
    if(!compatible && (!align || name != ebsdlib::Ctf::phi2))
    {
      continue;
    }
    if(std::none_of(parsers.begin(), parsers.end(),
                    [&](const auto& parser) { return parser.FieldDefinition.FieldName == name && parser.FieldDefinition.numericType == ebsdlib::NumericTypes::Type::Float; }))
    {
      return MakeErrorResult(k_RequiredChannel5Field, fmt::format("File '{}' is missing required field '{}' with type '{}'. Supply this field or turn off compatible arrays and hexagonal alignment.",
                                                                  reader.getFileName(), name, "float32"));
    }
  }
  return {};
}

} // namespace EbsdReaderUtilities
