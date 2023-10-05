# complex

[![windows](https://github.com/BlueQuartzSoftware/complex/actions/workflows/windows.yml/badge.svg)](https://github.com/BlueQuartzSoftware/complex/actions/workflows/windows.yml) [![linux](https://github.com/BlueQuartzSoftware/complex/actions/workflows/linux.yml/badge.svg)](https://github.com/BlueQuartzSoftware/complex/actions/workflows/linux.yml) [![macos](https://github.com/BlueQuartzSoftware/complex/actions/workflows/macos.yml/badge.svg)](https://github.com/BlueQuartzSoftware/complex/actions/workflows/macos.yml) [![clang-format](https://github.com/BlueQuartzSoftware/complex/actions/workflows/format_push.yml/badge.svg)](https://github.com/BlueQuartzSoftware/complex/actions/workflows/format_push.yml) [![PR's Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg?style=flat)](http://makeapullrequest.com) [![All Contributors](https://img.shields.io/github/all-contributors/BlueQuartzSoftware/complex?color=ee8449&style=flat-square)](#contributors)

## Multilanguage README

| Language | [English](https://github.com/BlueQuartzSoftware/complex/blob/develop/README.md) | PlaceHolder |
| -------- | ------------------------------------------------------------------------------- | ----------- |

## Introduction

DREAM3D-NX is a user-friendly and versatile desktop application that leverages the open-source ‘complex’ software library to enable users to manipulate, analyze, and visualize multidimensional, multimodal data with ease. With its advanced reconstruction, quantification, meshing, data organization, and visualization capabilities, DREAM3D-NX has emerged as a go-to tool for the materials science community to reconstruct and quantify 3D microstructures. Its flexibility and adaptability make it suitable for a broad range of multidimensional data analysis applications beyond materials science and engineering domain.

`complex` is a rewrite of the [SIMPL](https://www.github.com/bluequartzsoftware/simpl) library upon which [DREAM3D](https://www.github.com/bluequartzsoftware/dream3d) is written. This library aims to be fully C++17 compliant, removes the need for Qt5 at the library level and brings more flexible data organization and grouping options. The library is under active development by BlueQuartz Software at the current time.

## Project Vision

When the [DREAM3D](https://www.github.com/bluequartzsoftware/dream3d) project began in 2009, it was created with the goal of integrating the many disperate software codes in existance at the time under a single software framework. This software framework would allow these codes to define their requirements for input data, output data and required parameters in a common structure and thus enable the existing codes to ingest data from a previous code algorithm and produce output that the other code algorithms can readily utilize. By 2013 DREAM.3D had matured into a flexible desktop application that could peform a wide variety of materials science related processing. Over time, DREAM.3D’s capabilities expanded to include more generic data processing algorithms and image processing algorithms. The United States Government funded the entire development cycle for DREAM.3D and was gracious to allow the main project to be open sourced to allow it to penetrate into as many institutions as possible. In recent years the whole of DREAM3D and SIMPL were completely rewritten into DREAM3D-NX and the `complex` library. This new version focused on developing both maintainable and expandable software that enables a lower barrier to entry for contributions.

The vision for the DREAM.3D project is to produce accessible software tools that are professionally developed, relentlessly tested, and maintained into the future thus accelerating current and future R&D efforts by building upon the contributors to the DREAM3D project.

## Public Release Notice

This software library is directly supported by USAF Contract _FA8650-22-C-5290_ and has been cleared as publicly releasable under the following:

```text
AFRL has completed the review process for your case on 14 Sep 2022:

Subject: DREAM3D_Project (O) Shah #-11  (Software Code)

Originator Reference Number: RX22-0852
Case Reviewer: M. Allen
Case Number: AFRL-2022-4403

The material was assigned a clearance of CLEARED on 14 Sep 2022.
```

This clearance is in effect until 14 SEPT 2025 at which point any new additions created under USAF funding should be cleared. Public additions through the normal PR process will not be affected.

## Get Help

The current documentation is hosted at [dream3d.io docs](http://www.dream3d.io/nx_reference_manual/Index/).

The current python binding instructions and documentation can be viewed at [dream3d.io python docs](http://www.dream3d.io/python_docs/).

See our [Support File](/SUPPORT.md) for more ways to get help!

## Building from Source

For instructions on building from source see our [Guide to Building From Source](/docs/Build_From_Source.md).

## Code of Conduct

By participating in this project, you are expected to uphold our [Code of Conduct](/CODE_OF_CONDUCT.md).

If you experience or witness unacceptable behavior—or have any other concerns—please report it by contacting the project maintainers at [info@bluequartz.net](mailto:info@bluequartz.net). All reports will be handled with discretion.

## Community

For those looking to engage with the DREAM3DNX community, see the discussions board of [DREAM3DNX-Issues discussions](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions).

## Contact Us

You can talk directly with developers in the [DREAM3DNX-Issues discussions](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions).

If this doesn't work for you you can reach out to our team at our general address [info@bluequartz.net](mailto:info@bluequartz.net).

See our [Support File](/SUPPORT.md) for more ways to contact us!

## Contribute

Thanks so much for your interest in contributing to `complex`!

The `complex` project needs all sorts of contributions, so don't be discouraged if you are unfamiliar with C++. We have all sorts of tasks that can be done without any prior coding knowledge. We need have non-coding tasks, computer power user tasks, security research tasks, Python tasks, and C++ tasks, all of which are outlined in our [Contributing Guide](/CONTRIBUTING.md).

We also understand the struggle involved with getting started on a new project and the frustration that can come with learning a new codebase. To help cut down on the stress of this we set up a discussion section specifically for [Contributor Questions](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions/categories/contributor-questions) where you can quickly get a response directly from maintainers.

The average response time for new pull requests is same-day if it's Monday-Thursday (often times less than 1 hour), but Friday-Sunday response time is variable.

We deeply appreciate any and all contributions and that will absolutely be reflected in your interactions with active maintainers.

## Contributors

Huge thanks go to these wonderful people ([emoji key](https://allcontributors.org/docs/en/emoji-key)):

<!-- ALL-CONTRIBUTORS-LIST:START - Do not remove or modify this section -->
<!-- prettier-ignore-start -->
<!-- markdownlint-disable -->
<table>
  <tbody>
    <tr>
      <td align="center" valign="top" width="14.28%"><a href="http://www.bluequartz.net"><img src="https://avatars.githubusercontent.com/u/5182396?v=4?s=100" width="100px;" alt="Michael Jackson"/><br /><sub><b>Michael Jackson</b></sub></a><br /><a href="https://github.com/BlueQuartzSoftware/complex/commits?author=imikejackson" title="Code">💻</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/JDuffeyBQ"><img src="https://avatars.githubusercontent.com/u/43142415?v=4?s=100" width="100px;" alt="Jared Duffey"/><br /><sub><b>Jared Duffey</b></sub></a><br /><a href="https://github.com/BlueQuartzSoftware/complex/commits?author=JDuffeyBQ" title="Code">💻</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/joeykleingers"><img src="https://avatars.githubusercontent.com/u/6197698?v=4?s=100" width="100px;" alt="Joey Kleingers"/><br /><sub><b>Joey Kleingers</b></sub></a><br /><a href="https://github.com/BlueQuartzSoftware/complex/commits?author=joeykleingers" title="Code">💻</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/jmarquisbq"><img src="https://avatars.githubusercontent.com/u/83971431?v=4?s=100" width="100px;" alt="Jessica Marquis"/><br /><sub><b>Jessica Marquis</b></sub></a><br /><a href="https://github.com/BlueQuartzSoftware/complex/commits?author=jmarquisbq" title="Code">💻</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/mmarineBlueQuartz"><img src="https://avatars.githubusercontent.com/u/22151460?v=4?s=100" width="100px;" alt="Matthew Marine"/><br /><sub><b>Matthew Marine</b></sub></a><br /><a href="https://github.com/BlueQuartzSoftware/complex/commits?author=mmarineBlueQuartz" title="Code">💻</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/nyoungbq"><img src="https://avatars.githubusercontent.com/u/109472155?v=4?s=100" width="100px;" alt="Nathan Young"/><br /><sub><b>Nathan Young</b></sub></a><br /><a href="https://github.com/BlueQuartzSoftware/complex/commits?author=nyoungbq" title="Code">💻</a></td>
    </tr>
  </tbody>
  <tfoot>
    <tr>
      <td align="center" size="13px" colspan="7">
        <img src="https://raw.githubusercontent.com/all-contributors/all-contributors-cli/1b8533af435da9854653492b1327a23a4dbd0a10/assets/logo-small.svg">
          <a href="https://all-contributors.js.org/docs/en/bot/usage">Add your contributions</a>
        </img>
      </td>
    </tr>
  </tfoot>
</table>

<!-- markdownlint-restore -->
<!-- prettier-ignore-end -->

<!-- ALL-CONTRIBUTORS-LIST:END -->

This project follows the [all-contributors](https://allcontributors.org) specification.
