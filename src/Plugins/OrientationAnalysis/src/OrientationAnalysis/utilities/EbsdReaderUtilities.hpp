#pragma once

#include <optional>

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

#include <concepts>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

using namespace nx::core;

namespace ebsd_reader_utilities
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
constexpr bool IsHexagonalCrystalStructure(uint32 crystalStructure)
{
  return crystalStructure == ebsdlib::CrystalStructure::Hexagonal_Low || crystalStructure == ebsdlib::CrystalStructure::Hexagonal_High;
}

// 1. Base template (empty)
template <typename ReaderType>
struct ReaderTraits;

// 2. Specialization for AngReader
template <>
struct ReaderTraits<ebsdlib::AngReader>
{
  static auto GetCols()
  {
    return std::mem_fn(&ebsdlib::AngReader::getNumEvenCols);
  }
  static auto GetRows()
  {
    return std::mem_fn(&ebsdlib::AngReader::getNumRows);
  }
  static auto GetXSpacing()
  {
    return std::mem_fn(&ebsdlib::AngReader::getXStep);
  }
  static auto GetYSpacing()
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
  static auto GetCols()
  {
    return std::mem_fn(&ebsdlib::CtfReader::getXCells);
  }
  static auto GetRows()
  {
    return std::mem_fn(&ebsdlib::CtfReader::getYCells);
  }
  static auto GetXSpacing()
  {
    return std::mem_fn(&ebsdlib::CtfReader::getXStep);
  }
  static auto GetYSpacing()
  {
    return std::mem_fn(&ebsdlib::CtfReader::getYStep);
  }
};

template <>
struct ReaderTraits<ebsdlib::H5OINAReader>
{
  static auto GetCols()
  {
    return std::mem_fn(&ebsdlib::H5OINAReader::getXCells);
  }
  static auto GetRows()
  {
    return std::mem_fn(&ebsdlib::H5OINAReader::getYCells);
  }
  static auto GetXSpacing()
  {
    return std::mem_fn(&ebsdlib::H5OINAReader::getXStep);
  }
  static auto GetYSpacing()
  {
    return std::mem_fn(&ebsdlib::H5OINAReader::getYStep);
  }
};

template <>
struct ReaderTraits<ebsdlib::CprReader> : public ReaderTraits<ebsdlib::CtfReader>
{
  static auto GetCols()
  {
    return std::mem_fn(&ebsdlib::CprReader::getXCells);
  }
  static auto GetRows()
  {
    return std::mem_fn(&ebsdlib::CprReader::getYCells);
  }
  static auto GetXSpacing()
  {
    return std::mem_fn(&ebsdlib::CprReader::getXStep);
  }
  static auto GetYSpacing()
  {
    return std::mem_fn(&ebsdlib::CprReader::getYStep);
  }
};

template <>
struct ReaderTraits<ebsdlib::H5EspritReader>
{
  static auto GetCols()
  {
    return std::mem_fn(&ebsdlib::H5EspritReader::getNumColumns);
  }
  static auto GetRows()
  {
    return std::mem_fn(&ebsdlib::H5EspritReader::getNumRows);
  }
  static auto GetXSpacing()
  {
    return std::mem_fn(&ebsdlib::H5EspritReader::getXStep);
  }
  static auto GetYSpacing()
  {
    return std::mem_fn(&ebsdlib::H5EspritReader::getYStep);
  }
};

template <>
struct ReaderTraits<grain_mapper_3d_utilities::GrainMapperReader>
{
  static auto GetCols()
  {
    return std::mem_fn(&grain_mapper_3d_utilities::GrainMapperReader::getNumColumns);
  }
  static auto GetRows()
  {
    return std::mem_fn(&grain_mapper_3d_utilities::GrainMapperReader::getNumRows);
  }
  static auto GetXSpacing()
  {
    return std::mem_fn(&grain_mapper_3d_utilities::GrainMapperReader::getXStep);
  }
  static auto GetYSpacing()
  {
    return std::mem_fn(&grain_mapper_3d_utilities::GrainMapperReader::getYStep);
  }
};

template <>
struct ReaderTraits<ebsdlib::H5AngVolumeReader>
{
  static auto GetCols()
  {
    return std::mem_fn(&ebsdlib::H5AngVolumeReader::getNumEvenCols);
  }
  static auto GetRows()
  {
    return std::mem_fn(&ebsdlib::H5AngVolumeReader::getNumRows);
  }
  static auto GetXSpacing()
  {
    return std::mem_fn(&ebsdlib::H5AngVolumeReader::getXStep);
  }
  static auto GetYSpacing()
  {
    return std::mem_fn(&ebsdlib::H5AngVolumeReader::getYStep);
  }
};

template <>
struct ReaderTraits<ebsdlib::H5CtfVolumeReader>
{
  static auto GetCols()
  {
    return std::mem_fn(&ebsdlib::H5CtfVolumeReader::getXCells);
  }
  static auto GetRows()
  {
    return std::mem_fn(&ebsdlib::H5CtfVolumeReader::getYCells);
  }
  static auto GetXSpacing()
  {
    return std::mem_fn(&ebsdlib::H5CtfVolumeReader::getXStep);
  }
  static auto GetYSpacing()
  {
    return std::mem_fn(&ebsdlib::H5CtfVolumeReader::getYStep);
  }
};

template <typename ReaderType>
void GeneratePreflightScanInformation(ReaderType& reader, std::vector<IFilter::PreflightValue>& preflightUpdatedValues, const std::optional<FloatVec3Type>& spacing = {})
{
  auto colCount = ReaderTraits<ReaderType>::GetCols()(reader);
  auto rowCount = ReaderTraits<ReaderType>::GetRows()(reader);
  const auto xStep = spacing ? (*spacing)[0] : ReaderTraits<ReaderType>::GetXSpacing()(reader);
  const auto yStep = spacing ? (*spacing)[1] : ReaderTraits<ReaderType>::GetYSpacing()(reader);

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
      ss << "Num Slices (Z): " << reader.getZCells() << "    Z Step: " << (spacing ? (*spacing)[2] : reader.getZStep()) << "\n";
    }
  }

  preflightUpdatedValues.push_back({"Scan Information", ss.str()});
}

template <typename ReaderType>
void GeneratePreflightPhaseInformation(ReaderType& reader, std::vector<IFilter::PreflightValue>& preflightUpdatedValues)
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

    if constexpr(std::is_same_v<ReaderType, grain_mapper_3d_utilities::GrainMapperReader>)
    {
      const auto laueOpPtr = ebsdlib::LaueOps::GetLaueOpsFromSpaceGroupNumber(phaseInfo.space_group);
      preflightUpdatedValues.push_back({fmt::format("{}: ", phaseInfo.phase_index), fmt::format("Material Name: {}    |    Crystal Symmetry: {}    |    Space Group: {}", phaseInfo.name,
                                                                                                laueOpPtr != nullptr ? laueOpPtr->getSymmetryName() : "Unknown", phaseInfo.space_group)});
    }
  }
}

} // namespace ebsd_reader_utilities
