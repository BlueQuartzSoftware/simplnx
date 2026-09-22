#!/usr/bin/env python3
"""Generate deterministic paper-inspired inputs for ImageProcessing examples."""

from __future__ import annotations

import argparse
import hashlib
from io import BytesIO
import json
from pathlib import Path
import re
import shutil
import zipfile

import numpy as np
from PIL import Image, ImageDraw, TiffImagePlugin
from scipy.io import loadmat
from scipy.ndimage import gaussian_filter


DEFAULT_SEED = 20260901
TARGETS_PATH = Path(__file__).with_name("PaperInputTargets.json")


PIPELINE_FILES = {
    "(05) AM XCT Porosity Segmentation": "(05) AM XCT Porosity Segmentation.d3dpipeline",
    "(06) AM Powder-Bed Layer Inspection": "(06) AM Powder-Bed Layer Inspection.d3dpipeline",
    "(07) AM Melt-Pool and Track Inspection": "(07) AM Melt-Pool and Track Inspection.d3dpipeline",
    "(08) Powder Particle Watershed Segmentation": "(08) Powder Particle Watershed Segmentation.d3dpipeline",
    "(09) Microstructure Watershed Segmentation": "(09) Microstructure Watershed Segmentation.d3dpipeline",
    "(10) Binary Mask Repair and Skeletonization": "(10) Binary Mask Repair and Skeletonization.d3dpipeline",
    "(11) Grayscale Surface-Defect Morphology": "(11) Grayscale Surface-Defect Morphology.d3dpipeline",
    "(12) Pore Distance and Wall-Thickness Metrology": "(12) Pore Distance and Wall-Thickness Metrology.d3dpipeline",
    "(13) Denoising and Edge Detection": "(13) Denoising and Edge Detection.d3dpipeline",
    "(14) Projection-Based Quality Summaries": "(14) Projection-Based Quality Summaries.d3dpipeline",
    "(15) Radiography Intensity Calibration": "(15) Radiography Intensity Calibration.d3dpipeline",
    "(16) Phase and Angle Field Transforms": "(16) Phase and Angle Field Transforms.d3dpipeline",
    "(19) Serial-Section Stack Reconstruction": "(19) Serial-Section Stack Reconstruction.d3dpipeline",
}


ASSET_GROUPS = {
    "am/xct_porosity.mha": ["am/xct_porosity.mha"],
    "am/powder_bed_layers": [f"am/powder_bed_layers/layer_{index:03d}.tif" for index in range(5)],
    "am/melt_pool_track.tif": ["am/melt_pool_track.tif"],
    "materials/powder_particles.png": ["materials/powder_particles.png"],
    "materials/microstructure_grayscale.png": ["materials/microstructure_grayscale.png"],
    "materials/binary_mask.png": ["materials/binary_mask.png"],
    "materials/grayscale_defects.png": ["materials/grayscale_defects.png"],
    "materials/edge_input.png": ["materials/edge_input.png"],
    "am/calibration/transmission.mha": [
        "am/calibration/transmission.mha",
        "am/calibration/residual.mha",
        "am/calibration/mask.mha",
    ],
    "am/calibration/phase_radians.mha": [
        "am/calibration/phase_radians.mha",
        "am/calibration/normalized_response.mha",
    ],
    "io/serial_stack": [f"io/serial_stack/section_{index:03d}.tif" for index in range(8)],
}


FILE_PROPERTIES = {
    "am/xct_porosity.mha": ("uint16", [128, 128, 48], [0.035, 0.035, 0.035], "millimeter"),
    "am/melt_pool_track.tif": ("uint8", [360, 127, 1], [0.0333, 0.0472, 1.0], "millimeter"),
    "materials/powder_particles.png": ("uint8", [512, 512, 1], [1.0, 1.0, 1.0], "pixel"),
    "materials/microstructure_grayscale.png": ("uint8", [604, 604, 1], [1.0, 1.0, 1.0], "pixel"),
    "materials/binary_mask.png": ("uint8", [512, 384, 1], [1.0, 1.0, 1.0], "pixel"),
    "materials/grayscale_defects.png": ("uint8", [512, 256, 1], [1.0, 1.0, 1.0], "pixel"),
    "materials/edge_input.png": ("uint8", [512, 384, 1], [1.0, 1.0, 1.0], "pixel"),
    "am/calibration/transmission.mha": ("float32", [512, 256, 1], [0.05, 0.05, 1.0], "millimeter"),
    "am/calibration/residual.mha": ("float32", [512, 256, 1], [0.05, 0.05, 1.0], "millimeter"),
    "am/calibration/mask.mha": ("uint8", [512, 256, 1], [0.05, 0.05, 1.0], "millimeter"),
    "am/calibration/phase_radians.mha": ("float32", [512, 256, 1], [0.05, 0.05, 1.0], "radian"),
    "am/calibration/normalized_response.mha": ("float32", [512, 256, 1], [0.05, 0.05, 1.0], "unitless"),
}
for layer_index in range(5):
    FILE_PROPERTIES[f"am/powder_bed_layers/layer_{layer_index:03d}.tif"] = ("uint8", [512, 256, 1], [0.05, 0.05, 0.05], "millimeter")
for section_index in range(8):
    FILE_PROPERTIES[f"io/serial_stack/section_{section_index:03d}.tif"] = ("uint8 RGB", [256, 256, 1], [0.5, 0.5, 1.0], "micrometer")


def ensure_parent(path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)


def save_image(path: Path, array: np.ndarray) -> None:
    ensure_parent(path)
    image = Image.fromarray(array)
    if path.suffix.lower() in {".tif", ".tiff"} and image.mode == "L":
        tiff_info = TiffImagePlugin.ImageFileDirectory_v2()
        tiff_info[277] = 1
        image.save(path, tiffinfo=tiff_info)
    else:
        image.save(path)


def write_mha(path: Path, array: np.ndarray, spacing: tuple[float, float, float]) -> None:
    if array.ndim == 2:
        array = array[np.newaxis, :, :]
    if array.ndim != 3:
        raise ValueError(f"MHA array must be 2-D or 3-D, got {array.shape}")
    element_types = {
        np.dtype("uint8"): "MET_UCHAR",
        np.dtype("uint16"): "MET_USHORT",
        np.dtype("int16"): "MET_SHORT",
        np.dtype("float32"): "MET_FLOAT",
    }
    dtype = np.dtype(array.dtype)
    if dtype not in element_types:
        raise ValueError(f"Unsupported MHA type: {dtype}")
    z_size, y_size, x_size = array.shape
    header = "\n".join(
        [
            "ObjectType = Image",
            "NDims = 3",
            "BinaryData = True",
            "BinaryDataByteOrderMSB = False",
            "CompressedData = False",
            "TransformMatrix = 1 0 0 0 1 0 0 0 1",
            "Offset = 0 0 0",
            "CenterOfRotation = 0 0 0",
            f"ElementSpacing = {spacing[0]} {spacing[1]} {spacing[2]}",
            f"DimSize = {x_size} {y_size} {z_size}",
            f"ElementType = {element_types[dtype]}",
            "ElementDataFile = LOCAL",
            "",
        ]
    ).encode("ascii")
    ensure_parent(path)
    path.write_bytes(header + np.ascontiguousarray(array).tobytes(order="C"))


def generate_xct_porosity(root: Path, rng: np.random.Generator) -> dict:
    shape = (48, 128, 128)
    z_grid, y_grid, x_grid = np.indices(shape)
    center_x = (shape[2] - 1) / 2
    center_y = (shape[1] - 1) / 2
    radial = np.sqrt((x_grid - center_x) ** 2 + (y_grid - center_y) ** 2)
    coupon = np.ones(shape, dtype=bool)
    material = 47000.0 - radial * 55.0 + z_grid * 8.0
    material += gaussian_filter(rng.normal(0.0, 850.0, shape), sigma=(0.7, 0.7, 0.7))
    volume = material.copy()

    void_mask = np.zeros(shape, dtype=bool)
    pores = []
    for _ in range(18):
        z0 = rng.uniform(5, shape[0] - 5)
        y0 = rng.uniform(center_y - 37, center_y + 37)
        x0 = rng.uniform(center_x - 37, center_x + 37)
        radii = rng.uniform([1.5, 1.8, 1.8], [4.0, 5.0, 5.0])
        pores.append((z0, y0, x0, *radii))
    pores.extend(
        [
            (13.0, center_y - 18, center_x + 8, 1.5, 3.0, 12.0),
            (31.0, center_y + 20, center_x - 14, 1.2, 10.0, 3.0),
        ]
    )
    for z0, y0, x0, rz, ry, rx in pores:
        ellipsoid = ((z_grid - z0) / rz) ** 2 + ((y_grid - y0) / ry) ** 2 + ((x_grid - x0) / rx) ** 2 <= 1.0
        ellipsoid &= coupon
        void_mask |= ellipsoid
        volume[ellipsoid] = rng.normal(2500.0, 350.0, int(ellipsoid.sum()))

    ring = np.sin(radial * 0.55) * 180.0
    volume[coupon] += ring[coupon]
    output = np.clip(volume, 0, 65535).astype(np.uint16)
    write_mha(root / "am/xct_porosity.mha", output, (0.035, 0.035, 0.035))
    return {
        "dimensions": [128, 128, 48],
        "void_fraction": float(void_mask.sum() / coupon.sum()),
        "dark_component_count": len(pores),
        "target": "Cropped AM coupon-interior XCT cross-sections with rounded gas pores and elongated lack-of-fusion voids",
    }


def generate_powder_bed(root: Path, rng: np.random.Generator) -> dict:
    y_size, x_size = 256, 512
    y_grid, x_grid = np.indices((y_size, x_size))
    layer_metrics = []
    for layer in range(5):
        low_frequency = gaussian_filter(rng.normal(0.0, 1.0, (y_size, x_size)), sigma=18)
        texture = rng.normal(0.0, 9.0, (y_size, x_size))
        illumination = 112.0 + 0.055 * x_grid - 0.035 * y_grid + 20.0 * low_frequency
        image = illumination + texture
        image += 7.0 * np.sin((y_grid + layer * 4) / 5.5)

        streak_y = 42 + layer * 31
        image[max(0, streak_y - 2) : min(y_size, streak_y + 3), 35:480] += 55
        bare_x0 = 90 + layer * 48
        image[145:205, bare_x0 : bare_x0 + 55] -= 48
        for _ in range(9):
            cy = int(rng.integers(20, y_size - 20))
            cx = int(rng.integers(20, x_size - 20))
            radius = int(rng.integers(3, 9))
            disk = (y_grid - cy) ** 2 + (x_grid - cx) ** 2 <= radius**2
            image[disk] += rng.uniform(45, 85)
        output = np.clip(image, 0, 255).astype(np.uint8)
        save_image(root / f"am/powder_bed_layers/layer_{layer:03d}.tif", output)
        layer_metrics.append(float(output.std()))
    return {
        "dimensions": [512, 256, 5],
        "layer_count": 5,
        "mean_layer_standard_deviation": float(np.mean(layer_metrics)),
        "target": "Obliquely illuminated LPBF powder layers with recoater streaks, bare patches, and bright agglomerates",
    }


def generate_powder_particles(root: Path, rng: np.random.Generator) -> dict:
    y_size, x_size = 512, 512
    y_grid, x_grid = np.indices((y_size, x_size))
    image = 24.0 + gaussian_filter(rng.normal(0, 5, (y_size, x_size)), 0.7)
    particles = []
    touching_pairs = 0
    for index in range(58):
        if index and index % 5 == 0:
            previous = particles[-1]
            radius = rng.uniform(10, 20)
            angle = rng.uniform(0, 2 * np.pi)
            distance = previous[4] + radius - rng.uniform(2, 7)
            cx = np.clip(previous[0] + np.cos(angle) * distance, 24, x_size - 24)
            cy = np.clip(previous[1] + np.sin(angle) * distance, 24, y_size - 24)
            touching_pairs += 1
        else:
            cx = rng.uniform(24, x_size - 24)
            cy = rng.uniform(24, y_size - 24)
            radius = rng.uniform(9, 22)
        rx = radius * rng.uniform(0.82, 1.18)
        ry = radius * rng.uniform(0.82, 1.18)
        theta = rng.uniform(0, np.pi)
        particles.append((cx, cy, rx, ry, radius, theta))
        cos_t, sin_t = np.cos(theta), np.sin(theta)
        local_x = (x_grid - cx) * cos_t + (y_grid - cy) * sin_t
        local_y = -(x_grid - cx) * sin_t + (y_grid - cy) * cos_t
        normalized_radius = np.sqrt((local_x / rx) ** 2 + (local_y / ry) ** 2)
        mask = normalized_radius <= 1
        shading = 205 - 32 * normalized_radius + 18 * (local_x / max(rx, 1))
        image[mask] = np.maximum(image[mask], shading[mask] + rng.normal(0, 4, int(mask.sum())))
        rim = (normalized_radius > 0.88) & (normalized_radius <= 1.02)
        image[rim] = np.maximum(image[rim], 230)
    output = np.clip(image, 0, 255).astype(np.uint8)
    save_image(root / "materials/powder_particles.png", output)
    return {
        "dimensions": [512, 512, 1],
        "particle_count": len(particles),
        "touching_pair_count": touching_pairs,
        "target": "Metal-powder micrograph with irregular shaded particles, broad size distribution, and touching clusters",
    }


def draw_polyline(draw: ImageDraw.ImageDraw, points: list[tuple[int, int]], width: int, fill: int) -> None:
    draw.line(points, fill=fill, width=width, joint="curve")


def generate_binary_cracks(root: Path, rng: np.random.Generator) -> dict:
    image = Image.new("L", (512, 384), 0)
    draw = ImageDraw.Draw(image)
    trunks = [
        [(15, 65), (105, 78), (170, 120), (245, 138), (330, 200), (495, 215)],
        [(82, 8), (105, 78), (92, 165), (135, 250), (120, 375)],
        [(310, 5), (295, 80), (245, 138), (225, 245), (270, 375)],
    ]
    branches = [
        [(170, 120), (210, 72), (265, 52)],
        [(92, 165), (45, 205), (10, 255)],
        [(135, 250), (185, 280), (205, 342)],
        [(330, 200), (380, 145), (430, 120)],
        [(330, 200), (355, 275), (420, 330)],
        [(225, 245), (165, 220), (130, 195)],
        [(295, 80), (350, 65), (405, 30)],
    ]
    for points in trunks + branches:
        jittered = [(x + int(rng.integers(-4, 5)), y + int(rng.integers(-4, 5))) for x, y in points]
        draw_polyline(draw, jittered, int(rng.integers(2, 6)), 1)
    for _ in range(45):
        x = int(rng.integers(5, 507))
        y = int(rng.integers(5, 379))
        draw.point((x, y), fill=1)
    array = np.asarray(image, dtype=np.uint8).copy()
    array[73:78, 100:108] = 0
    array[196:202, 326:337] = 0
    save_image(root / "materials/binary_mask.png", array)
    return {
        "dimensions": [512, 384, 1],
        "foreground_fraction": float(array.mean()),
        "branch_count": len(branches),
        "target": "Thresholded interconnected crack network with branches, variable width, small gaps, and isolated segmentation specks",
    }


def generate_surface_defects(root: Path, rng: np.random.Generator) -> dict:
    y_size, x_size = 256, 512
    y_grid, x_grid = np.indices((y_size, x_size))
    texture = 130 + 10 * np.sin(y_grid / 5.5) + 7 * np.sin(x_grid / 19)
    texture += gaussian_filter(rng.normal(0, 18, (y_size, x_size)), sigma=(1.0, 2.5))
    image = np.clip(texture, 0, 255).astype(np.uint8)
    pil_image = Image.fromarray(image)
    draw = ImageDraw.Draw(pil_image)
    for y in (48, 126, 198):
        draw.line([(15, y), (495, y + int(rng.integers(-8, 9)))], fill=42, width=int(rng.integers(2, 5)))
    for _ in range(13):
        cx = int(rng.integers(20, x_size - 20))
        cy = int(rng.integers(20, y_size - 20))
        rx = int(rng.integers(3, 10))
        ry = int(rng.integers(2, 7))
        fill = int(rng.choice([35, 225]))
        draw.ellipse((cx - rx, cy - ry, cx + rx, cy + ry), fill=fill)
    output = np.asarray(pil_image, dtype=np.uint8)
    save_image(root / "materials/grayscale_defects.png", output)
    return {
        "dimensions": [512, 256, 1],
        "dark_defect_contrast": float(np.median(output) - np.percentile(output, 2)),
        "bright_defect_contrast": float(np.percentile(output, 98) - np.median(output)),
        "target": "Scale-covered steel surface with rolling texture, long dark defects, pits, and bright inclusions",
    }


def generate_ultrasound(root: Path, rng: np.random.Generator) -> dict:
    y_size, x_size = 384, 512
    y_grid, x_grid = np.indices((y_size, x_size))
    depth_gain = 72 + 0.12 * y_grid
    speckle = rng.rayleigh(0.75, (y_size, x_size))
    tissue = depth_gain * speckle
    tissue = gaussian_filter(tissue, sigma=(0.7, 0.45))
    cx, cy, rx, ry = 265, 205, 118, 55
    distance = np.sqrt(((x_grid - cx) / rx) ** 2 + ((y_grid - cy) / ry) ** 2)
    lumen = distance < 0.82
    wall = (distance >= 0.82) & (distance < 1.06)
    tissue[lumen] *= 0.12
    tissue[wall] += 115 * np.exp(-((distance[wall] - 0.93) / 0.07) ** 2)
    shadow = (x_grid > 120) & (x_grid < 155) & (y_grid > 75)
    tissue[shadow] *= 0.55
    output = np.clip(tissue, 0, 255).astype(np.uint8)
    save_image(root / "materials/edge_input.png", output)
    tissue_region = (distance > 1.25) & (y_grid > 70)
    return {
        "dimensions": [512, 384, 1],
        "tissue_speckle_cv": float(output[tissue_region].std() / max(output[tissue_region].mean(), 1)),
        "lumen_mean": float(output[lumen].mean()),
        "wall_mean": float(output[wall].mean()),
        "target": "Long-axis vascular ultrasound with speckled tissue, dark lumen, bright paired walls, depth gain, and acoustic shadow",
    }


def generate_calibration_fields(root: Path, rng: np.random.Generator) -> dict:
    y_size, x_size = 256, 512
    y_grid, x_grid = np.indices((y_size, x_size))
    beam = 0.94 - 0.13 * ((x_grid - x_size / 2) / (x_size / 2)) ** 2
    stripes = 0.025 * np.sin(x_grid / 7.5) + 0.012 * np.sin(x_grid / 2.3)
    attenuation = np.zeros((y_size, x_size), dtype=float)
    for cx, cy, rx, ry, value in [
        (145, 125, 72, 90, 1.1),
        (300, 130, 52, 72, 0.65),
        (405, 132, 34, 48, 1.7),
    ]:
        mask = ((x_grid - cx) / rx) ** 2 + ((y_grid - cy) / ry) ** 2 <= 1
        attenuation[mask] += value
    step_wedge = (x_grid > 35) & (x_grid < 95) & (y_grid > 35) & (y_grid < 220)
    attenuation[step_wedge] += ((y_grid[step_wedge] - 35) // 30) * 0.2
    transmission = beam * np.exp(-attenuation) + stripes
    transmission += rng.normal(0, 0.008, (y_size, x_size))
    transmission = np.clip(transmission, 0.02, 1.0).astype(np.float32)
    residual = (stripes + gaussian_filter(rng.normal(0, 0.02, (y_size, x_size)), 2)).astype(np.float32)
    valid_mask = np.ones((y_size, x_size), dtype=np.uint8)
    valid_mask[:8, :] = 0
    valid_mask[-8:, :] = 0
    valid_mask[:, :10] = 0
    valid_mask[:, -10:] = 0
    valid_mask[80:95, 248:256] = 0

    phase = (0.8 * (x_grid - x_size / 2) / (x_size / 2) + 0.55 * np.exp(-(((x_grid - 305) / 70) ** 2 + ((y_grid - 128) / 52) ** 2))).astype(np.float32)
    normalized = np.sin(phase).astype(np.float32)
    write_mha(root / "am/calibration/transmission.mha", transmission, (0.05, 0.05, 1.0))
    write_mha(root / "am/calibration/residual.mha", residual, (0.05, 0.05, 1.0))
    write_mha(root / "am/calibration/mask.mha", valid_mask, (0.05, 0.05, 1.0))
    write_mha(root / "am/calibration/phase_radians.mha", phase, (0.05, 0.05, 1.0))
    write_mha(root / "am/calibration/normalized_response.mha", normalized, (0.05, 0.05, 1.0))
    return {
        "dimensions": [512, 256, 1],
        "dynamic_range": float(transmission.max() - transmission.min()),
        "stripe_amplitude": float(np.std(np.mean(transmission, axis=0) - gaussian_filter(np.mean(transmission, axis=0), 20))),
        "target": "Flat-field-normalized X-ray projection with beam profile, step wedge, attenuating specimens, detector stripes, and invalid pixels",
    }


def generate_serial_sections(root: Path, rng: np.random.Generator) -> dict:
    y_size, x_size = 256, 256
    y_grid, x_grid = np.indices((y_size, x_size))
    seed_count = 38
    centers = np.column_stack((rng.uniform(0, x_size, seed_count), rng.uniform(0, y_size, seed_count)))
    intensities = rng.integers(55, 205, seed_count)
    slices = []
    for z_index in range(8):
        shift = np.column_stack((2.2 * np.sin(z_index / 2 + np.arange(seed_count)), 1.7 * np.cos(z_index / 2 + np.arange(seed_count))))
        shifted = centers + shift
        distances = (x_grid[..., None] - shifted[:, 0]) ** 2 + (y_grid[..., None] - shifted[:, 1]) ** 2
        labels = np.argmin(distances, axis=2)
        grayscale = intensities[labels].astype(float)
        boundary = np.zeros((y_size, x_size), dtype=bool)
        boundary[:, 1:] |= labels[:, 1:] != labels[:, :-1]
        boundary[1:, :] |= labels[1:, :] != labels[:-1, :]
        grayscale[boundary] = 235
        grayscale += gaussian_filter(rng.normal(0, 5, (y_size, x_size)), 0.7)
        gray_u8 = np.clip(grayscale, 0, 255).astype(np.uint8)
        rgb = np.stack((gray_u8, np.clip(gray_u8 * 0.96 + 7, 0, 255), np.clip(gray_u8 * 0.90 + 12, 0, 255)), axis=2).astype(np.uint8)
        save_image(root / f"io/serial_stack/section_{z_index:03d}.tif", rgb)
        slices.append(gray_u8.astype(float))
    correlations = [np.corrcoef(slices[index].ravel(), slices[index + 1].ravel())[0, 1] for index in range(len(slices) - 1)]
    return {
        "slice_count": len(slices),
        "dimensions": [256, 256, len(slices)],
        "adjacent_slice_correlation": float(np.mean(correlations)),
        "target": "Registered serial metallography sections with persistent grains, bright boundaries, small section-to-section motion, and acquisition noise",
    }


def generate_synthetic_assets(root: Path, seed: int = DEFAULT_SEED) -> dict:
    root.mkdir(parents=True, exist_ok=True)
    rng = np.random.default_rng(seed)
    summaries = {
        "am/xct_porosity.mha": generate_xct_porosity(root, rng),
        "am/powder_bed_layers": generate_powder_bed(root, rng),
        "materials/powder_particles.png": generate_powder_particles(root, rng),
        "materials/binary_mask.png": generate_binary_cracks(root, rng),
        "materials/grayscale_defects.png": generate_surface_defects(root, rng),
        "materials/edge_input.png": generate_ultrasound(root, rng),
        "am/calibration/transmission.mha": generate_calibration_fields(root, rng),
        "io/serial_stack": generate_serial_sections(root, rng),
    }
    metrics_path = root / "PaperInputMetrics.json"
    metrics_path.write_text(json.dumps({"schema_version": 1, "seed": seed, "assets": summaries}, indent=2) + "\n", encoding="utf-8")
    return summaries


def derive_nist_melt_pool(nist_zip: Path, output_path: Path) -> dict:
    with zipfile.ZipFile(nist_zip) as archive:
        camera_name = next(name for name in archive.namelist() if name.endswith("_CameraSignal.mat"))
        camera_signal = loadmat(BytesIO(archive.read(camera_name)))["CameraSignal"]
    frame_index = int(np.argmax(camera_signal.max(axis=(0, 1))))
    frame = camera_signal[:, :, frame_index].astype(float)
    low = float(np.percentile(frame, 40))
    high = float(np.percentile(frame, 99.9))
    output = np.clip((frame - low) / max(high - low, 1) * 255, 0, 255).astype(np.uint8)
    save_image(output_path, output)
    return {
        "dimensions": [int(output.shape[1]), int(output.shape[0]), 1],
        "source_frame": frame_index,
        "source_camera_minimum": int(frame.min()),
        "source_camera_maximum": int(frame.max()),
        "display_percentiles": [40.0, 99.9],
    }


def derive_ti_microstructure(source_image: Path, output_path: Path) -> dict:
    image = Image.open(source_image).convert("L")
    array = np.asarray(image, dtype=np.uint8)
    save_image(output_path, array)
    return {
        "dimensions": [array.shape[1], array.shape[0], 1],
        "intensity_minimum": int(array.min()),
        "intensity_maximum": int(array.max()),
    }


def summarize_generated_assets(root: Path) -> dict:
    return json.loads((root / "PaperInputMetrics.json").read_text(encoding="utf-8"))["assets"]


def sha512(path: Path) -> str:
    return hashlib.sha512(path.read_bytes()).hexdigest()


def update_archive_readme(root: Path) -> None:
    readme = root / "README.md"
    if not readme.exists():
        return
    readme.write_text(
        """# ImageProcessing Example Data

This directory contains the inputs for the DREAM3D-NX ImageProcessing real-world example pipelines.

The source priority is original paper data, compatible paper-linked data, then deterministic paper-inspired synthesis. `Provenance.json` records the source choice, license, generator, seed, paper visual target, metrics, checksums, and known differences for each asset.

## Data classes

- `am/` contains one measured-derived NIST thermography frame plus paper-inspired XCT, powder-bed, radiography, and phase fixtures.
- `materials/` contains one paper-linked Ti-6Al-4V micrograph plus paper-inspired powder, crack, surface-defect, and ultrasound fixtures.
- `io/` contains tested format fixtures, measured Fiji and Zeiss data, and paper-inspired serial metallography sections.
- `Generators/` contains the exact generator and target contract used for synthetic and derived data.

Generated data is not experimental measurement data. Measured-derived and paper-linked data retain their original attribution and source checksum. Synthetic data is labeled `synthetic-paper-inspired` and cannot support an `exact` reproduction claim.

## Archive rules

- Keep `ImageProcessing_Examples_v1.tar.gz` immutable after publication.
- Create a new versioned archive when a published input changes.
- Verify the SHA-512 checksum before extraction.
- Extract the archive so this directory appears at `Data/ImageProcessing_Examples`.
""",
        encoding="utf-8",
    )


def update_provenance(root: Path) -> None:
    targets = json.loads(TARGETS_PATH.read_text(encoding="utf-8"))
    metrics = summarize_generated_assets(root)
    generator_dir = root / "Generators"
    generator_dir.mkdir(parents=True, exist_ok=True)
    archived_generator = generator_dir / Path(__file__).name
    archived_targets = generator_dir / TARGETS_PATH.name
    shutil.copy2(Path(__file__), archived_generator)
    shutil.copy2(TARGETS_PATH, archived_targets)
    generator_sha512 = sha512(archived_generator)

    provenance_path = root / "Provenance.json"
    if provenance_path.exists():
        provenance = json.loads(provenance_path.read_text(encoding="utf-8"))
    else:
        provenance = {"version": 1, "generated_on": "", "archive": "ImageProcessing_Examples_v1.tar.gz", "assets": []}
    managed_paths = {path for paths in ASSET_GROUPS.values() for path in paths}
    managed_paths.update(
        {
            "PaperInputMetrics.json",
            "Generators/GenerateImageProcessingExampleData.py",
            "Generators/PaperInputTargets.json",
        }
    )
    preserved_assets = [asset for asset in provenance.get("assets", []) if asset.get("path") not in managed_paths]

    asset_targets: dict[str, list[tuple[str, dict]]] = {}
    for pipeline_name, target in targets["pipelines"].items():
        if pipeline_name not in PIPELINE_FILES:
            continue
        for asset_key in target["asset_keys"]:
            if asset_key not in ASSET_GROUPS:
                continue
            for relative_path in ASSET_GROUPS[asset_key]:
                asset_targets.setdefault(relative_path, []).append((pipeline_name, target))

    updated_assets = []
    for relative_path in sorted(managed_paths):
        output_path = root / relative_path
        if not output_path.is_file():
            continue
        target_pairs = asset_targets.get(relative_path, [])
        if not target_pairs:
            continue
        _, primary = target_pairs[0]
        is_nist = relative_path == "am/melt_pool_track.tif"
        is_ti = relative_path == "materials/microstructure_grayscale.png"
        if is_nist:
            kind = "measured-derived-paper-linked"
            source = primary["source_url"] + "::20170213_PowderPlate1_SingleLine_CameraSignal.mat"
            source_hash = primary["source_sha512"]
            license_value = "NIST public data terms; review the dataset record for third-party restrictions"
            transform = "Selected frame 20 with the largest camera-signal maximum and percentile-scaled the 40th to 99.9th percentile range to uint8."
        elif is_ti:
            kind = "measured-derived-paper-linked"
            source = primary["source_url"]
            source_hash = primary["source_sha512"]
            license_value = "MIT license in the paper-linked GitHub repository"
            transform = "Converted the original RGBA paper-linked micrograph to uint8 grayscale without resizing or cropping."
        else:
            kind = "synthetic-paper-inspired"
            source = "Generators/GenerateImageProcessingExampleData.py"
            source_hash = generator_sha512
            license_value = "Generated for this example archive"
            transform = f"Generated deterministically with seed {targets['seed']} to match the recorded paper visual and statistical targets."
        properties = FILE_PROPERTIES[relative_path]
        asset_key = next(key for key, paths in ASSET_GROUPS.items() if relative_path in paths)
        updated_assets.append(
            {
                "path": relative_path,
                "kind": kind,
                "source": source,
                "license": license_value,
                "source_sha512": source_hash,
                "transform": transform,
                "sha512": sha512(output_path),
                "data_type": properties[0],
                "dimensions": properties[1],
                "spacing": properties[2],
                "units": properties[3],
                "pipelines": [PIPELINE_FILES[name] for name, _ in target_pairs],
                "generator": None
                if primary["strategy"].startswith("original")
                else {
                    "path": "Generators/GenerateImageProcessingExampleData.py",
                    "sha512": generator_sha512,
                    "seed": targets["seed"],
                },
                "paper_target": {
                    "doi": primary.get("paper_doi", ""),
                    "url": primary.get("paper_url", ""),
                    "reference_visual": primary["reference_visual"],
                    "target_traits": primary["target_traits"],
                },
                "metrics": metrics.get(asset_key, {}),
                "known_differences": primary["known_differences"],
            }
        )

    for metadata_path, description in [
        (root / "PaperInputMetrics.json", "Generated paper-input metrics"),
        (archived_generator, "Archived deterministic generator source"),
        (archived_targets, "Archived paper-input target contract"),
    ]:
        relative_path = metadata_path.relative_to(root).as_posix()
        updated_assets.append(
            {
                "path": relative_path,
                "kind": "generation-metadata",
                "source": relative_path,
                "license": "BlueQuartz project source",
                "source_sha512": sha512(metadata_path),
                "transform": description,
                "sha512": sha512(metadata_path),
                "data_type": "text",
                "dimensions": [0, 0, 0],
                "spacing": [0.0, 0.0, 0.0],
                "units": "not applicable",
                "pipelines": sorted(PIPELINE_FILES.values()),
            }
        )

    provenance["generated_on"] = "2026-09-01"
    provenance["assets"] = sorted(preserved_assets + updated_assets, key=lambda asset: asset["path"])
    provenance_path.write_text(json.dumps(provenance, indent=2) + "\n", encoding="utf-8")
    update_archive_readme(root)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-root", type=Path, required=True)
    parser.add_argument("--seed", type=int, default=DEFAULT_SEED)
    parser.add_argument("--nist-single-line-zip", type=Path)
    parser.add_argument("--ti-microstructure-image", type=Path)
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    generate_synthetic_assets(args.output_root, args.seed)
    metrics = summarize_generated_assets(args.output_root)
    if args.nist_single_line_zip is not None:
        metrics["am/melt_pool_track.tif"] = derive_nist_melt_pool(args.nist_single_line_zip, args.output_root / "am/melt_pool_track.tif")
    if args.ti_microstructure_image is not None:
        metrics["materials/microstructure_grayscale.png"] = derive_ti_microstructure(args.ti_microstructure_image, args.output_root / "materials/microstructure_grayscale.png")
    (args.output_root / "PaperInputMetrics.json").write_text(
        json.dumps({"schema_version": 1, "seed": args.seed, "assets": metrics}, indent=2) + "\n",
        encoding="utf-8",
    )
    update_provenance(args.output_root)


if __name__ == "__main__":
    main()
