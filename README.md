# complex #

[![windows](https://github.com/BlueQuartzSoftware/complex/actions/workflows/windows.yml/badge.svg)](https://github.com/BlueQuartzSoftware/complex/actions/workflows/windows.yml) [![linux](https://github.com/BlueQuartzSoftware/complex/actions/workflows/linux.yml/badge.svg)](https://github.com/BlueQuartzSoftware/complex/actions/workflows/linux.yml) [![macos](https://github.com/BlueQuartzSoftware/complex/actions/workflows/macos.yml/badge.svg)](https://github.com/BlueQuartzSoftware/complex/actions/workflows/macos.yml) [![clang-format](https://github.com/BlueQuartzSoftware/complex/actions/workflows/format_push.yml/badge.svg)](https://github.com/BlueQuartzSoftware/complex/actions/workflows/format_push.yml)

## License and Public Release Notice ##

This software library is directly supported by USAF Contract *FA8650-22-C-5290* and has been cleared as publicly releasable under the following:

    AFRL has completed the review process for your case on 14 Sep 2022:

    Subject: DREAM3D_Project (O) Shah #-11  (Software Code)

    Originator Reference Number: RX22-0852
    Case Reviewer: M. Allen
    Case Number: AFRL-2022-4403

    The material was assigned a clearance of CLEARED on 14 Sep 2022.

This clearance is in effect until 14 SEPT 2025 at which point any new additions created under USAF funding should be cleared. Public additions through the normal PR process will not be affected.

## Introduction ##

`complex` is a rewrite of the [SIMPL](https://www.github.com/bluequartzsoftware/simpl) library upon which [DREAM3D](https://www.github.com/bluequartzsoftware/dream3d) is written. This library aims to be fully C++17 compliant, removes the need for Qt5 at the library level and brings more flexible data organization and grouping options.
