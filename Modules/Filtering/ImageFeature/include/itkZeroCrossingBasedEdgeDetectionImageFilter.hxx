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
#ifndef itkZeroCrossingBasedEdgeDetectionImageFilter_hxx
#define itkZeroCrossingBasedEdgeDetectionImageFilter_hxx

#include "itkDiscreteGaussianImageFilter.h"
#include "itkLaplacianImageFilter.h"
#include "itkZeroCrossingImageFilter.h"
#include "itkProgressAccumulator.h"

namespace itk
{


template <typename TInputImage, typename TOutputImage>
ZeroCrossingBasedEdgeDetectionImageFilter<TInputImage, TOutputImage>::ZeroCrossingBasedEdgeDetectionImageFilter()
  : m_BackgroundValue(OutputImagePixelType{})
  , m_ForegroundValue(NumericTraits<OutputImagePixelType>::OneValue())
{
  m_Variance.Fill(1.0);
  m_MaximumError.Fill(0.01);
}

template <typename TInputImage, typename TOutputImage>
void
ZeroCrossingBasedEdgeDetectionImageFilter<TInputImage, TOutputImage>::GenerateData()
{
  const typename InputImageType::ConstPointer input = this->GetInput();

  // Create the filters that are needed
  auto gaussianFilter = DiscreteGaussianImageFilter<TInputImage, TOutputImage>::New();
  auto laplacianFilter = LaplacianImageFilter<TInputImage, TOutputImage>::New();
  auto zerocrossingFilter = ZeroCrossingImageFilter<TInputImage, TOutputImage>::New();

  // Create a process accumulator for tracking the progress of this minipipeline
  auto progress = ProgressAccumulator::New();
  progress->SetMiniPipelineFilter(this);

  // Construct the mini-pipeline

  // Apply the Gaussian filter
  gaussianFilter->SetVariance(m_Variance);
  gaussianFilter->SetMaximumError(m_MaximumError);
  gaussianFilter->SetInput(input);
  progress->RegisterInternalFilter(gaussianFilter, 1.0f / 3.0f);

  // Calculate the laplacian of the smoothed image
  laplacianFilter->SetInput(gaussianFilter->GetOutput());
  progress->RegisterInternalFilter(laplacianFilter, 1.0f / 3.0f);

  // Find the zero-crossings of the laplacian
  zerocrossingFilter->SetInput(laplacianFilter->GetOutput());
  zerocrossingFilter->SetBackgroundValue(m_BackgroundValue);
  zerocrossingFilter->SetForegroundValue(m_ForegroundValue);
  zerocrossingFilter->GraftOutput(this->GetOutput());
  progress->RegisterInternalFilter(zerocrossingFilter, 1.0f / 3.0f);
  zerocrossingFilter->Update();

  // Graft the output of the mini-pipeline back onto the filter's output.
  // This action copies back the region ivars and meta-data
  this->GraftOutput(zerocrossingFilter->GetOutput());
}

template <typename TInputImage, typename TOutputImage>
void
ZeroCrossingBasedEdgeDetectionImageFilter<TInputImage, TOutputImage>::PrintSelf(std::ostream & os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);
  os << indent << "Variance: " << m_Variance << std::endl;
  os << indent << "MaximumError: " << m_MaximumError << std::endl;
  os << indent
     << "ForegroundValue: " << static_cast<typename NumericTraits<OutputImagePixelType>::PrintType>(m_ForegroundValue)
     << std::endl;
  os << indent
     << "BackgroundValue: " << static_cast<typename NumericTraits<OutputImagePixelType>::PrintType>(m_BackgroundValue)
     << std::endl;
}
} // namespace itk

#endif
