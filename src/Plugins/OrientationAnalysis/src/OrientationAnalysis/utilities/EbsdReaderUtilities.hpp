#pragma once

#include "OrientationAnalysis/utilities/GrainMapper3DUtilities.hpp"

#include "simplnx/Filter/IFilter.hpp"

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
#include <functional>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

using namespace nx::core;

namespace EbsdReaderUtilities
{
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
      if(phaseInfo.SpaceGroup < 1 || phaseInfo.SpaceGroup > 230 || ebsdlib::LaueOps::GetOrientationOpsFromSpaceGroupNumber(phaseInfo.SpaceGroup) == nullptr)
      {
        return MakeErrorResult(k_InvalidPhaseSymmetry,
                               fmt::format("File '{}' phase {} has invalid Space Group {}. Use a space group from 1 to 230.", filePath, phaseInfo.PhaseIndex, phaseInfo.SpaceGroup));
      }
      preflightUpdatedValues.push_back(
          {fmt::format("{}: ", phaseInfo.PhaseIndex), fmt::format("Material Name: {}    |    Crystal Symmetry: {}    |    Space Group: {}", phaseInfo.Name,
                                                                  ebsdlib::LaueOps::GetOrientationOpsFromSpaceGroupNumber(phaseInfo.SpaceGroup)->getSymmetryName(), phaseInfo.SpaceGroup)});
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

} // namespace EbsdReaderUtilities
