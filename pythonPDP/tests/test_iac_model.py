from __future__ import annotations

import unittest
from pathlib import Path

from pdp.io.network_parser import parse_network_file
from pdp.io.pattern_parser import PatternSet
from pdp.models.iac import IACModel


class TestIACModel(unittest.TestCase):
    def test_cycle_keeps_activation_bounds(self) -> None:
        repo_root = Path(__file__).resolve().parents[2]
        spec = parse_network_file("JETS.NET", base_dir=repo_root / "iac")

        model = IACModel()
        model.load_network(spec)
        model.set_unit_names([f"u{i}" for i in range(model.nunits)])

        model.extinput[0] = 1.0
        model.extinput[1] = -1.0
        model.cycle(5)

        self.assertEqual(model.cycleno, 5)
        self.assertEqual(len(model.activation), model.nunits)
        for value in model.activation:
            self.assertLessEqual(value, model.maxactiv)
            self.assertGreaterEqual(value, model.minactiv)

    def test_test_pattern_sets_extinput_and_cycles(self) -> None:
        repo_root = Path(__file__).resolve().parents[2]
        spec = parse_network_file("JETS.NET", base_dir=repo_root / "iac")

        model = IACModel()
        model.load_network(spec)
        model.load_patterns(
            PatternSet(
                names=["jets", "sharks"],
                inputs=[
                    [1.0] * model.nunits,
                    [-1.0] * model.nunits,
                ],
            )
        )

        index = model.test_pattern("jets", iterations=3)

        self.assertEqual(index, 0)
        self.assertEqual(model.cycleno, 3)
        self.assertTrue(all(value == 1.0 for value in model.extinput))


if __name__ == "__main__":
    unittest.main()
