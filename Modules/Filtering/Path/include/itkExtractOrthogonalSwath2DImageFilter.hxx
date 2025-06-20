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
#ifndef itkExtractOrthogonalSwath2DImageFilter_hxx
#define itkExtractOrthogonalSwath2DImageFilter_hxx
#include "itkImageRegionIteratorWithIndex.h"
#include "itkLinearInterpolateImageFunction.h"
#include "itkProgressReporter.h"
#include "itkNumericTraits.h"
#include "itkMath.h"

namespace itk
{
template <typename TImage>
void
ExtractOrthogonalSwath2DImageFilter<TImage>::SetSpacing(const double * spacing)
{

  if (ContainerCopyWithCheck(m_Spacing, spacing, Self::ImageDimension))
  {
    this->Modified();
  }
}

template <typename TImage>
void
ExtractOrthogonalSwath2DImageFilter<TImage>::SetSpacing(const float * spacing)
{
  if (ContainerCopyWithCheck(m_Spacing, spacing, Self::ImageDimension))
  {
    this->Modified();
  }
}

template <typename TImage>
const double *
ExtractOrthogonalSwath2DImageFilter<TImage>::GetSpacing() const
{
  return m_Spacing;
}

//----------------------------------------------------------------------------
template <typename TImage>
void
ExtractOrthogonalSwath2DImageFilter<TImage>::SetOrigin(const double * origin)
{
  if (ContainerCopyWithCheck(m_Origin, origin, Self::ImageDimension))
  {
    this->Modified();
  }
}

template <typename TImage>
void
ExtractOrthogonalSwath2DImageFilter<TImage>::SetOrigin(const float * origin)
{
  if (ContainerCopyWithCheck(m_Origin, origin, Self::ImageDimension))
  {
    this->Modified();
  }
}

template <typename TImage>
const double *
ExtractOrthogonalSwath2DImageFilter<TImage>::GetOrigin() const
{
  return m_Origin;
}

//----------------------------------------------------------------------------

template <typename TImage>
void
ExtractOrthogonalSwath2DImageFilter<TImage>::GenerateOutputInformation()
{
  const ImagePointer outputPtr = this->GetOutput(0);

  const ImageRegionType outputRegion(this->m_Size);
  outputPtr->SetLargestPossibleRegion(outputRegion);
  outputPtr->SetSpacing(this->m_Spacing);
  outputPtr->SetOrigin(this->m_Origin);
}

/**
 * GenerateData Performs the reflection
 */
template <typename TImage>
void
ExtractOrthogonalSwath2DImageFilter<TImage>::GenerateData()
{
  const ImageConstPointer inputImagePtr = this->GetImageInput();
  const PathConstPointer  inputPathPtr = this->GetPathInput();
  const ImagePointer      outputPtr = this->GetOutput(0);

  // Generate the output image
  const ImageRegionType outputRegion = outputPtr->GetRequestedRegion();

  outputPtr->SetBufferedRegion(outputRegion);
  outputPtr->Allocate();

  // support progress methods/callbacks
  ProgressReporter progress(this, 0, outputRegion.GetNumberOfPixels());

  using OutputIterator = ImageRegionIteratorWithIndex<ImageType>;
  using InterpolatorType = LinearInterpolateImageFunction<ImageType, itk::SpacePrecisionType>;

  auto interpolator = InterpolatorType::New();
  interpolator->SetInputImage(inputImagePtr);

  // Iterate through the output image
  for (OutputIterator outputIt(outputPtr, outputPtr->GetRequestedRegion()); !outputIt.IsAtEnd(); ++outputIt)
  {
    ImageIndexType index = outputIt.GetIndex();

    // what position along the path corresponds to this column of the swath?
    PathInputType pathInput =
      inputPathPtr->StartOfInput() + static_cast<double>(inputPathPtr->EndOfInput() - inputPathPtr->StartOfInput()) *
                                       static_cast<double>(index[0]) / static_cast<double>(m_Size[0]);

    // What is the orthogonal offset from the path in the input image for this
    // particular index in the output swath image?
    // Vertically centered swath pixels lie on the path in the input image.
    double orthogonalOffset = index[1] - static_cast<int>(m_Size[1] / 2); // use signed arithmetic

    // Make continuousIndex point to the source pixel in the input image
    ContinuousIndex<SpacePrecisionType> continousIndex = inputPathPtr->Evaluate(pathInput);
    PathVectorType                      pathDerivative = inputPathPtr->EvaluateDerivative(pathInput);
    pathDerivative.Normalize();
    continousIndex[0] -= orthogonalOffset * pathDerivative[1];
    continousIndex[1] += orthogonalOffset * pathDerivative[0];

    // set the swath pixel to the interpolated input pixel
    if (!interpolator->IsInsideBuffer(continousIndex))
    {
      // itkExceptionMacro(<<"Requested input index ["<<continuousIndex
      //                  <<"] is not in the input image" );
      outputIt.Set(m_DefaultPixelValue);
      progress.CompletedPixel();
      continue;
    }

    // prevent small interpolation error from rounding-down integer types
    if (NumericTraits<ImagePixelType>::is_integer)
    {
      outputIt.Set(static_cast<ImagePixelType>(0.5 + interpolator->EvaluateAtContinuousIndex(continousIndex)));
    }
    else
    {
      outputIt.Set(static_cast<ImagePixelType>(interpolator->EvaluateAtContinuousIndex(continousIndex)));
    }

    progress.CompletedPixel();
  }
}

template <typename TImage>
void
ExtractOrthogonalSwath2DImageFilter<TImage>::PrintSelf(std::ostream & os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);
  os << indent << "Size:  " << m_Size << std::endl;
  os << indent
     << "DefaultPixelValue:  " << static_cast<typename NumericTraits<ImagePixelType>::PrintType>(m_DefaultPixelValue)
     << std::endl;
}
} // end namespace itk

#endif
