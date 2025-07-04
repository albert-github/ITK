![ITK - The Insight Toolkit](https://raw.githubusercontent.com/InsightSoftwareConsortium/ITK/801370c025c7d296783481779a41c6d559c992c5/Documentation/Art/itkBannerSmall.png)

ITK: The Insight Toolkit
========================

[![GitHub release](https://img.shields.io/github/release/InsightSoftwareConsortium/ITK.svg)](https://github.com/InsightSoftwareConsortium/ITK/releases/latest)
[![PyPI](https://img.shields.io/pypi/v/itk.svg)](https://pypi.python.org/pypi/itk)
[![Wheels](https://img.shields.io/pypi/wheel/itk.svg)](https://pypi.org/project/itk)
[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://github.com/InsightSoftwareConsortium/ITK/blob/main/LICENSE)
[![DOI](https://zenodo.org/badge/800928.svg)](https://zenodo.org/badge/latestdoi/800928)
[![Powered by NumFOCUS](https://img.shields.io/badge/powered%20by-NumFOCUS-orange.svg?style=flat&colorA=E1523D&colorB=007D8A)](https://numfocus.org)

| | | C++ | Python |
|:------:|:------:|:--------:|:--------:|
| Azure Pipelines | Linux | [![Build Status](https://dev.azure.com/itkrobotlinux/ITK.Linux/_apis/build/status/ITK.Linux?branchName=main)](https://dev.azure.com/itkrobotlinux/ITK.Linux/_build/latest?definitionId=2&branchName=main) | [![Build Status](https://dev.azure.com/itkrobotlinuxpython/ITK.Linux.Python/_apis/build/status/ITK.Linux.Python?branchName=main)](https://dev.azure.com/itkrobotlinuxpython/ITK.Linux.Python/_build/latest?definitionId=3&branchName=main) |
| Azure Pipelines | Windows | [![Build Status](https://dev.azure.com/itkrobotwindow/ITK.Windows/_apis/build/status/ITK.Windows?branchName=main)](https://dev.azure.com/itkrobotwindow/ITK.Windows/_build/latest?definitionId=2&branchName=main) | [![Build Status](https://dev.azure.com/itkrobotwindowpython/ITK.Windows.Python/_apis/build/status/ITK.Windows.Python?branchName=main)](https://dev.azure.com/itkrobotwindowpython/ITK.Windows.Python/_build/latest?definitionId=1) |
| Azure Pipelines | macOS | [![Build Status](https://dev.azure.com/itkrobotmacos/ITK.macOS/_apis/build/status/ITK.macOS?branchName=main)](https://dev.azure.com/itkrobotmacos/ITK.macOS/_build/latest?definitionId=2&branchName=main) | [![Build Status](https://dev.azure.com/itkrobotmacospython/ITK.macOS.Python/_apis/build/status/ITK.macOS.Python?branchName=main)](https://dev.azure.com/itkrobotmacospython/ITK.macOS.Python/_build/latest?definitionId=2&branchName=main) |
| GitHub Actions | Linux, Windows, macOS | [![ITK.Pixi](https://github.com/InsightSoftwareConsortium/ITK/actions/workflows/pixi.yml/badge.svg)](https://github.com/InsightSoftwareConsortium/ITK/actions/workflows/pixi.yml) | |
| GitHub Actions | macOS (Apple Silicon)|  [![ITK.macOS.Arm64](https://github.com/InsightSoftwareConsortium/ITK/actions/workflows/macos-arm.yml/badge.svg)](https://github.com/InsightSoftwareConsortium/ITK/actions/workflows/macos-arm.yml)| |
| Azure Pipelines | Linux (Code coverage)| [![Build Status](https://dev.azure.com/itkrobotbatch/ITK.Coverage/_apis/build/status/ITK.Coverage?branchName=main)](https://dev.azure.com/itkrobotbatch/ITK.Coverage/_build/latest?definitionId=3&branchName=main) | |

Links
-----

* [Homepage](https://itk.org)
* [Download](https://docs.itk.org/en/latest/download.html)
* [Documentation](https://docs.itk.org/)
* [Discussion](https://discourse.itk.org/)
* [Software Guide](https://itk.org/ItkSoftwareGuide.pdf)
* [Examples](https://examples.itk.org/)
* [Issue tracking](https://github.com/InsightSoftwareConsortium/ITK/issues)
* [Submit a patch](CONTRIBUTING.md)

*Note*: For questions related to ITK, please use the official [Discussion](https://discourse.itk.org/)
space: the [issue tracker](https://github.com/InsightSoftwareConsortium/ITK/issues)
is reserved to track different aspects of the software development process, as
highlighted by the available templates.


About
-----

The Insight Toolkit (ITK) is an open-source, cross-platform toolkit for
N-dimensional scientific image processing, segmentation, and registration.
Segmentation is the process of identifying and classifying data found in a
digitally sampled representation. Typically the sampled representation is an
image acquired from such medical instrumentation as CT or MRI scanners.
Registration is the task of aligning or developing correspondences between
data. For example, in the medical environment, a CT scan may be aligned with a
MRI scan in order to combine the information contained in both.

[//]: # (numfocus-fiscal-sponsor-attribution)

The ITK project uses an [open governance model](./GOVERNANCE.md)
and is fiscally sponsored by [NumFOCUS](https://numfocus.org/). Consider making
a [tax-deductible donation](https://opencollective.com/itk) to help the project
pay for developer time, professional services, travel, workshops, and a variety of other needs.

<div align="center">
  <a href="https://numfocus.org/project/itk">
    <img height="60px"
         src="https://raw.githubusercontent.com/numfocus/templates/main/images/numfocus-logo.png"
         align="center">
  </a>
</div>
<br>

ITK is distributed in binary Python packages. To install:

```bash
pip install itk
```

or

```bash
conda install -c conda-forge itk
```

The cross-platform, C++ core of the toolkit may be built from source using
[CMake](https://cmake.org).

Copyright
---------

[NumFOCUS](https://numfocus.org/) holds the copyright of this software.
NumFOCUS is a non-profit entity that promotes the use of open source
scientific software for educational and research purposes.  NumFOCUS delegates
project governance to the [Insight Software
Consortium](https://www.insightsoftwareconsortium.org) Council, an educational
consortium dedicated to promoting and maintaining open-source, freely
available software for medical image analysis. This includes promoting such
software in teaching, research, and commercial applications, and maintaining
webpages and user and developer communities.  ITK is distributed under a
license that enables use for both non-commercial and commercial applications.
See
[LICENSE](https://github.com/InsightSoftwareConsortium/ITK/blob/main/LICENSE)
and
[NOTICE](https://github.com/InsightSoftwareConsortium/ITK/blob/main/NOTICE)
files for details.

Supporting ITK
--------------

ITK is a fiscally sponsored project of [NumFOCUS](https://numfocus.org/), a non-profit dedicated
to supporting the open source scientific computing community. If you want to
support ITK's mission to develop and maintain open-source, reproducible
scientific image analysis software for education and research, please consider
making a [donation](https://opencollective.com/itk) to support our efforts.

NumFOCUS is 501(c)(3) non-profit charity in the United States; as such,
donations to NumFOCUS are tax-deductible as allowed by law. As with any
donation, you should consult with your personal tax adviser or the IRS about
your particular tax situation.

Professional Services
---------------------

[Kitware](https://www.kitware.com/) provides professional services for ITK,
including custom solution creation, collaborative research and development,
development support, and training.

Citation
--------

To cite ITK, please reference, as appropriate:

### The papers

> McCormick M, Liu X, Jomier J, Marion C, Ibanez L. ITK: enabling reproducible research and open science. Front Neuroinform. 2014;8:13. Published 2014 Feb 20. doi:10.3389/fninf.2014.00013

> Yoo TS, Ackerman MJ, Lorensen WE, Schroeder W, Chalana V, Aylward S, Metaxas D, Whitaker R. Engineering and Algorithm Design for an Image Processing API: A Technical Report on ITK – The Insight Toolkit. In Proc. of Medicine Meets Virtual Reality, J. Westwood, ed., IOS Press Amsterdam pp 586-592 (2002).

### The books

> Johnson, McCormick, Ibanez. "The ITK Software Guide: Design and Functionality." Fourth Edition. Published by Kitware, Inc. 2015 ISBN: 9781-930934-28-3.

> Johnson, McCormick, Ibanez. "The ITK Software Guide: Introduction and Development Guidelines." Fourth Edition. Published by Kitware, Inc. 2015 ISBN: 9781-930934-27-6.

### Specific software version

[![DOI](https://zenodo.org/badge/800928.svg)](https://zenodo.org/badge/latestdoi/800928)

Once your work has been published, please create a pull request to add
the publication to the
[ITKBibliography.bib](https://github.com/InsightSoftwareConsortium/insightsoftwareconsortium.org/blob/main/static/citations-visualization/ITKBibliography.bib)
file.

[![Alt](https://repobeats.axiom.co/api/embed/e2194d411e086b1a9a9db4336692fdb302612295.svg "Repobeats analytics image")](https://github.com/InsightSoftwareConsortium/ITK/pulse/monthly)
