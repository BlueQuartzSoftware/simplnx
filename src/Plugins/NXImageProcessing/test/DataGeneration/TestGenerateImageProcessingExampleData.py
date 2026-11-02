#!/usr/bin/env python3

import hashlib
from io import BytesIO
import json
from pathlib import Path
import tempfile
import unittest
import zipfile

import numpy as np
from PIL import Image
from scipy.io import savemat

from GenerateImageProcessingExampleData import derive_nist_melt_pool, derive_ti_microstructure, generate_synthetic_assets, summarize_generated_assets, update_provenance


class GenerateImageProcessingExampleDataTests(unittest.TestCase):
    def generate(self, root: Path):
        generate_synthetic_assets(root, seed=20260901)
        return summarize_generated_assets(root)

    def test_generation_is_deterministic(self):
        with tempfile.TemporaryDirectory() as first_dir, tempfile.TemporaryDirectory() as second_dir:
            first = Path(first_dir)
            second = Path(second_dir)
            self.generate(first)
            self.generate(second)
            first_hashes = {
                path.relative_to(first): hashlib.sha512(path.read_bytes()).hexdigest()
                for path in first.rglob("*")
                if path.is_file()
            }
            second_hashes = {
                path.relative_to(second): hashlib.sha512(path.read_bytes()).hexdigest()
                for path in second.rglob("*")
                if path.is_file()
            }
            self.assertEqual(first_hashes, second_hashes)

    def test_generated_metrics_match_paper_inspired_targets(self):
        with tempfile.TemporaryDirectory() as output_dir:
            summaries = self.generate(Path(output_dir))

            xct = summaries["am/xct_porosity.mha"]
            self.assertEqual(xct["dimensions"], [128, 128, 48])
            self.assertGreater(xct["void_fraction"], 0.002)
            self.assertLess(xct["void_fraction"], 0.04)
            self.assertGreaterEqual(xct["dark_component_count"], 8)

            powder = summaries["materials/powder_particles.png"]
            self.assertEqual(powder["dimensions"], [512, 512, 1])
            self.assertGreaterEqual(powder["particle_count"], 45)
            self.assertGreater(powder["touching_pair_count"], 5)

            cracks = summaries["materials/binary_mask.png"]
            self.assertEqual(cracks["dimensions"], [512, 384, 1])
            self.assertGreater(cracks["foreground_fraction"], 0.005)
            self.assertLess(cracks["foreground_fraction"], 0.08)
            self.assertGreaterEqual(cracks["branch_count"], 6)

            ultrasound = summaries["materials/edge_input.png"]
            self.assertEqual(ultrasound["dimensions"], [512, 384, 1])
            self.assertGreater(ultrasound["tissue_speckle_cv"], 0.2)
            self.assertLess(ultrasound["lumen_mean"], ultrasound["wall_mean"] * 0.5)

            calibration = summaries["am/calibration/transmission.mha"]
            self.assertEqual(calibration["dimensions"], [512, 256, 1])
            self.assertGreater(calibration["dynamic_range"], 0.75)
            self.assertGreater(calibration["stripe_amplitude"], 0.01)

            serial = summaries["io/serial_stack"]
            self.assertEqual(serial["slice_count"], 8)
            self.assertGreater(serial["adjacent_slice_correlation"], 0.85)
            self.assertLess(serial["adjacent_slice_correlation"], 0.999)

    def test_binary_mask_uses_filter_foreground_value(self):
        with tempfile.TemporaryDirectory() as output_dir:
            root = Path(output_dir)
            self.generate(root)
            values = np.unique(np.asarray(Image.open(root / "materials/binary_mask.png")))
            self.assertEqual(values.tolist(), [0, 1])

    def test_grayscale_tiff_declares_one_sample_per_pixel(self):
        with tempfile.TemporaryDirectory() as output_dir:
            root = Path(output_dir)
            self.generate(root)
            with Image.open(root / "am/powder_bed_layers/layer_000.tif") as image:
                self.assertEqual(image.tag_v2.get(277), 1)

    def test_nist_derivation_selects_peak_camera_frame(self):
        with tempfile.TemporaryDirectory() as output_dir:
            root = Path(output_dir)
            camera_signal = np.zeros((7, 11, 3), dtype=np.uint16)
            camera_signal[3, 2:8, 1] = np.arange(6, dtype=np.uint16) * 1000 + 4000
            mat_buffer = BytesIO()
            savemat(mat_buffer, {"CameraSignal": camera_signal})
            source_zip = root / "source.zip"
            with zipfile.ZipFile(source_zip, "w") as archive:
                archive.writestr("example_CameraSignal.mat", mat_buffer.getvalue())
            output_path = root / "melt_pool.tif"
            metrics = derive_nist_melt_pool(source_zip, output_path)
            self.assertEqual(metrics["source_frame"], 1)
            self.assertEqual(metrics["dimensions"], [11, 7, 1])
            self.assertGreater(np.asarray(Image.open(output_path)).max(), 240)

    def test_ti_derivation_converts_original_pixels_to_grayscale(self):
        with tempfile.TemporaryDirectory() as output_dir:
            root = Path(output_dir)
            source = np.zeros((13, 17, 4), dtype=np.uint8)
            source[:, :, 0] = 180
            source[:, :, 1] = np.arange(17, dtype=np.uint8)
            source[:, :, 3] = 255
            source_path = root / "source.png"
            Image.fromarray(source).save(source_path)
            output_path = root / "microstructure.png"
            metrics = derive_ti_microstructure(source_path, output_path)
            self.assertEqual(metrics["dimensions"], [17, 13, 1])
            self.assertEqual(np.asarray(Image.open(output_path)).shape, (13, 17))

    def test_provenance_records_generator_and_paper_targets(self):
        with tempfile.TemporaryDirectory() as output_dir:
            root = Path(output_dir)
            self.generate(root)
            (root / "Provenance.json").write_text(
                json.dumps({"version": 1, "generated_on": "old", "archive": "example.tar.gz", "assets": []}),
                encoding="utf-8",
            )
            update_provenance(root)
            update_provenance(root)
            provenance = json.loads((root / "Provenance.json").read_text(encoding="utf-8"))
            asset_paths = [asset["path"] for asset in provenance["assets"]]
            self.assertEqual(len(asset_paths), len(set(asset_paths)))
            xct = next(asset for asset in provenance["assets"] if asset["path"] == "am/xct_porosity.mha")
            self.assertEqual(xct["kind"], "synthetic-paper-inspired")
            self.assertEqual(xct["generator"]["seed"], 20260901)
            self.assertEqual(xct["paper_target"]["doi"], "10.1016/j.mex.2018.09.005")
            self.assertIn("known_differences", xct)


if __name__ == "__main__":
    unittest.main()
