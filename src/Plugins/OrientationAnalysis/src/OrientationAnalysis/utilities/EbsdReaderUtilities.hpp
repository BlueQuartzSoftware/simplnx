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

    if constexpr(std::is_same_v<ReaderType, GrainMapper3DUtilities::GrainMapperReader>)
    {
      preflightUpdatedValues.push_back(
          {fmt::format("{}: ", phaseInfo.PhaseIndex), fmt::format("Material Name: {}    |    Crystal Symmetry: {}    |    Space Group: {}", phaseInfo.Name,
                                                                  ebsdlib::LaueOps::GetOrientationOpsFromSpaceGroupNumber(phaseInfo.SpaceGroup)->getSymmetryName(), phaseInfo.SpaceGroup)});
    }
  }
}

} // namespace EbsdReaderUtilities
