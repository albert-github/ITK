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
#ifndef itkMedianImageFunction_hxx
#define itkMedianImageFunction_hxx

#include "itkImage.h"
#include "itkShapedImageNeighborhoodRange.h"

#include <vector>
#include <algorithm>

namespace itk
{

template <typename TInputImage, typename TCoordinate>
MedianImageFunction<TInputImage, TCoordinate>::MedianImageFunction() = default;

template <typename TInputImage, typename TCoordinate>
void
MedianImageFunction<TInputImage, TCoordinate>::SetNeighborhoodRadius(const unsigned int radius)
{
  if (m_NeighborhoodRadius != radius)
  {
    m_NeighborhoodOffsets = GenerateRectangularImageNeighborhoodOffsets(ImageSizeType::Filled(radius));
    m_NeighborhoodRadius = radius;
    this->Modified();
  }
}

template <typename TInputImage, typename TCoordinate>
void
MedianImageFunction<TInputImage, TCoordinate>::PrintSelf(std::ostream & os, Indent indent) const
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NeighborhoodRadius: " << m_NeighborhoodRadius << std::endl;
}

template <typename TInputImage, typename TCoordinate>
auto
MedianImageFunction<TInputImage, TCoordinate>::EvaluateAtIndex(const IndexType & index) const -> OutputType
{
  const InputImageType * const image = this->GetInputImage();

  if (image == nullptr)
  {
    return (NumericTraits<OutputType>::max());
  }

  if (!this->IsInsideBuffer(index))
  {
    return (NumericTraits<OutputType>::max());
  }

  const ShapedImageNeighborhoodRange<const InputImageType> neighborhoodRange(*image, index, m_NeighborhoodOffsets);

  // We have to copy the pixels so we can run std::nth_element.
  std::vector<InputPixelType> pixels(neighborhoodRange.cbegin(), neighborhoodRange.cend());

  // Get the median value
  const auto medianIterator = pixels.begin() + (pixels.size() / 2);
  std::nth_element(pixels.begin(), medianIterator, pixels.end());

  return (*medianIterator);
}
} // namespace itk

#endif
