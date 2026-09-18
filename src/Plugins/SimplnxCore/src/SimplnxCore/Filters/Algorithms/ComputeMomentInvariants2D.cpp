#include "ComputeMomentInvariants2D.hpp"

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <algorithm>
#include <array>
#include <memory>
#include <nonstd/span.hpp>

using namespace nx::core;

namespace
{
/**
 * @brief Calculates a nonnegative integer factorial recursively.
 * @param n Specifies the nonnegative integer.
 * @return Factorial of n.
 * @pre n is nonnegative.
 */
int Factorial(int n)
{
  return (n == 1 || n == 0) ? 1 : Factorial(n - 1) * n;
}

/**
 * @brief Builds binomial coefficients through the selected order.
 * @param maxOrder Specifies the largest moment order.
 * @return Symmetric coefficient matrix.
 */
ComputeMomentInvariants2D::DoubleMatrixType Binomial(usize maxOrder)
{
  const int dim = static_cast<int>(maxOrder + 1);
  ComputeMomentInvariants2D::DoubleMatrixType bn(dim, dim);
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

/**
 * @brief Builds normalized integration weights for one coordinate axis.
 * @param maxOrder Specifies the largest moment order.
 * @param dim Specifies the square feature-image dimension.
 * @return Matrix that maps cell occupancy to moment terms.
 */
ComputeMomentInvariants2D::DoubleMatrixType GetBigX(usize maxOrder, usize dim)
{
  const int dRows = static_cast<int>(dim);
  const int dCols = static_cast<int>(dim + 1);

  ComputeMomentInvariants2D::DoubleMatrixType xx(1, dCols);
  for(int c = 0; c < dCols; c++)
  {
    xx(0, c) = c - static_cast<double>(dim) / 2.0 - 0.5;
  }

  const double fNorm = xx.maxCoeff();
  xx = xx / fNorm;

  ComputeMomentInvariants2D::DoubleMatrixType doubleMatrix(dRows, dCols);
  doubleMatrix.setZero();

  ComputeMomentInvariants2D::IntMatrixType j(1, dRows);
  for(int c = 0; c < dRows; c++)
  {
    j(0, c) = c;
  }

  for(int r = 0; r < dRows; r++)
  {
    doubleMatrix(r, r) = -1.0;
    doubleMatrix(r, r + 1) = 1.0;
  }

  ComputeMomentInvariants2D::DoubleMatrixType sc(1, maxOrder + 1);
  const int mop1 = static_cast<int>(maxOrder + 1);
  for(int c = 0; c < mop1; c++)
  {
    sc(0, c) = 1.0 / (c + 1.0);
  }

  ComputeMomentInvariants2D::DoubleMatrixType bigX(dim, maxOrder + 1);
  bigX.setZero();

  ComputeMomentInvariants2D::DoubleMatrixType yy;
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

    ComputeMomentInvariants2D::DoubleMatrixType mm = yy * doubleMatrix.transpose();
    mm = mm * sc(0, i);
    bigX.col(i) = mm.row(0);
  }

  return bigX;
}

/**
 * @brief Converts a square binary image to central moments.
 * @param input Provides the feature occupancy image.
 * @param inputDims Specifies the square image dimensions.
 * @param maxOrder Specifies the largest moment order.
 * @return Central-moment matrix through the selected order.
 * @pre inputDims contains equal nonzero dimensions.
 */
ComputeMomentInvariants2D::DoubleMatrixType ComputeMomentInvariants(const ComputeMomentInvariants2D::DoubleMatrixType& input, const usize* inputDims, usize maxOrder)
{
  assert(inputDims[0] == inputDims[1]);
  const usize dim = inputDims[0];
  ComputeMomentInvariants2D::DoubleMatrixType bigX = GetBigX(maxOrder, inputDims[0]);

  const int mDim = static_cast<int>(maxOrder + 1);
  const double fNorm = static_cast<double>(dim - 1) / 2.0;

  ComputeMomentInvariants2D::DoubleMatrixType bn = Binomial(maxOrder);

  ComputeMomentInvariants2D::DoubleMatrixType mnk(mDim, mDim);
  mnk.setZero();

  const ComputeMomentInvariants2D::DoubleMatrixType inter = input * bigX;

  mnk = bigX.transpose() * inter;

  for(int c = 0; c < mDim; c++)
  {
    for(int r = 0; r < mDim; r++)
    {
      mnk(r, c) *= std::pow(fNorm, (2 + c + r));
    }
  }

  // The binomial theorem moves raw moments to the feature centroid.
  const double xc = mnk(1, 0) / mnk(0, 0); // mnk[0,0] is the area of the object in units of pixels
  const double yc = mnk(0, 1) / mnk(0, 0);

  ComputeMomentInvariants2D::DoubleMatrixType mnkNew(mDim, mDim);
  mnkNew.setZero();

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

/**
 * @brief Converts raw moments to second-order central moments.
 * @param mnk Provides raw moment values.
 * @param dim Specifies the square feature-image dimension.
 * @return Second-order central-moment matrix.
 */
ComputeMomentInvariants2D::DoubleMatrixType ComputeCentralMoments(ComputeMomentInvariants2D::DoubleMatrixType mnk, usize dim)
{
  constexpr usize k_MaxOrder = 2;
  constexpr int k_MatrixDimension = static_cast<int>(k_MaxOrder + 1);
  const double fNorm = static_cast<double>(dim - 1) / 2.0;
  const ComputeMomentInvariants2D::DoubleMatrixType bn = Binomial(k_MaxOrder);

  for(int c = 0; c < k_MatrixDimension; c++)
  {
    for(int r = 0; r < k_MatrixDimension; r++)
    {
      mnk(r, c) *= std::pow(fNorm, (2 + c + r));
    }
  }

  const double xc = mnk(1, 0) / mnk(0, 0);
  const double yc = mnk(0, 1) / mnk(0, 0);

  ComputeMomentInvariants2D::DoubleMatrixType centralMoments(k_MatrixDimension, k_MatrixDimension);
  centralMoments.setZero();
  for(int p = 0; p < k_MatrixDimension; p++)
  {
    for(int q = 0; q < k_MatrixDimension; q++)
    {
      for(int k = 0; k < p + 1; k++)
      {
        for(int l = 0; l < q + 1; l++)
        {
          centralMoments(p, q) += std::pow(-1.0, (p + q - k - l)) * std::pow(xc, (p - k)) * std::pow(yc, (q - l)) * bn(p, k) * bn(q, l) * mnk(k, l);
        }
      }
    }
  }

  return centralMoments;
}

/**
 * @class ComputeMomentInvariants2DImpl
 * @brief Calculates feature moments with direct array access.
 *
 * The parallel worker reads and writes shared DataStore instances. DataStore
 * does not generally guarantee concurrent access. This is an existing direct-
 * path limitation.
 */
class ComputeMomentInvariants2DImpl
{
public:
  /**
   * @brief Creates a direct moment worker.
   * @param featureIds Provides cell Feature Id values.
   * @param featureRect Provides feature bounding rectangles.
   * @param omega1 Receives the first Omega invariant.
   * @param omega2 Receives the second Omega invariant.
   * @param centralMoments Receives optional central moments.
   * @param volDims Specifies image dimensions.
   * @param normalizeMomentInvariants Enables circle-based normalization.
   * @param mesgHandler Receives feature progress messages.
   * @param shouldCancel Stops later feature work when true.
   */
  ComputeMomentInvariants2DImpl(const Int32AbstractDataStore& featureIds, const UInt32AbstractDataStore& featureRect, Float32AbstractDataStore& omega1, Float32AbstractDataStore& omega2,
                                Float32Array* centralMoments, const SizeVec3& volDims, const bool normalizeMomentInvariants, const IFilter::MessageHandler& mesgHandler,
                                const std::atomic_bool& shouldCancel)
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

  /**
   * @brief Calculates moments and writes optional central moments.
   * @param start Specifies the first feature index.
   * @param end Specifies the exclusive feature index.
   * @param centralMoments Receives nine values per feature.
   *
   * A non-XY feature stops this worker range after zeroing its Omega values.
   */
  void convert(usize start, usize end, Float32AbstractDataStore& centralMoments) const
  {
    const usize numRectComponents = m_FeatureRect.getNumberOfComponents();
    const usize numFeatures = m_FeatureRect.getNumberOfTuples();
    for(usize featureId = start; featureId < end; featureId++)
    {
      const auto featureIdRectIndex = featureId * numRectComponents;
      std::array<uint32, 6> corner = {m_FeatureRect[featureIdRectIndex],     m_FeatureRect[featureIdRectIndex + 1], m_FeatureRect[featureIdRectIndex + 2],
                                      m_FeatureRect[featureIdRectIndex + 3], m_FeatureRect[featureIdRectIndex + 4], m_FeatureRect[featureIdRectIndex + 5]};
      constexpr usize maxOrder = 2;

      // The larger XY extent makes a square image for the moment basis.
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

      usize dim = std::max(xDim, yDim);

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
      ComputeMomentInvariants2D::DoubleMatrixType m2D = ::ComputeMomentInvariants(input2D, inputDims, maxOrder);
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
      m_Omega1[featureId] = static_cast<float32>(omega1);
      m_Omega2[featureId] = static_cast<float32>(omega2);

      const double* m2DInternal = m2D.array().data();
      for(usize comp = 0; comp < 9; comp++)
      {
        centralMoments[static_cast<usize>(featureId) * 9UL + comp] = static_cast<float32>(m2DInternal[comp]);
      }

      m_MessageHandler(IFilter::Message::Type::Info, fmt::format("[{}/{}] : Completed", featureId, numFeatures));

      if(m_ShouldCancel)
      {
        return;
      }
    }
  }

  /**
   * @brief Calculates Omega moments without central-moment output.
   * @param start Specifies the first feature index.
   * @param end Specifies the exclusive feature index.
   *
   * A non-XY feature stops this worker range after zeroing its Omega values.
   */
  void convert(usize start, usize end) const
  {
    const usize numRectComponents = m_FeatureRect.getNumberOfComponents();
    const usize numFeatures = m_FeatureRect.getNumberOfTuples();
    for(usize featureId = start; featureId < end; featureId++)
    {
      const auto featureIdRectIndex = featureId * numRectComponents;
      std::array<uint32, 6> corner = {m_FeatureRect[featureIdRectIndex],     m_FeatureRect[featureIdRectIndex + 1], m_FeatureRect[featureIdRectIndex + 2],
                                      m_FeatureRect[featureIdRectIndex + 3], m_FeatureRect[featureIdRectIndex + 4], m_FeatureRect[featureIdRectIndex + 5]};
      constexpr usize maxOrder = 2;

      // The larger XY extent makes a square image for the moment basis.
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

      usize dim = std::max(xDim, yDim);

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
      ComputeMomentInvariants2D::DoubleMatrixType m2D = ::ComputeMomentInvariants(input2D, inputDims, maxOrder);
      // compute the second order moment invariants
      double omega1 = 2.0 * (m2D(0, 0) * m2D(0, 0)) / (m2D(0, 2) + m2D(2, 0));
      double omega2 = std::pow(m2D(0, 0), 4) / (m2D(2, 0) * m2D(0, 2) - std::pow(m2D(1, 1), 2));

      if(m_NormalizeMomentInvariants)
      {
        // Circle values make the selected invariants dimensionless.
        constexpr double circleOmega[2] = {4.0 * numbers::pi, 16.0 * numbers::pi * numbers::pi};
        omega1 /= circleOmega[0];
        omega2 /= circleOmega[1];
      }
      m_Omega1[featureId] = static_cast<float32>(omega1);
      m_Omega2[featureId] = static_cast<float32>(omega2);

      m_MessageHandler(IFilter::Message::Type::Info, fmt::format("[{}/{}] : Completed", featureId, numFeatures));

      if(m_ShouldCancel)
      {
        return;
      }
    }
  }

  /**
   * @brief Processes one parallel feature range.
   * @param range Specifies the half-open feature-index range.
   */
  void operator()(const Range& range) const
  {
    if(m_CentralMoments != nullptr)
    {
      convert(range.min(), range.max(), m_CentralMoments->getDataStoreRef());
    }
    else
    {
      convert(range.min(), range.max());
    }
  }

  /**
   * @brief Executes direct calculation for all non-background features.
   * @return Success after worker completion.
   */
  Result<> operator()() const
  {
    const int32 numFeatures = static_cast<int32>(m_FeatureRect.getNumberOfTuples());
    ParallelDataAlgorithm dataAlg;
    dataAlg.setRange(1, numFeatures);
    dataAlg.setParallelizationEnabled(true);
    dataAlg.execute(*this);
    return {};
  }

private:
  const Int32AbstractDataStore& m_FeatureIds;
  const UInt32AbstractDataStore& m_FeatureRect;
  Float32AbstractDataStore& m_Omega1;
  Float32AbstractDataStore& m_Omega2;
  Float32Array* m_CentralMoments = nullptr;
  const SizeVec3& m_VolDims;
  const bool m_NormalizeMomentInvariants = true;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

/**
 * @class ComputeMomentInvariants2DScanline
 * @brief Calculates feature moments from bounded cell chunks.
 *
 * A 64 Ki-cell buffer bounds Feature Id I/O. Feature rectangles, moments, and
 * result buffers scale with the feature count.
 */
class ComputeMomentInvariants2DScanline
{
public:
  /**
   * @brief Creates a bulk-I/O moment worker.
   * @param featureIds Provides cell Feature Id values.
   * @param featureRect Provides feature bounding rectangles.
   * @param omega1 Receives the first Omega invariant.
   * @param omega2 Receives the second Omega invariant.
   * @param centralMoments Receives optional central moments.
   * @param volDims Specifies image dimensions.
   * @param normalizeMomentInvariants Enables circle-based normalization.
   * @param mesgHandler Receives feature progress messages.
   * @param shouldCancel Stops later feature work when true.
   */
  ComputeMomentInvariants2DScanline(const Int32AbstractDataStore& featureIds, const UInt32AbstractDataStore& featureRect, Float32AbstractDataStore& omega1, Float32AbstractDataStore& omega2,
                                    Float32Array* centralMoments, const SizeVec3& volDims, const bool normalizeMomentInvariants, const IFilter::MessageHandler& mesgHandler,
                                    const std::atomic_bool& shouldCancel)
  : m_FeatureIds(featureIds)
  , m_FeatureRect(featureRect)
  , m_Omega1(omega1)
  , m_Omega2(omega2)
  , m_CentralMoments(centralMoments)
  , m_VolDims(volDims)
  , m_NormalizeMomentInvariants(normalizeMomentInvariants)
  , m_MessageHandler(mesgHandler)
  , m_ShouldCancel(shouldCancel)
  {
  }

  /**
   * @brief Calculates all non-background feature moments.
   * @return Error from bulk I/O, or success after cancellation.
   *
   * Cancellation occurs before result-array write-back. Sequential Omega1,
   * Omega2, and central-moment writes can fail separately. A later write error
   * can leave earlier output arrays written.
   */
  Result<> operator()() const
  {
    constexpr usize k_MaxOrder = 2;
    constexpr usize k_MatrixDimension = k_MaxOrder + 1;
    constexpr usize k_CellChunkSize = 64ULL * 1024ULL;

    const usize numFeatures = m_FeatureRect.getNumberOfTuples();
    const usize numRectComponents = m_FeatureRect.getNumberOfComponents();
    const usize numCells = m_FeatureIds.getNumberOfTuples();

    // Feature buffers avoid a full cell cache but can be large for many features.
    // NOLINTNEXTLINE(modernize-avoid-c-arrays) -- Runtime-sized buffer; std::array cannot represent this extent.
    auto featureRects = std::make_unique<uint32[]>(numFeatures * numRectComponents);
    Result<> result = m_FeatureRect.copyIntoBuffer(0, nonstd::span<uint32>(featureRects.get(), numFeatures * numRectComponents));
    if(result.invalid())
    {
      return result;
    }

    // NOLINTNEXTLINE(modernize-avoid-c-arrays) -- The outer extent is runtime-sized; each fixed-size inner sequence is a std::array.
    auto rawMoments = std::make_unique<std::array<double, k_MatrixDimension * k_MatrixDimension>[]>(numFeatures);
    auto featureIdsBuffer = std::make_unique<std::array<int32, k_CellChunkSize>>();
    MessageHelper messageHelper(m_MessageHandler);

    for(usize cellOffset = 0; cellOffset < numCells; cellOffset += k_CellChunkSize)
    {
      if(m_ShouldCancel)
      {
        return {};
      }

      const usize cellCount = std::min(k_CellChunkSize, numCells - cellOffset);
      result = m_FeatureIds.copyIntoBuffer(cellOffset, nonstd::span<int32>(featureIdsBuffer->data(), cellCount));
      if(result.invalid())
      {
        return result;
      }

      for(usize localIndex = 0; localIndex < cellCount; localIndex++)
      {
        const int32 featureIdValue = (*featureIdsBuffer)[localIndex];
        if(featureIdValue <= 0 || static_cast<usize>(featureIdValue) >= numFeatures)
        {
          continue;
        }

        const usize featureId = static_cast<usize>(featureIdValue);
        const auto* rect = featureRects.get() + featureId * numRectComponents;
        const uint32 xDim = rect[3] - rect[0] + 1;
        const uint32 yDim = rect[4] - rect[1] + 1;
        if(rect[5] - rect[2] + 1 != 1)
        {
          continue;
        }

        const usize cellIndex = cellOffset + localIndex;
        const usize x = cellIndex % m_VolDims[0];
        const usize y = (cellIndex / m_VolDims[0]) % m_VolDims[1];
        if(x < rect[0] || x > rect[3] || y < rect[1] || y > rect[4])
        {
          continue;
        }

        const usize dim = std::max(static_cast<usize>(xDim), static_cast<usize>(yDim));
        const auto xBasis = getBasis(x - rect[0], dim);
        const auto yBasis = getBasis(y - rect[1], dim);
        auto& featureMoments = rawMoments[featureId];
        for(usize row = 0; row < k_MatrixDimension; row++)
        {
          for(usize column = 0; column < k_MatrixDimension; column++)
          {
            featureMoments[row * k_MatrixDimension + column] += yBasis[row] * xBasis[column];
          }
        }
      }
    }

    // NOLINTNEXTLINE(modernize-avoid-c-arrays) -- Runtime-sized result buffer; std::array cannot represent this extent.
    auto omega1Values = std::make_unique<float32[]>(numFeatures);
    // NOLINTNEXTLINE(modernize-avoid-c-arrays) -- Runtime-sized result buffer; std::array cannot represent this extent.
    auto omega2Values = std::make_unique<float32[]>(numFeatures);
    // NOLINTNEXTLINE(modernize-avoid-c-arrays) -- Runtime-sized optional result buffer; std::array cannot represent this extent.
    std::unique_ptr<float32[]> centralMomentValues;
    if(m_CentralMoments != nullptr)
    {
      // NOLINTNEXTLINE(modernize-avoid-c-arrays) -- Runtime-sized optional result buffer; std::array cannot represent this extent.
      centralMomentValues = std::make_unique<float32[]>(numFeatures * k_MatrixDimension * k_MatrixDimension);
    }

    for(usize featureId = 1; featureId < numFeatures; featureId++)
    {
      if(m_ShouldCancel)
      {
        return {};
      }

      const auto* rect = featureRects.get() + featureId * numRectComponents;
      if(rect[5] - rect[2] + 1 != 1)
      {
        messageHelper.trySendMessage(fmt::format("[{}/{}] : Feature {} is NOT strictly 2D in the XY plane. Skipping this feature.", featureId, numFeatures, featureId));
        continue;
      }

      const usize dim = std::max(static_cast<usize>(rect[3] - rect[0] + 1), static_cast<usize>(rect[4] - rect[1] + 1));
      ComputeMomentInvariants2D::DoubleMatrixType moments(k_MatrixDimension, k_MatrixDimension);
      for(usize row = 0; row < k_MatrixDimension; row++)
      {
        for(usize column = 0; column < k_MatrixDimension; column++)
        {
          moments(static_cast<int>(row), static_cast<int>(column)) = rawMoments[featureId][row * k_MatrixDimension + column];
        }
      }

      const auto centralMoments = ComputeCentralMoments(std::move(moments), dim);
      double omega1 = 2.0 * (centralMoments(0, 0) * centralMoments(0, 0)) / (centralMoments(0, 2) + centralMoments(2, 0));
      double omega2 = std::pow(centralMoments(0, 0), 4) / (centralMoments(2, 0) * centralMoments(0, 2) - std::pow(centralMoments(1, 1), 2));
      if(m_NormalizeMomentInvariants)
      {
        constexpr std::array<double, 2> k_CircleOmega = {4.0 * numbers::pi, 16.0 * numbers::pi * numbers::pi};
        omega1 /= k_CircleOmega[0];
        omega2 /= k_CircleOmega[1];
      }

      omega1Values[featureId] = static_cast<float32>(omega1);
      omega2Values[featureId] = static_cast<float32>(omega2);
      if(centralMomentValues != nullptr)
      {
        const double* centralMomentsData = centralMoments.array().data();
        const usize centralMomentsOffset = featureId * k_MatrixDimension * k_MatrixDimension;
        for(usize component = 0; component < k_MatrixDimension * k_MatrixDimension; component++)
        {
          centralMomentValues[centralMomentsOffset + component] = static_cast<float32>(centralMomentsData[component]);
        }
      }
    }

    result = m_Omega1.copyFromBuffer(0, nonstd::span<const float32>(omega1Values.get(), numFeatures));
    if(result.invalid())
    {
      return result;
    }
    result = m_Omega2.copyFromBuffer(0, nonstd::span<const float32>(omega2Values.get(), numFeatures));
    if(result.invalid())
    {
      return result;
    }
    if(centralMomentValues != nullptr)
    {
      result = m_CentralMoments->getDataStoreRef().copyFromBuffer(0, nonstd::span<const float32>(centralMomentValues.get(), numFeatures * k_MatrixDimension * k_MatrixDimension));
      if(result.invalid())
      {
        return result;
      }
    }

    return {};
  }

private:
  /**
   * @brief Integrates normalized coordinate powers over one cell.
   * @param coordinate Specifies the zero-based cell coordinate.
   * @param dim Specifies the square feature-image dimension.
   * @return Basis terms through second order.
   */
  static std::array<double, 3> getBasis(usize coordinate, usize dim)
  {
    std::array<double, 3> basis = {};
    const double normalization = static_cast<double>(dim - 1) / 2.0;
    const double start = (static_cast<double>(coordinate) - static_cast<double>(dim) / 2.0 - 0.5) / normalization;
    const double end = (static_cast<double>(coordinate + 1) - static_cast<double>(dim) / 2.0 - 0.5) / normalization;

    double startPower = start;
    double endPower = end;
    for(usize order = 0; order < basis.size(); order++)
    {
      basis[order] = (endPower - startPower) / static_cast<double>(order + 1);
      startPower *= start;
      endPower *= end;
    }
    return basis;
  }

  const Int32AbstractDataStore& m_FeatureIds;
  const UInt32AbstractDataStore& m_FeatureRect;
  Float32AbstractDataStore& m_Omega1;
  Float32AbstractDataStore& m_Omega2;
  Float32Array* m_CentralMoments = nullptr;
  const SizeVec3& m_VolDims;
  const bool m_NormalizeMomentInvariants = true;
  const IFilter::MessageHandler& m_MessageHandler;
  const std::atomic_bool& m_ShouldCancel;
};
} // namespace
ComputeMomentInvariants2D::ComputeMomentInvariants2D(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                     ComputeMomentInvariants2DInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ComputeMomentInvariants2D::~ComputeMomentInvariants2D() noexcept = default;

const std::atomic_bool& ComputeMomentInvariants2D::getCancel()
{
  return m_ShouldCancel;
}

Result<> ComputeMomentInvariants2D::operator()()
{
  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  const SizeVec3 volDims = imageGeom.getDimensions();

  const auto& featureIdsArray = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  const auto& featureRectArray = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->FeatureRectArrayPath);
  auto& omega1Array = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->Omega1ArrayPath);
  auto& omega2Array = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->Omega2ArrayPath);
  const auto& featureIds = featureIdsArray.getDataStoreRef();
  const auto& featureRect = featureRectArray.getDataStoreRef();
  auto& omega1 = omega1Array.getDataStoreRef();
  auto& omega2 = omega2Array.getDataStoreRef();
  Float32Array* centralMoments = nullptr;
  if(m_InputValues->SaveCentralMoments)
  {
    centralMoments = &m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CentralMomentsArrayPath);
  }

  return DispatchAlgorithm<ComputeMomentInvariants2DImpl, ComputeMomentInvariants2DScanline>({&featureIdsArray, &featureRectArray, &omega1Array, &omega2Array, centralMoments}, featureIds, featureRect,
                                                                                             omega1, omega2, centralMoments, volDims, m_InputValues->NormalizeMomentInvariants, m_MessageHandler,
                                                                                             m_ShouldCancel);
}
