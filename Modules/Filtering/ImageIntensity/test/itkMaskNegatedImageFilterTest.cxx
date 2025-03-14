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


#include "itkImageRegionIteratorWithIndex.h"
#include "itkMaskNegatedImageFilter.h"
#include "itkVectorImage.h"
#include "itkTestingMacros.h"


int
itkMaskNegatedImageFilterTest(int, char *[])
{

  // Define the dimension of the images
  constexpr unsigned int myDimension = 3;

  // Declare the types of the images
  using InputImageType = itk::Image<float, myDimension>;
  using MaskImageType = itk::Image<unsigned short, myDimension>;
  using OutputImageType = itk::Image<float, myDimension>;

  // Declare the type of the index to access images
  using myIndexType = itk::Index<myDimension>;

  // Declare the type of the size
  using mySizeType = itk::Size<myDimension>;

  // Declare the type of the Region
  using myRegionType = itk::ImageRegion<myDimension>;

  // Create two images
  auto inputImage = InputImageType::New();
  auto inputMask = MaskImageType::New();

  // Define their size, and start index
  mySizeType size;
  size[0] = 2;
  size[1] = 2;
  size[2] = 2;

  myIndexType start;
  start[0] = 0;
  start[1] = 0;
  start[2] = 0;

  const myRegionType region{ start, size };

  // Initialize the image
  inputImage->SetRegions(region);
  inputImage->Allocate();

  // Initialize the mask
  inputMask->SetRegions(region);
  inputMask->Allocate();


  // Declare Iterator types apropriated for each image
  using InputIteratorType = itk::ImageRegionIteratorWithIndex<InputImageType>;
  using MaskIteratorType = itk::ImageRegionIteratorWithIndex<MaskImageType>;
  using OutputIteratorType = itk::ImageRegionIteratorWithIndex<OutputImageType>;

  // Create one iterator for Image A (this is a light object)
  InputIteratorType inputIterator(inputImage, inputImage->GetBufferedRegion());

  // Initialize the content of Image A
  std::cout << "First operand " << std::endl;
  while (!inputIterator.IsAtEnd())
  {
    inputIterator.Set(255.0);
    std::cout << inputIterator.Get() << std::endl;
    ++inputIterator;
  }

  // Create one iterator for Image B (this is a light object)
  MaskIteratorType maskIterator(inputMask, inputMask->GetBufferedRegion());

  // Initialize the content of Image B
  // Set to mask first 2 pixels and last 2 pixels and leave the rest as is
  std::cout << "Second operand " << std::endl;
  for (unsigned int i = 0; i < 2; ++i, ++maskIterator)
  {
    maskIterator.Set(0);
  }

  while (!maskIterator.IsAtEnd())
  {
    maskIterator.Set(3);
    ++maskIterator;
  }

  for (unsigned int i = 0; i < 3; ++i, --maskIterator)
  {
    maskIterator.Set(0);
  }

  maskIterator.GoToBegin();
  while (!maskIterator.IsAtEnd())
  {
    std::cout << maskIterator.Get() << std::endl;
    ++maskIterator;
  }

  // Declare the type for the MaskNegated filter
  using myFilterType = itk::MaskNegatedImageFilter<InputImageType, MaskImageType, OutputImageType>;


  // Create an MaskNegated Filter
  auto filter = myFilterType::New();


  // Connect the input images
  filter->SetInput1(inputImage);
  filter->SetInput2(inputMask);
  filter->SetOutsideValue(50);

  // Get the Smart Pointer to the Filter Output
  const OutputImageType::Pointer outputImage = filter->GetOutput();


  // Execute the filter
  filter->Update();

  // Create an iterator for going through the image output
  OutputIteratorType outputIterator(outputImage, outputImage->GetBufferedRegion());

  // Print the content of the result image
  std::cout << " Result " << std::endl;
  while (!outputIterator.IsAtEnd())
  {
    std::cout << outputIterator.Get() << std::endl;
    ++outputIterator;
  }


  filter->Print(std::cout);

  // Test named mutator/accessors
  {
    filter->SetMaskImage(inputMask);
    const myFilterType::MaskImageType::ConstPointer retrievedMask = filter->GetMaskImage();
    if (retrievedMask != inputMask)
    {
      std::cerr << "Mask not retrieved successfully!" << std::endl;
      return EXIT_FAILURE;
    }
  }

  // Vector image tests
  using myVectorImageType = itk::VectorImage<float, myDimension>;

  auto inputVectorImage = myVectorImageType::New();
  inputVectorImage->SetRegions(region);
  inputVectorImage->SetNumberOfComponentsPerPixel(3);
  inputVectorImage->Allocate();

  using myVectorFilterType = itk::MaskNegatedImageFilter<myVectorImageType, MaskImageType, myVectorImageType>;

  auto vectorFilter = myVectorFilterType::New();
  vectorFilter->SetInput1(inputVectorImage);
  vectorFilter->SetMaskImage(inputMask);

  const myVectorImageType::PixelType outsideValue = vectorFilter->GetOutsideValue();
  ITK_TEST_EXPECT_EQUAL(outsideValue.GetSize(), 0);


  ITK_TRY_EXPECT_NO_EXCEPTION(vectorFilter->Update());

  // Check that the outside value consists of three zeros.
  const myVectorImageType::PixelType outsideValue3 = vectorFilter->GetOutsideValue();
  myVectorImageType::PixelType       threeZeros(3);
  threeZeros.Fill(0.0f);
  ITK_TEST_EXPECT_EQUAL(outsideValue3, threeZeros);

  // Reset the outside value to zero vector of length 23.
  myVectorImageType::PixelType zeros23(23);
  zeros23.Fill(1.0f);
  vectorFilter->SetOutsideValue(zeros23);

  ITK_TRY_EXPECT_EXCEPTION(vectorFilter->Update());


  // All objects should be automatically destroyed at this point
  return EXIT_SUCCESS;
}
