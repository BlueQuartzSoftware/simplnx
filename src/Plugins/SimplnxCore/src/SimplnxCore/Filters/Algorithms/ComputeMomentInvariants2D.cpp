#include "ComputeMomentInvariants2D.hpp"

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

using namespace nx::core;

class ComputeMomentInvariants2DImpl
{
public:
  ComputeMomentInvariants2DImpl(const Int32Array& featureIds, const UInt32Array& featureRect, Float32Array& omega1, Float32Array& omega2, Float32Array* centralMoments, const SizeVec3& volDims,
                                const bool normalizeMomentInvariants, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel)
  : m_FeatureIds(featureIds)
  , m_FeatureRect(featureRect)
  , m_Omega1(omega1)
  , m_Omega2(omega2)
  , m_CentralMoments(centralMoments)
  , m_VolDims(volDims)
  , m_NormalizeMomentInvariants(normalizeMomentInvariants)
  , m_ShouldCancel(shouldCancel)
  , m_MessageHandler(mesgHandler)
  {
  }

  void convert(usize start, usize end) const
  {
    const usize numRectComponents = m_FeatureRect.getNumberOfComponents();
    const auto numFeatures = static_cast<int32>(m_FeatureRect.getNumberOfTuples());
    for(int32 featureId = start; featureId < end; featureId++)
    {
      const auto featureIdRectIndex = featureId * numRectComponents;
      std::array<uint32, 6> corner = {m_FeatureRect[featureIdRectIndex],     m_FeatureRect[featureIdRectIndex + 1], m_FeatureRect[featureIdRectIndex + 2],
                                      m_FeatureRect[featureIdRectIndex + 3], m_FeatureRect[featureIdRectIndex + 4], m_FeatureRect[featureIdRectIndex + 5]};
      constexpr usize maxOrder = 2;

      // Figure the largest X || Y dimension so we can create a square matrix
      const uint32 xDim = corner[3] - corner[0] + 1;
      const uint32 yDim = corner[4] - corner[1] + 1;
      const uint32 zDim = corner[5] - corner[2] + 1;

      if(zDim != 1)
      {
        m_Omega1[featureId] = 0.0f;
        m_Omega2[featureId] = 0.0f;
        m_MessageHandler(IFilter::Message::Type::Info, fmt::format("[{}/{}] : Feature {} is NOT strictly 2D in the XY plane. Skipping this feature.", featureId, numFeatures, featureId));
        return;
      }

      usize dim = xDim; // Assume XDim is the largest value
      if(yDim > xDim)
      {
        dim = yDim; // Nope, YDim is largest
      }

      ComputeMomentInvariants2D::DoubleMatrixType input2D(dim, dim);
      input2D.setZero();

      uint32 height = 0;

      for(uint32_t y = corner[1]; y <= corner[4]; y++)
      {
        for(uint32_t x = corner[0]; x <= corner[3]; x++)
        {
          const usize index = (m_VolDims[1] * m_VolDims[0] * height) + (m_VolDims[0] * y) + x;
          if(m_FeatureIds[index] == featureId)
          {
            input2D(y - corner[1], x - corner[0]) = 1;
          }
          else
          {
            input2D(y - corner[1], x - corner[0]) = 0;
          }
        }
      }

      const usize inputDims[2] = {dim, dim};
      ComputeMomentInvariants2D::DoubleMatrixType m2D = ComputeMomentInvariants2D::ComputeMomentInvariants(input2D, inputDims, maxOrder);
      // compute the second order moment invariants
      double omega1 = 2.0 * (m2D(0, 0) * m2D(0, 0)) / (m2D(0, 2) + m2D(2, 0));
      double omega2 = std::pow(m2D(0, 0), 4) / (m2D(2, 0) * m2D(0, 2) - std::pow(m2D(1, 1), 2));

      if(m_NormalizeMomentInvariants)
      {
        // normalize the invariants by those of the circle
        constexpr double circleOmega[2] = {4.0 * numbers::pi, 16.0 * numbers::pi * numbers::pi};
        omega1 /= circleOmega[0];
        omega2 /= circleOmega[1];
      }
      m_Omega1[featureId] = static_cast<float>(omega1);
      m_Omega2[featureId] = static_cast<float>(omega2);

      if(m_CentralMoments != nullptr)
      {
        const double* m2DInternal = m2D.array().data();
        for(usize comp = 0; comp < 9; comp++)
        {
          (*m_CentralMoments)[static_cast<usize>(featureId) * 9UL + comp] = static_cast<float>(m2DInternal[comp]);
        }
      }

      m_MessageHandler(IFilter::Message::Type::Info, fmt::format("[{}/{}] : Completed", featureId, numFeatures));

      if(m_ShouldCancel)
      {
        return;
      }
    }
  }

  void operator()(const Range& range) const
  {
    convert(range.min(), range.max());
  }

private:
  const Int32Array& m_FeatureIds;
  const UInt32Array& m_FeatureRect;
  Float32Array& m_Omega1;
  Float32Array& m_Omega2;
  Float32Array* m_CentralMoments = nullptr;
  const SizeVec3& m_VolDims;
  const bool m_NormalizeMomentInvariants = true;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

// -----------------------------------------------------------------------------
ComputeMomentInvariants2D::ComputeMomentInvariants2D(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                     ComputeMomentInvariants2DInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeMomentInvariants2D::~ComputeMomentInvariants2D() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeMomentInvariants2D::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ComputeMomentInvariants2D::operator()()
{
  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  const SizeVec3 volDims = imageGeom.getDimensions();

  const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  const auto& featureRect = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->FeatureRectArrayPath);
  const auto numFeatures = static_cast<int32>(featureRect.getNumberOfTuples());
  const usize numRectComponents = featureRect.getNumberOfComponents();
  auto& omega1 = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->Omega1ArrayPath);
  auto& omega2 = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->Omega2ArrayPath);
  Float32Array* centralMoments = nullptr;
  if(m_InputValues->SaveCentralMoments)
  {
    centralMoments = m_DataStructure.getDataAs<Float32Array>(m_InputValues->CentralMomentsArrayPath);
  }

  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(1, numFeatures);
  dataAlg.setParallelizationEnabled(true);
  dataAlg.execute(ComputeMomentInvariants2DImpl(featureIds, featureRect, omega1, omega2, centralMoments, volDims, m_InputValues->NormalizeMomentInvariants, m_MessageHandler, m_ShouldCancel));

  return {};
}

// -----------------------------------------------------------------------------
ComputeMomentInvariants2D::DoubleMatrixType ComputeMomentInvariants2D::ComputeMomentInvariants(const DoubleMatrixType& input, const usize* inputDims, usize maxOrder)
{
  assert(inputDims[0] == inputDims[1]);
  const usize dim = inputDims[0];
  DoubleMatrixType bigX = GetBigX(maxOrder, inputDims[0]);

  const int mDim = static_cast<int>(maxOrder + 1);
  const double fNorm = static_cast<double>(dim - 1) / 2.0;

  // precompute the binomial coefficients for central moment conversion;  (could be hard-coded for maxOrder = 2)
  DoubleMatrixType bn = Binomial(maxOrder);

  DoubleMatrixType mnk(mDim, mDim);
  mnk.setZero();

  const DoubleMatrixType inter = input * bigX;

  mnk = bigX.transpose() * inter;

  for(int c = 0; c < mDim; c++)
  {
    for(int r = 0; r < mDim; r++)
    {
      mnk(r, c) *= std::pow(fNorm, (2 + c + r));
    }
  }

  // transform the moments to central moments using the binomial theorem
  // first get the center of mass coordinates (xc, yc)
  const double xc = mnk(1, 0) / mnk(0, 0); // mnk[0,0] is the area of the object in units of pixels
  const double yc = mnk(0, 1) / mnk(0, 0);

  // declare an intermediate array to hold the transformed moment values
  DoubleMatrixType mnkNew(mDim, mDim);
  mnkNew.setZero();

  // apply the binomial theorem
  for(int p = 0; p < mDim; p++)
  {
    for(int q = 0; q < mDim; q++)
    {
      for(int k = 0; k < p + 1; k++)
      {
        for(int l = 0; l < q + 1; l++)
        {
          mnkNew(p, q) += std::pow(-1.0, (p + q - k - l)) * std::pow(xc, (p - k)) * std::pow(yc, (q - l)) * bn(p, k) * bn(q, l) * mnk(k, l);
        }
      }
    }
  }

  return mnkNew;
}

// -----------------------------------------------------------------------------
int ComputeMomentInvariants2D::Factorial(int n)
{
  return (n == 1 || n == 0) ? 1 : Factorial(n - 1) * n;
}

// -----------------------------------------------------------------------------
ComputeMomentInvariants2D::DoubleMatrixType ComputeMomentInvariants2D::Binomial(usize maxOrder)
{
  const int dim = static_cast<int>(maxOrder + 1);
  DoubleMatrixType bn(dim, dim);
  bn.setZero();

  for(int i = 0; i < dim; i++)
  {
    for(int j = 0; j <= i; j++)
    {
      bn(i, j) = (Factorial(i)) / (Factorial(j)) / (Factorial(i - j));
      bn(j, i) = bn(i, j);
    }
  }
  return bn;
}

// -----------------------------------------------------------------------------
ComputeMomentInvariants2D::DoubleMatrixType ComputeMomentInvariants2D::GetBigX(usize maxOrder, usize dim)
{
  const int dRows = static_cast<int>(dim);
  const int dCols = static_cast<int>(dim + 1);

  DoubleMatrixType xx(1, dCols);
  for(int c = 0; c < dCols; c++)
  {
    xx(0, c) = c - static_cast<double>(dim) / 2.0 - 0.5;
  }

  const double fNorm = xx.maxCoeff();
  xx = xx / fNorm;

  DoubleMatrixType doubleMatrix(dRows, dCols);
  doubleMatrix.setZero();

  IntMatrixType j(1, dRows);
  for(int c = 0; c < dRows; c++)
  {
    j(0, c) = c;
  }

  for(int r = 0; r < dRows; r++)
  {
    doubleMatrix(r, r) = -1.0;
    doubleMatrix(r, r + 1) = 1.0;
  }

  // Set the Scale Factors
  DoubleMatrixType sc(1, maxOrder + 1);
  const int mop1 = static_cast<int>(maxOrder + 1);
  for(int c = 0; c < mop1; c++)
  {
    sc(0, c) = 1.0 / (c + 1.0);
  }

  DoubleMatrixType bigX(dim, maxOrder + 1);
  bigX.setZero();

  DoubleMatrixType yy;
  for(int i = 0; i < mop1; i++)
  {
    if(i == 0)
    {
      yy = xx;
    }
    else
    {
      yy = yy.cwiseProduct(xx);
    }

    DoubleMatrixType mm = yy * doubleMatrix.transpose();
    mm = mm * sc(0, i);
    bigX.col(i) = mm.row(0);
  }

  return bigX;
}
