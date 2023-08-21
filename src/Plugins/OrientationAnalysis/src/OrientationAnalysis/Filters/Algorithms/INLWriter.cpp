#include "INLWriter.hpp"

#include "complex/ComplexVersion.h"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/StringArray.hpp"
#include "complex/Utilities/FilterUtilities.hpp"

#include "EbsdLib/Core/EbsdLibConstants.h"
#include "EbsdLib/IO/TSL/AngConstants.h"

#include <fstream>

using namespace complex;

namespace
{
// -----------------------------------------------------------------------------
uint32 mapCrystalSymmetryToTslSymmetry(uint32 symmetry)
{
  switch(symmetry)
  {
  case EbsdLib::CrystalStructure::Cubic_High:
    return EbsdLib::Ang::PhaseSymmetry::Cubic;
  case EbsdLib::CrystalStructure::Cubic_Low:
    return EbsdLib::Ang::PhaseSymmetry::Tetrahedral;
  case EbsdLib::CrystalStructure::Tetragonal_High:
    return EbsdLib::Ang::PhaseSymmetry::DiTetragonal;
  case EbsdLib::CrystalStructure::Tetragonal_Low:
    return EbsdLib::Ang::PhaseSymmetry::Tetragonal;
  case EbsdLib::CrystalStructure::OrthoRhombic:
    return EbsdLib::Ang::PhaseSymmetry::Orthorhombic;
  case EbsdLib::CrystalStructure::Monoclinic:
    return EbsdLib::Ang::PhaseSymmetry::Monoclinic_c;
    // Not sure why these are here, but they were in the original filter and will never be reached so commented out
    //    return EbsdLib::Ang::PhaseSymmetry::Monoclinic_b;
    //    return EbsdLib::Ang::PhaseSymmetry::Monoclinic_a;
  case EbsdLib::CrystalStructure::Triclinic:
    return EbsdLib::Ang::PhaseSymmetry::Triclinic;
  case EbsdLib::CrystalStructure::Hexagonal_High:
    return EbsdLib::Ang::PhaseSymmetry::DiHexagonal;
  case EbsdLib::CrystalStructure::Hexagonal_Low:
    return EbsdLib::Ang::PhaseSymmetry::Hexagonal;
  case EbsdLib::CrystalStructure::Trigonal_High:
    return EbsdLib::Ang::PhaseSymmetry::DiTrigonal;
  case EbsdLib::CrystalStructure::Trigonal_Low:
    return EbsdLib::Ang::PhaseSymmetry::Trigonal;
  default:
    return EbsdLib::CrystalStructure::UnknownCrystalStructure;
  }
}
} // namespace

// -----------------------------------------------------------------------------
INLWriter::INLWriter(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, INLWriterInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
INLWriter::~INLWriter() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& INLWriter::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> INLWriter::operator()()
{
  // Make sure any directory path is also available as the user may have just typed
  // in a path without actually creating the full path
  Result<> createDirectoriesResult = complex::CreateOutputDirectories(m_InputValues->OutputFile.parent_path());
  if(createDirectoriesResult.invalid())
  {
    return createDirectoriesResult;
  }

  // Make sure any directory path is also available as the user may have just typed
  // in a path without actually creating the full path
  std::ofstream fout(m_InputValues->OutputFile, std::ios_base::out | std::ios_base::binary);
  if(!fout.is_open())
  {
    return MakeErrorResult(-74100, fmt::format("Error creating and opening output file at path: {}", m_InputValues->OutputFile.string()));
  }

  auto imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeomPath);

  auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  auto& eulerAngles = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CellEulerAnglesArrayPath);
  auto& cellPhases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellPhasesArrayPath);
  auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);
  auto& numFeatures = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->NumFeaturesArrayPath);
  auto& materialNames = m_DataStructure.getDataRefAs<StringArray>(m_InputValues->MaterialNameArrayPath);

  usize totalPoints = featureIds.getNumberOfTuples();

  SizeVec3 dims = imageGeom.getDimensions();
  FloatVec3 res = imageGeom.getSpacing();
  FloatVec3 origin = imageGeom.getOrigin();

  // Write the header, Each line starts with a "#" symbol
  fout << "# File written from " << COMPLEX_PACKAGE_COMPLETE << "\n";
  fout << "# X_STEP: " << std::fixed << res[0] << "\n";
  fout << "# Y_STEP: " << std::fixed << res[1] << "\n";
  fout << "# Z_STEP: " << std::fixed << res[2] << "\n";
  fout << "#\n";
  fout << "# X_MIN: " << std::fixed << origin[0] << "\n";
  fout << "# Y_MIN: " << std::fixed << origin[1] << "\n";
  fout << "# Z_MIN: " << std::fixed << origin[2] << "\n";
  fout << "#\n";
  fout << "# X_MAX: " << std::fixed << origin[0] + (static_cast<float64>(dims[0]) * res[0]) << "\n";
  fout << "# Y_MAX: " << std::fixed << origin[1] + (static_cast<float64>(dims[1]) * res[1]) << "\n";
  fout << "# Z_MAX: " << std::fixed << origin[2] + (static_cast<float64>(dims[2]) * res[2]) << "\n";
  fout << "#\n";
  fout << "# X_DIM: " << dims[0] << "\n";
  fout << "# Y_DIM: " << dims[1] << "\n";
  fout << "# Z_DIM: " << dims[2] << "\n";
  fout << "#\n";

  /*
   * -------------------------------------------- -
   * #Phase_1 : MOX with 30 % Pu
   * #Symmetry_1 : 43
   * #Features_1 : 4
   * #
   * #Phase_2 : Brahman
   * #Symmetry_2 : 62
   * #Features_2 : 6
   * #
   * #Phase_3 : Void
   * #Symmetry_3 : 22
   * #Features_3 : 1
   * #
   * #Total_Features : 11
   * -------------------------------------------- -
   */

  auto count = static_cast<int32>(materialNames.getNumberOfTuples());
  for(uint32 i = 1; i < count; ++i)
  {
    fout << "# Phase_" << i << ": " << materialNames[i].c_str() << "\n";
    fout << "# Symmetry_" << i << ": " << mapCrystalSymmetryToTslSymmetry(crystalStructures[i]) << "\n";
    fout << "# Features_" << i << ": " << numFeatures[i] << "\n";
    fout << "#\n";
  }

  std::set<int32> uniqueFeatureIds;
  for(usize i = 0; i < totalPoints; ++i)
  {
    uniqueFeatureIds.insert(featureIds[i]);
  }
  count = static_cast<int32_t>(uniqueFeatureIds.size());
  fout << "# Num_Features: " << count << " \n";
  fout << "#\n";

  fout << "# phi1 PHI phi2 x y z FeatureId PhaseId Symmetry\n";

  for(usize z = 0; z < dims[2]; ++z)
  {
    for(usize y = 0; y < dims[1]; ++y)
    {
      for(usize x = 0; x < dims[0]; ++x)
      {
        usize index = (z * dims[0] * dims[1]) + (dims[0] * y) + x;
        float32 phi1 = eulerAngles[index * 3];
        float32 phi = eulerAngles[index * 3 + 1];
        float32 phi2 = eulerAngles[index * 3 + 2];
        float64 xPos = origin[0] + (static_cast<float64>(x) * res[0]);
        float64 yPos = origin[1] + (static_cast<float64>(y) * res[1]);
        float64 zPos = origin[2] + (static_cast<float64>(z) * res[2]);
        int32 phaseId = cellPhases[index];
        uint32 symmetry = crystalStructures[phaseId];
        if(phaseId > 0)
        {
          if(symmetry == EbsdLib::CrystalStructure::Cubic_High)
          {
            symmetry = EbsdLib::Ang::PhaseSymmetry::Cubic;
          }
          else if(symmetry == EbsdLib::CrystalStructure::Hexagonal_High)
          {
            symmetry = EbsdLib::Ang::PhaseSymmetry::DiHexagonal;
          }
          else
          {
            symmetry = EbsdLib::Ang::PhaseSymmetry::UnknownSymmetry;
          }
        }
        else
        {
          symmetry = EbsdLib::Ang::PhaseSymmetry::UnknownSymmetry;
        }

        fout << std::fixed << phi1 << " " << phi << " " << phi2 << " " << xPos << " " << yPos << " " << zPos << " " << featureIds[index] << " " << phaseId << " " << symmetry << "\n";
      }
    }
  }

  return {};
}
