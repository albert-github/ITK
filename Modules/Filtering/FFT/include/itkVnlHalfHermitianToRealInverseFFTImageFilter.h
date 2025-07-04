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
#ifndef itkVnlHalfHermitianToRealInverseFFTImageFilter_h
#define itkVnlHalfHermitianToRealInverseFFTImageFilter_h

#include "itkHalfHermitianToRealInverseFFTImageFilter.h"

#include "itkVnlFFTCommon.h"
#include "itkImage.h"
#include "vnl/algo/vnl_fft_base.h"

#include "itkFFTImageFilterFactory.h"

namespace itk
{
/**
 * \class VnlHalfHermitianToRealInverseFFTImageFilter
 *
 * \brief VNL-based reverse Fast Fourier Transform.
 *
 * The input image size must be a multiple of combinations of 2s, 3s,
 * and/or 5s in all dimensions (2, 3, and 5 should be the only prime
 * factors of the image size along each dimension).
 *
 * \ingroup FourierTransform
 *
 * \sa HalfHermitianToRealInverseFFTImageFilter
 * \ingroup ITKFFT
 *
 */
template <typename TInputImage,
          typename TOutputImage = Image<typename TInputImage::PixelType::value_type, TInputImage::ImageDimension>>
class ITK_TEMPLATE_EXPORT VnlHalfHermitianToRealInverseFFTImageFilter
  : public HalfHermitianToRealInverseFFTImageFilter<TInputImage, TOutputImage>
{
public:
  ITK_DISALLOW_COPY_AND_MOVE(VnlHalfHermitianToRealInverseFFTImageFilter);

  /** Standard class type aliases. */
  using InputImageType = TInputImage;
  using InputPixelType = typename InputImageType::PixelType;
  using InputSizeType = typename InputImageType::SizeType;
  using InputIndexType = typename InputImageType::IndexType;
  using InputSizeValueType = typename InputImageType::SizeValueType;
  using OutputImageType = TOutputImage;
  using OutputPixelType = typename OutputImageType::PixelType;
  using OutputIndexType = typename OutputImageType::IndexType;
  using OutputSizeType = typename OutputImageType::SizeType;
  using OutputIndexValueType = typename OutputImageType::IndexValueType;

  using Self = VnlHalfHermitianToRealInverseFFTImageFilter;
  using Superclass = HalfHermitianToRealInverseFFTImageFilter<TInputImage, TOutputImage>;
  using Pointer = SmartPointer<Self>;
  using ConstPointer = SmartPointer<const Self>;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** \see LightObject::GetNameOfClass() */
  itkOverrideGetNameOfClassMacro(VnlHalfHermitianToRealInverseFFTImageFilter);

  /** Extract the dimensionality of the images. They must be the
   * same. */
  static constexpr unsigned int ImageDimension = TOutputImage::ImageDimension;
  static constexpr unsigned int InputImageDimension = TInputImage::ImageDimension;
  static constexpr unsigned int OutputImageDimension = TOutputImage::ImageDimension;

  [[nodiscard]] SizeValueType
  GetSizeGreatestPrimeFactor() const override;

  itkConceptMacro(PixelUnsignedIntDivisionOperatorsCheck, (Concept::DivisionOperators<OutputPixelType, unsigned int>));
  itkConceptMacro(ImageDimensionsMatchCheck, (Concept::SameDimension<InputImageDimension, OutputImageDimension>));

protected:
  VnlHalfHermitianToRealInverseFFTImageFilter() = default;
  ~VnlHalfHermitianToRealInverseFFTImageFilter() override = default;

  void
  GenerateData() override;

private:
  using SignalVectorType = vnl_vector<InputPixelType>;
};

// Describe whether input/output are real- or complex-valued
// for factory registration
template <>
struct FFTImageFilterTraits<VnlHalfHermitianToRealInverseFFTImageFilter>
{
  template <typename TUnderlying>
  using InputPixelType = std::complex<TUnderlying>;
  template <typename TUnderlying>
  using OutputPixelType = TUnderlying;
  using FilterDimensions = std::integer_sequence<unsigned int, 4, 3, 2, 1>;
};

} // namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#  include "itkVnlHalfHermitianToRealInverseFFTImageFilter.hxx"
#endif

#endif
