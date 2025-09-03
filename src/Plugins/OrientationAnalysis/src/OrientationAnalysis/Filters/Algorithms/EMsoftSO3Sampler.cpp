#include "EMsoftSO3Sampler.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"

#include "EbsdLib/Core/EbsdLibConstants.h"
#include "EbsdLib/Core/Orientation.hpp"
#include "EbsdLib/IO/TSL/AngConstants.h"
#include "EbsdLib/LaueOps/LaueOps.h"
#include "EbsdLib/Orientation/Cubochoric.hpp"
#include "EbsdLib/Orientation/Rodrigues.hpp"

using namespace nx::core;

namespace
{
template <class ContainerType, typename T>
ebsdlib::Rodrigues<T> RodriguesComposition(const ContainerType& sigma, const ebsdlib::Rodrigues<T>& rod)
{
  ContainerType rho(3);
  ContainerType rhomis(3);
  rho[0] = -rod[0] * rod[3];
  rho[1] = -rod[1] * rod[3];
  rho[2] = -rod[2] * rod[3];

  // perform the Rodrigues rotation composition with sigma to get rhomis
  double denom = 1.0 + (sigma[0] * rho[0] + sigma[1] * rho[1] + sigma[2] * rho[2]);
  if(denom == 0.0)
  {
    double len = sqrt(sigma[0] * sigma[0] + sigma[1] * sigma[1] + sigma[2] * sigma[2]);
    return {sigma[0] / len, sigma[1] / len, sigma[2] / len, std::numeric_limits<double>::infinity()};
  }

  rhomis[0] = (rho[0] - sigma[0] + (rho[1] * sigma[2] - rho[2] * sigma[1])) / denom;
  rhomis[1] = (rho[1] - sigma[1] + (rho[2] * sigma[0] - rho[0] * sigma[2])) / denom;
  rhomis[2] = (rho[2] - sigma[2] + (rho[0] * sigma[1] - rho[1] * sigma[0])) / denom;
  // revert rhomis to a four-component Rodrigues vector
  double len = sqrt(rhomis[0] * rhomis[0] + rhomis[1] * rhomis[1] + rhomis[2] * rhomis[2]);
  if(len != 0.0)
  {
    return {-rhomis[0] / len, -rhomis[1] / len, -rhomis[2] / len, len};
  }

  return {0.0, 0.0, 0.0, 0.0};
}
} // namespace

// -----------------------------------------------------------------------------
EMsoftSO3Sampler::EMsoftSO3Sampler(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, EMsoftSO3SamplerInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
EMsoftSO3Sampler::~EMsoftSO3Sampler() noexcept = default;

// -----------------------------------------------------------------------------
Result<> EMsoftSO3Sampler::operator()()
{
  typedef std::list<ebsdlib::RodriguesDType> OrientationListArrayType;

  OrientationListArrayType FZlist;

  if(m_InputValues->sampleModeSelector == orientation_sampling::k_FZModeIndex)
  {
    // here we perform the actual calculation; once we have the FZlist,
    // we can allocate the data array and copy all entries
    double x, y, z, delta;

    // step size for sampling of grid; maximum total number of samples = pow(2*m_InputValues->Numsp+1,3)
    delta = (0.50 * LPs::ap) / static_cast<double>(m_InputValues->Numsp);

    // do we need to shift this array away from the origin?
    double gridShift = 0.0;
    if(m_InputValues->OffsetGrid)
    {
      gridShift = 0.5;
    }

    std::vector<ebsdlib::LaueOps::Pointer> ops = ebsdlib::LaueOps::GetAllOrientationOps();
    // determine which function we should call for this point group symmetry
    ebsdlib::LaueOps::FZType fzType = ops[m_InputValues->CrystalStructureIndex]->getFZType();
    ebsdlib::LaueOps::AxisOrderingType fzAxisOrder = ops[m_InputValues->CrystalStructureIndex]->getAxisOrderingType();

    // loop over the cube of volume pi^2; note that we do not want to include
    // the opposite edges/facets of the cube, to avoid double counting rotations
    // with a rotation angle of 180 degrees.  This only affects the cyclic groups.
    int Np = m_InputValues->Numsp;
    int Totp = (2 * Np + 1) * (2 * Np + 1) * (2 * Np + 1);
    int Dn = Totp / 10;
    int Dc = Dn;
    int Di = 0;
    int Dg = 0;

    // eliminate points for which any of the coordinates lies outside the cube with semi-edge length "edge"
    double edge = 0.5 * LPs::ap;

    for(int i = -Np + 1; i < Np + 1; i++)
    {
      x = (static_cast<double>(i) + gridShift) * delta;

      if(fabs(x) <= edge)
      {

        for(int j = -Np + 1; j < Np + 1; j++)
        {
          y = (static_cast<double>(j) + gridShift) * delta;

          if(fabs(y) <= edge)
          {

            for(int k = -Np + 1; k < Np + 1; k++)
            {
              z = (static_cast<double>(k) + gridShift) * delta;

              if(fabs(z) <= edge)
              {
                // convert to Rodrigues representation
                ebsdlib::RodriguesDType rod = ebsdlib::CubochoricDType(x, y, z).toRodrigues();
                // If insideFZ=true, then add this point to FZlist
                if(ebsdlib::LaueOps::IsInsideFZ(rod, fzType, fzAxisOrder))
                {
                  FZlist.push_back(rod);
                  Dg += 1;
                }
                Di += 1;
              }
            }
          }
        }
      }

      // report on status of computation
      if(Di > Dc)
      {
        m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Euler Angles | Tested: {} of {} | Inside RFZ: {} ", Di, Totp, Dg));
        Dc += Dn;
      }
      if(m_ShouldCancel)
      {
        break;
      }
    }
  }

  // here are the misorientation sampling cases:
  if(m_InputValues->sampleModeSelector != 0)
  {
    // here we perform the actual calculation; once we have the FZlist,
    // we can allocate the data array and copy all entries
    double x, y, z, delta, omega, semi;

    // step size for sampling of grid; the edge length of the cube is (pi ( w - sin(w) ))^1/3 with w the misorientation angle
    omega = m_InputValues->MisOr * Constants::k_PiOver180D;
    semi = pow(Constants::k_PiD * (omega - sin(omega)), 1.0 / 3.0) * 0.5;
    delta = semi / static_cast<double>(m_InputValues->Numsp);

    // convert the reference orientation to a 3-component Rodrigues vector sigma
    ebsdlib::RodriguesDType sigm =
        ebsdlib::EulerDType(m_InputValues->RefOr[0] * Constants::k_PiOver180D, m_InputValues->RefOr[1] * Constants::k_PiOver180D, m_InputValues->RefOr[2] * Constants::k_PiOver180D).toRodrigues();

    std::vector<double> sigma = {sigm[0] * sigm[3], sigm[1] * sigm[3], sigm[2] * sigm[3]};

    if(m_InputValues->sampleModeSelector == 1)
    {
      // set counter parameters for the loop over the sub-cube surface
      int Np = m_InputValues->Numsp;
      int Totp = 24 * Np * Np + 2;
      int Dn = Totp / 20;
      int Dc = Dn;
      int Dg = 0;

      // x-y bottom and top planes
      for(int i = -Np; i <= Np; i++)
      {
        x = static_cast<double>(i) * delta;
        for(int j = -Np; j <= Np; j++)
        {
          y = static_cast<double>(j) * delta;
          // convert to Rodrigues representation and apply Rodrigues composition formula
          {
            ebsdlib::RodriguesDType rod = ebsdlib::CubochoricDType(-x, -y, -semi).toRodrigues();
            rod = RodriguesComposition(sigma, rod);
            FZlist.push_back(rod);
            Dg += 1;
          }
          {
            ebsdlib::RodriguesDType rod = ebsdlib::CubochoricDType(-x, -y, semi).toRodrigues();
            rod = RodriguesComposition(sigma, rod);
            FZlist.push_back(rod);
            Dg += 1;
          }
        }
        if(m_ShouldCancel)
        {
          break;
        }
      }
      // y-z  planes
      for(int j = -Np; j <= Np; j++)
      {
        y = static_cast<double>(j) * delta;
        for(int k = -Np + 1; k <= Np - 1; k++)
        {
          z = static_cast<double>(k) * delta;
          // convert to Rodrigues representation and apply Rodrigues composition formula
          {
            ebsdlib::RodriguesDType rod = ebsdlib::CubochoricDType(-semi, -y, -z).toRodrigues();
            rod = RodriguesComposition(sigma, rod);
            FZlist.push_back(rod);
            Dg += 1;
          }
          {
            ebsdlib::RodriguesDType rod = ebsdlib::CubochoricDType(semi, -y, -z).toRodrigues();
            rod = RodriguesComposition(sigma, rod);
            FZlist.push_back(rod);
            Dg += 1;
          }
        }
        if(m_ShouldCancel)
        {
          break;
        }
      }
      // finally the x-z  planes
      for(int i = -Np + 1; i <= Np - 1; i++)
      {
        x = static_cast<double>(i) * delta;
        for(int k = -Np + 1; k <= Np - 1; k++)
        {
          z = static_cast<double>(k) * delta;
          // convert to Rodrigues representation and apply Rodrigues composition formula
          {
            ebsdlib::RodriguesDType rod = ebsdlib::CubochoricDType(-x, -semi, -z).toRodrigues();
            rod = RodriguesComposition(sigma, rod);
            FZlist.push_back(rod);
            Dg += 1;
          }
          {
            ebsdlib::RodriguesDType rod = ebsdlib::CubochoricDType(-x, semi, -z).toRodrigues();
            rod = RodriguesComposition(sigma, rod);
            FZlist.push_back(rod);
            Dg += 1;
          }
        }
        if(m_ShouldCancel)
        {
          break;
        }
      }

      // report on status of computation
      if(Dg > Dc)
      {
        m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Euler Angles | Generated: {} / {}", Dg, Totp));
        Dc += Dn;
      }
    }
    else
    {
      // set counter parameters for the loop over the sub-cube surface
      int Np = m_InputValues->Numsp;
      int Totp = (2 * Np + 1) * (2 * Np + 1) * (2 * Np + 1); // see misorientation sampling paper for this expression
      int Dn = Totp / 20;
      int Dc = Dn;
      int Dg = 0;

      for(int i = -Np; i <= Np; i++)
      {
        x = static_cast<double>(i) * delta;
        for(int j = -Np; j <= Np; j++)
        {
          y = static_cast<double>(j) * delta;
          for(int k = -Np; k <= Np; k++)
          {
            z = static_cast<double>(k) * delta;
            // convert to Rodrigues representation and apply Rodrigues composition formula
            {
              ebsdlib::RodriguesDType rod = ebsdlib::CubochoricDType(-x, -y, -z).toRodrigues();
              rod = RodriguesComposition(sigma, rod);
              FZlist.push_back(rod);
              Dg += 1;
            }
          }
        }
        // report on status of computation
        if(Dg > Dc)
        {
          m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Euler Angles | Generated: {} / {}", Dg, Totp));
          Dc += Dn;
        }
      }
    }
  }

  // resize the EulerAngles array to the number of items in FZlist; don't forget to redefine the hard pointer
  std::vector<size_t> tDims = {FZlist.size()};

  auto& m_EulerAngles = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->EulerAnglesArrayName);
  m_EulerAngles.resizeTuples(tDims);
  auto& m_EulerAnglesStoreRef = m_EulerAngles.getDataStoreRef();
  // copy the Rodrigues vectors as Euler angles into the m_EulerAngles array; convert doubles to floats along the way
  int j = -1;
  for(const ebsdlib::RodriguesDType& rod : FZlist)
  {
    j += 1;
    ebsdlib::EulerDType eu = rod.toEuler();

    m_EulerAnglesStoreRef.setValue(j * 3 + 0, static_cast<float>(eu[0]));
    m_EulerAnglesStoreRef.setValue(j * 3 + 1, static_cast<float>(eu[1]));
    m_EulerAnglesStoreRef.setValue(j * 3 + 2, static_cast<float>(eu[2]));
  }

  auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->EnsembleAttrMatrixPath.createChildPath(ebsdlib::AngFile::CrystalStructures));

  auto& materialNames = m_DataStructure.getDataRefAs<StringArray>(m_InputValues->EnsembleAttrMatrixPath.createChildPath(ebsdlib::AngFile::MaterialName));
  const std::string k_InvalidPhase = "Invalid Phase";

  // Initialize the zero'th element to unknowns.
  crystalStructures.setValue(0, ebsdlib::CrystalStructure::UnknownCrystalStructure);
  materialNames.setValue(0, k_InvalidPhase);

  crystalStructures.setValue(1, m_InputValues->CrystalStructureIndex);
  materialNames.setValue(1, "EMSoftSO3Sampler");

  return {};
}
