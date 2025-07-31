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

#include <concepts>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

using namespace nx::core;

namespace EbsdReaderUtilities
{
// 1. Base template (empty)
template <typename ReaderType>
struct ReaderTraits;

// 2. Specialization for AngReader
template <>
struct ReaderTraits<AngReader>
{
  static auto getCols()
  {
    return std::mem_fn(&AngReader::getNumEvenCols);
  }
  static auto getRows()
  {
    return std::mem_fn(&AngReader::getNumRows);
  }
  static auto getXSpacing()
  {
    return std::mem_fn(&AngReader::getXStep);
  }
  static auto getYSpacing()
  {
    return std::mem_fn(&AngReader::getYStep);
  }
};

template <>
struct ReaderTraits<H5OIMReader> : public ReaderTraits<AngReader>
{
};

// 2. Specialization for CtfReader
template <>
struct ReaderTraits<CtfReader>
{
  static auto getCols()
  {
    return std::mem_fn(&CtfReader::getXCells);
  }
  static auto getRows()
  {
    return std::mem_fn(&CtfReader::getYCells);
  }
  static auto getXSpacing()
  {
    return std::mem_fn(&CtfReader::getXStep);
  }
  static auto getYSpacing()
  {
    return std::mem_fn(&CtfReader::getYStep);
  }
};

template <>
struct ReaderTraits<H5OINAReader>
{
  static auto getCols()
  {
    return std::mem_fn(&H5OINAReader::getXCells);
  }
  static auto getRows()
  {
    return std::mem_fn(&H5OINAReader::getYCells);
  }
  static auto getXSpacing()
  {
    return std::mem_fn(&H5OINAReader::getXStep);
  }
  static auto getYSpacing()
  {
    return std::mem_fn(&H5OINAReader::getYStep);
  }
};

template <>
struct ReaderTraits<CprReader> : public ReaderTraits<CtfReader>
{
  static auto getCols()
  {
    return std::mem_fn(&CprReader::getXCells);
  }
  static auto getRows()
  {
    return std::mem_fn(&CprReader::getYCells);
  }
  static auto getXSpacing()
  {
    return std::mem_fn(&CprReader::getXStep);
  }
  static auto getYSpacing()
  {
    return std::mem_fn(&CprReader::getYStep);
  }
};

template <>
struct ReaderTraits<H5EspritReader>
{
  static auto getCols()
  {
    return std::mem_fn(&H5EspritReader::getNumColumns);
  }
  static auto getRows()
  {
    return std::mem_fn(&H5EspritReader::getNumRows);
  }
  static auto getXSpacing()
  {
    return std::mem_fn(&H5EspritReader::getXStep);
  }
  static auto getYSpacing()
  {
    return std::mem_fn(&H5EspritReader::getYStep);
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
struct ReaderTraits<H5AngVolumeReader>
{
  static auto getCols()
  {
    return std::mem_fn(&H5AngVolumeReader::getNumEvenCols);
  }
  static auto getRows()
  {
    return std::mem_fn(&H5AngVolumeReader::getNumRows);
  }
  static auto getXSpacing()
  {
    return std::mem_fn(&H5AngVolumeReader::getXStep);
  }
  static auto getYSpacing()
  {
    return std::mem_fn(&H5AngVolumeReader::getYStep);
  }
};

template <>
struct ReaderTraits<H5CtfVolumeReader>
{
  static auto getCols()
  {
    return std::mem_fn(&H5CtfVolumeReader::getXCells);
  }
  static auto getRows()
  {
    return std::mem_fn(&H5CtfVolumeReader::getYCells);
  }
  static auto getXSpacing()
  {
    return std::mem_fn(&H5CtfVolumeReader::getXStep);
  }
  static auto getYSpacing()
  {
    return std::mem_fn(&H5CtfVolumeReader::getYStep);
  }
};

template <typename ReaderType>
void GeneratePreflightScanInformation(ReaderType& reader, std::vector<IFilter::PreflightValue>& preflightUpdatedValues)
{
  auto colCount = ReaderTraits<ReaderType>::getCols()(reader);
  auto rowCount = ReaderTraits<ReaderType>::getRows()(reader);
  auto xStep = ReaderTraits<ReaderType>::getXSpacing()(reader);
  auto yStep = ReaderTraits<ReaderType>::getYSpacing()(reader);

  std::stringstream ss;

  if constexpr(std::is_same_v<ReaderType, AngReader>)
  {
    ss << "Grid: " << reader.getGrid() << "\n";
  }

  ss << "X Step: " << xStep << "    Y Step: " << yStep << "\n";
  if constexpr(std::is_same_v<ReaderType, AngReader>)
  {
    ss << "Num Odd Cols: " << reader.getNumOddCols() << "    ";
    ss << "Num Even Cols: " << reader.getNumEvenCols() << "    ";
  }
  else
  {
    ss << "Num Cols: " << colCount << "    ";
  }

  ss << "Num Rows: " << rowCount << "\n"
     << "Sample Physical Dimensions: " << (xStep * colCount) << " (W) x " << (yStep * rowCount) << " (H) microns"
     << "\n";

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
  auto laueOps = LaueOps::GetAllOrientationOps();
  int phaseIndex = 1;
  for(const auto& phaseInfo : phaseInfos)
  {

    if constexpr(std::is_same_v<ReaderType, AngReader> || std::is_same_v<ReaderType, H5OIMReader> || std::is_same_v<ReaderType, H5AngVolumeReader>)
    {
      preflightUpdatedValues.push_back({fmt::format("{}: ", phaseIndex++), fmt::format("Material Name: {}    |    Formula: {}    |    Crystal Symmetry: {}", phaseInfo->getMaterialName(),
                                                                                       phaseInfo->getFormula(), laueOps[phaseInfo->determineOrientationOpsIndex()]->getSymmetryName())});
    }

    if constexpr(std::is_same_v<ReaderType, CtfReader> || std::is_same_v<ReaderType, H5OINAReader> || std::is_same_v<ReaderType, CprReader> || std::is_same_v<ReaderType, H5CtfVolumeReader>)
    {
      preflightUpdatedValues.push_back({fmt::format("{}: ", phaseIndex++), fmt::format("Material Name: {}    |    Crystal Symmetry: {}    |    Comment: {}", phaseInfo->getMaterialName(),
                                                                                       laueOps[phaseInfo->determineOrientationOpsIndex()]->getSymmetryName(), phaseInfo->getComment())});
    }

    if constexpr(std::is_same_v<ReaderType, H5EspritReader>)
    {
      preflightUpdatedValues.push_back({fmt::format("{}: ", phaseIndex++), fmt::format("Material Name: {}    |    Crystal Symmetry: {}    |    Space Group: {}", phaseInfo->getMaterialName(),
                                                                                       laueOps[phaseInfo->determineOrientationOpsIndex()]->getSymmetryName(), phaseInfo->getSpaceGroup())});
    }

    if constexpr(std::is_same_v<ReaderType, GrainMapper3DUtilities::GrainMapperReader>)
    {
      preflightUpdatedValues.push_back({fmt::format("{}: ", phaseIndex++), fmt::format("Material Name: {}    |    Crystal Symmetry: {}    |    Space Group: {}", phaseInfo.Name,
                                                                                       phaseInfo.UniversalHermannMauguin, phaseInfo.SpaceGroup)});
    }
  }
}

} // namespace EbsdReaderUtilities
