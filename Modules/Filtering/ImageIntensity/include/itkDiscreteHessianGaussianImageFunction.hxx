/*=========================================================================
 *
 *  Copyright NumFOCUS
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         https://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/
#ifndef itkDiscreteHessianGaussianImageFunction_hxx
#define itkDiscreteHessianGaussianImageFunction_hxx

#include "itkNeighborhoodOperatorImageFilter.h"

namespace itk
{
/** Set the Input Image */
template <typename TInputImage, typename TOutput>
DiscreteHessianGaussianImageFunction<TInputImage, TOutput>::DiscreteHessianGaussianImageFunction()
  : m_OperatorImageFunction(OperatorImageFunctionType::New())
{
  m_Variance.Fill(1.0);
}

/** Print self method */
template <typename TInputImage, typename TOutput>
void
DiscreteHessianGaussianImageFunction<TInputImage, TOutput>::PrintSelf(std::ostream & os, Indent indent) const
{
  this->Superclass::PrintSelf(os, indent);
  itkPrintSelfBooleanMacro(UseImageSpacing);
  os << indent << "NormalizeAcrossScale: " << m_NormalizeAcrossScale << std::endl;
  os << indent << "Variance: " << m_Variance << std::endl;
  os << indent << "MaximumError: " << m_MaximumError << std::endl;
  os << indent << "MaximumKernelWidth: " << m_MaximumKernelWidth << std::endl;
  os << indent << "OperatorArray: " << m_OperatorArray << std::endl;
  os << indent << "KernelArray: " << m_KernelArray << std::endl;
  os << indent << "OperatorImageFunction: " << m_OperatorImageFunction << std::endl;
  os << indent << "InterpolationMode: " << m_InterpolationMode << std::endl;
}

/** Set the input image */
template <typename TInputImage, typename TOutput>
void
DiscreteHessianGaussianImageFunction<TInputImage, TOutput>::SetInputImage(const InputImageType * ptr)
{
  Superclass::SetInputImage(ptr);
  m_OperatorImageFunction->SetInputImage(ptr);
}

/** Recompute the gaussian kernel used to evaluate indexes
 *  This should use a fastest Derivative Gaussian operator */
template <typename TInputImage, typename TOutput>
void
DiscreteHessianGaussianImageFunction<TInputImage, TOutput>::RecomputeGaussianKernel()
{
  /* Create 3*N operators (N=ImageDimension) where the
   * first N are zero-order, the second N are first-order
   * and the third N are second order */
  unsigned int maxRadius = 0;

  for (unsigned int direction = 0; direction < Self::ImageDimension2; ++direction)
  {
    for (unsigned int order = 0; order <= 2; ++order)
    {
      const auto idx = Self::ImageDimension2 * order + direction;
      m_OperatorArray[idx].SetDirection(direction);
      m_OperatorArray[idx].SetMaximumKernelWidth(m_MaximumKernelWidth);
      m_OperatorArray[idx].SetMaximumError(m_MaximumError);

      if (m_UseImageSpacing && (this->GetInputImage()))
      {
        m_OperatorArray[idx].SetSpacing(this->GetInputImage()->GetSpacing()[direction]);
      }

      // NOTE: GaussianDerivativeOperator modifies the variance when
      // setting image spacing
      m_OperatorArray[idx].SetVariance(m_Variance[direction]);
      m_OperatorArray[idx].SetOrder(order);
      m_OperatorArray[idx].SetNormalizeAcrossScale(m_NormalizeAcrossScale);
      m_OperatorArray[idx].CreateDirectional();

      // Check for maximum radius
      for (unsigned int i = 0; i < Self::ImageDimension2; ++i)
      {
        if (m_OperatorArray[idx].GetRadius()[i] > maxRadius)
        {
          maxRadius = m_OperatorArray[idx].GetRadius()[i];
        }
      }
    }
  }

  // Now precompute the n-dimensional kernel. This fastest as we don't
  // have to perform N convolutions for each point we calculate but
  // only one.

  using KernelImageType = itk::Image<TOutput, Self::ImageDimension2>;
  auto kernelImage = KernelImageType::New();

  using RegionType = typename KernelImageType::RegionType;
  RegionType region;

  auto size = RegionType::SizeType::Filled(4 * maxRadius + 1);
  region.SetSize(size);

  kernelImage->SetRegions(region);
  kernelImage->AllocateInitialized();

  // Initially the kernel image will be an impulse at the center
  auto centerIndex = KernelImageType::IndexType::Filled(2 * maxRadius); // include also boundaries

  // Create an image region to be used later that does not include boundaries
  RegionType kernelRegion;
  size.Fill(2 * maxRadius + 1);
  auto origin = RegionType::IndexType::Filled(maxRadius);
  kernelRegion.SetSize(size);
  kernelRegion.SetIndex(origin);

  // Now create an image filter to perform successive convolutions
  using NeighborhoodFilterType = itk::NeighborhoodOperatorImageFilter<KernelImageType, KernelImageType>;
  auto convolutionFilter = NeighborhoodFilterType::New();

  // Array that stores the current order for each direction
  using OrderArrayType = FixedArray<unsigned int, Self::ImageDimension2>;
  OrderArrayType orderArray;

  // Precalculate compound derivative kernels (n-dimensional)
  // The order of calculation in the 3D case is: dxx, dxy, dxz, dyy,
  // dyz, dzz

  unsigned int kernelidx = 0;

  for (unsigned int i = 0; i < Self::ImageDimension2; ++i)
  {
    for (unsigned int j = i; j < Self::ImageDimension2; ++j)
    {
      orderArray.Fill(0);
      ++orderArray[i];
      ++orderArray[j];

      // Reset kernel image
      kernelImage->FillBuffer(TOutput{});
      kernelImage->SetPixel(centerIndex, itk::NumericTraits<TOutput>::OneValue());

      for (unsigned int direction = 0; direction < Self::ImageDimension2; ++direction)
      {
        const auto opidx = Self::ImageDimension2 * orderArray[direction] + direction;
        convolutionFilter->SetInput(kernelImage);
        convolutionFilter->SetOperator(m_OperatorArray[opidx]);
        convolutionFilter->Update();
        kernelImage = convolutionFilter->GetOutput();
        kernelImage->DisconnectPipeline();
      }

      // Set the size of the current kernel
      m_KernelArray[kernelidx].SetRadius(maxRadius);

      // Copy kernel image to neighborhood. Do not copy boundaries.
      ImageRegionConstIterator<KernelImageType> it(kernelImage, kernelRegion);
      it.GoToBegin();
      unsigned int idx = 0;
      while (!it.IsAtEnd())
      {
        m_KernelArray[kernelidx][idx] = it.Get();
        ++idx;
        ++it;
      }
      ++kernelidx;
    }
  }
}

/** Evaluate the function at the specified index */
template <typename TInputImage, typename TOutput>
auto
DiscreteHessianGaussianImageFunction<TInputImage, TOutput>::EvaluateAtIndex(const IndexType & index) const -> OutputType
{
  OutputType hessian;

  for (unsigned int i = 0; i < m_KernelArray.Size(); ++i)
  {
    m_OperatorImageFunction->SetOperator(m_KernelArray[i]);
    hessian[i] = m_OperatorImageFunction->EvaluateAtIndex(index);
  }
  return hessian;
}

/** Evaluate the function at the specified point */
template <typename TInputImage, typename TOutput>
auto
DiscreteHessianGaussianImageFunction<TInputImage, TOutput>::Evaluate(const PointType & point) const -> OutputType
{
  if (m_InterpolationMode == InterpolationModeEnum::NearestNeighbourInterpolation)
  {
    IndexType index;
    this->ConvertPointToNearestIndex(point, index);
    return this->EvaluateAtIndex(index);
  }

  ContinuousIndexType cindex;
  this->ConvertPointToContinuousIndex(point, cindex);
  return this->EvaluateAtContinuousIndex(cindex);
}

/** Evaluate the function at specified ContinuousIndex position.*/
template <typename TInputImage, typename TOutput>
auto
DiscreteHessianGaussianImageFunction<TInputImage, TOutput>::EvaluateAtContinuousIndex(
  const ContinuousIndexType & cindex) const -> OutputType
{
  if (m_InterpolationMode == InterpolationModeEnum::NearestNeighbourInterpolation)
  {
    IndexType index;
    this->ConvertContinuousIndexToNearestIndex(cindex, index);
    return this->EvaluateAtIndex(index);
  }

  using NumberOfNeighborsType = unsigned int;

  constexpr NumberOfNeighborsType neighbors = 1 << ImageDimension2;

  // Compute base index = closet index below point
  // Compute distance from point to base index
  IndexType baseIndex;
  double    distance[ImageDimension2];
  for (unsigned int dim = 0; dim < ImageDimension2; ++dim)
  {
    baseIndex[dim] = Math::Floor<IndexValueType>(cindex[dim]);
    distance[dim] = cindex[dim] - static_cast<double>(baseIndex[dim]);
  }

  // Interpolated value is the weighted sum of each of the surrounding
  // neighbors. The weight for each neighbor is the fraction overlap
  // of the neighbor pixel with respect to a pixel centered on point.
  OutputType hessian;
  TOutput    totalOverlap{};
  for (NumberOfNeighborsType counter = 0; counter < neighbors; ++counter)
  {
    double                overlap = 1.0;   // fraction overlap
    NumberOfNeighborsType upper = counter; // each bit indicates upper/lower neighbour
    IndexType             neighIndex;

    // get neighbor index and overlap fraction
    for (unsigned int dim = 0; dim < ImageDimension2; ++dim)
    {
      if (upper & 1)
      {
        neighIndex[dim] = baseIndex[dim] + 1;
        overlap *= distance[dim];
      }
      else
      {
        neighIndex[dim] = baseIndex[dim];
        overlap *= 1.0 - distance[dim];
      }
      upper >>= 1;
    }

    // get neighbor value only if overlap is not zero
    if (overlap)
    {
      const auto currentHessian = this->EvaluateAtIndex(neighIndex);
      for (unsigned int i = 0; i < hessian.Size(); ++i)
      {
        hessian[i] += overlap * currentHessian[i];
      }
      totalOverlap += overlap;
    }

    if (totalOverlap == 1.0)
    {
      // finished
      break;
    }
  }

  return hessian;
}
} // end namespace itk

#endif
