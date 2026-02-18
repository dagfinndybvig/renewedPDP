"""Tests for the PA (pattern associator) model and its supporting parsers."""
from __future__ import annotations

import tempfile
import unittest
from pathlib import Path

from pdp.io.network_parser import NetworkSpec, parse_network_file
from pdp.io.pattern_parser import PatternPairSet, parse_pattern_pairs_file
from pdp.models.pa import PAModel, _dotprod, _sumsquares, _veccor, _veclen


# ---------------------------------------------------------------------------
# PatternPairSet parser
# ---------------------------------------------------------------------------

class TestPatternPairsParser(unittest.TestCase):
    def test_parses_numeric_columns(self) -> None:
        content = "p0 1 0 1 0  1 0\np1 0 1 0 1  0 1\n"
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "test.pat"
            path.write_text(content, encoding="utf-8")
            pairs = parse_pattern_pairs_file(path, expected_inputs=4, expected_outputs=2)

        self.assertEqual(pairs.names, ["p0", "p1"])
        self.assertEqual(pairs.inputs[0], [1.0, 0.0, 1.0, 0.0])
        self.assertEqual(pairs.targets[0], [1.0, 0.0])
        self.assertEqual(pairs.inputs[1], [0.0, 1.0, 0.0, 1.0])
        self.assertEqual(pairs.targets[1], [0.0, 1.0])

    def test_parses_symbolic_tokens(self) -> None:
        content = "x + - .  + -\n"
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "sym.pat"
            path.write_text(content, encoding="utf-8")
            pairs = parse_pattern_pairs_file(path, expected_inputs=3, expected_outputs=2)

        self.assertEqual(pairs.inputs[0], [1.0, -1.0, 0.0])
        self.assertEqual(pairs.targets[0], [1.0, -1.0])

    def test_jets_pa_file(self) -> None:
        repo_root = Path(__file__).resolve().parents[2]
        pairs = parse_pattern_pairs_file(
            "JETS.PAT", expected_inputs=12, expected_outputs=2, base_dir=repo_root / "pa"
        )
        self.assertEqual(len(pairs.names), 27)
        self.assertEqual(len(pairs.inputs[0]), 12)
        self.assertEqual(len(pairs.targets[0]), 2)
        # First person is Art — Jets member → target [1, 0]
        self.assertEqual(pairs.names[0], "Art")
        self.assertEqual(pairs.targets[0], [1.0, 0.0])


# ---------------------------------------------------------------------------
# NetworkSpec biases extension
# ---------------------------------------------------------------------------

class TestNetworkBiasesParser(unittest.TestCase):
    def test_biases_parsed(self) -> None:
        repo_root = Path(__file__).resolve().parents[2]
        spec = parse_network_file("JETS.NET", base_dir=repo_root / "pa")
        self.assertIsNotNone(spec.biases)
        assert spec.biases is not None
        self.assertEqual(len(spec.biases), spec.nunits)
        # First 12 are inputs → bias = 0
        self.assertEqual(spec.biases[:12], [0.0] * 12)

    def test_network_without_biases(self) -> None:
        """Network files that have no biases: section should give biases=None."""
        content = (
            "definitions:\nnunits 2\nend\n"
            "constraints:\na 0.5\nend\n"
            "network:\n.a\na.\nend\n"
        )
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "mini.net"
            path.write_text(content, encoding="utf-8")
            spec = parse_network_file(path)
        self.assertIsNone(spec.biases)


# ---------------------------------------------------------------------------
# PAModel — construction and reset
# ---------------------------------------------------------------------------

class TestPAModelLoad(unittest.TestCase):
    def _make_model(self) -> PAModel:
        repo_root = Path(__file__).resolve().parents[2]
        spec = parse_network_file("JETS.NET", base_dir=repo_root / "pa")
        model = PAModel()
        model.load_network(spec)
        return model

    def test_architecture_sizes(self) -> None:
        model = self._make_model()
        self.assertEqual(model.nunits, 14)
        self.assertEqual(model.ninputs, 12)
        self.assertEqual(model.noutputs, 2)

    def test_weights_shape_after_load(self) -> None:
        model = self._make_model()
        # After load_network, reset_weights zeroes everything
        self.assertEqual(len(model.weights), 2)
        self.assertEqual(len(model.weights[0]), 12)
        self.assertTrue(all(w == 0.0 for row in model.weights for w in row))

    def test_arrays_allocated(self) -> None:
        model = self._make_model()
        self.assertEqual(len(model.input), 12)
        self.assertEqual(len(model.output), 14)
        self.assertEqual(len(model.target), 2)
        self.assertEqual(len(model.netinput), 14)
        self.assertEqual(len(model.error), 14)


# ---------------------------------------------------------------------------
# PAModel — forward pass and learning
# ---------------------------------------------------------------------------

class TestPAModelForward(unittest.TestCase):
    def _loaded_model(self) -> PAModel:
        repo_root = Path(__file__).resolve().parents[2]
        spec = parse_network_file("JETS.NET", base_dir=repo_root / "pa")
        model = PAModel(cs_mode=True, noise=0.0)  # deterministic output
        model.load_network(spec)
        pairs = parse_pattern_pairs_file(
            "JETS.PAT", expected_inputs=12, expected_outputs=2, base_dir=repo_root / "pa"
        )
        model.load_patterns(pairs)
        return model

    def test_tall_runs_without_error(self) -> None:
        model = self._loaded_model()
        model.tall()  # one pass, no learning
        # tall runs one epoch (epochno incremented) but does not update weights
        self.assertEqual(model.epochno, 1)
        self.assertTrue(all(w == 0.0 for row in model.weights for w in row))

    def test_test_pattern_by_name(self) -> None:
        model = self._loaded_model()
        index = model.test_pattern("Art")
        self.assertEqual(index, 0)
        self.assertEqual(model.patno, 0)
        # Output vector has 2 elements (Jets/Sharks)
        out = model.output[model.ninputs :]
        self.assertEqual(len(out), 2)

    def test_test_pattern_by_index(self) -> None:
        model = self._loaded_model()
        index = model.test_pattern("1")
        self.assertEqual(index, 1)

    def test_pss_is_non_negative(self) -> None:
        model = self._loaded_model()
        model.test_pattern("Art")
        self.assertGreaterEqual(model.pss, 0.0)

    def test_output_in_sigmoid_range_cs_mode(self) -> None:
        model = self._loaded_model()
        model.test_pattern("Art")
        out = model.output[model.ninputs :]
        for v in out:
            self.assertGreater(v, 0.0)
            self.assertLess(v, 1.0)

    def test_strain_reduces_tss(self) -> None:
        model = self._loaded_model()
        model.nepochs = 50
        model.lflag = True
        model.strain()
        self.assertEqual(model.epochno, 50)
        # After 50 epochs of delta-rule learning, tss should be substantially lower
        # than nunits * npatterns (upper bound if all wrong)
        self.assertLess(model.tss, len(model.input_patterns) * model.noutputs)

    def test_reset_zeroes_weights(self) -> None:
        model = self._loaded_model()
        model.nepochs = 5
        model.strain()
        model.reset_weights()
        self.assertTrue(all(w == 0.0 for row in model.weights for w in row))
        self.assertEqual(model.epochno, 0)
        self.assertEqual(model.tss, 0.0)


# ---------------------------------------------------------------------------
# Maths helpers
# ---------------------------------------------------------------------------

class TestMathHelpers(unittest.TestCase):
    def test_dotprod(self) -> None:
        self.assertAlmostEqual(_dotprod([1.0, 0.0], [1.0, 0.0], 2), 0.5)
        self.assertAlmostEqual(_dotprod([1.0, 1.0], [1.0, 1.0], 2), 1.0)

    def test_sumsquares_identical(self) -> None:
        self.assertAlmostEqual(_sumsquares([1.0, 0.5], [1.0, 0.5], 2), 0.0)

    def test_sumsquares_opposite(self) -> None:
        self.assertAlmostEqual(_sumsquares([1.0, 0.0], [0.0, 1.0], 2), 2.0)

    def test_veccor_identical(self) -> None:
        self.assertAlmostEqual(_veccor([1.0, 0.0], [1.0, 0.0], 2), 1.0)

    def test_veccor_orthogonal(self) -> None:
        self.assertAlmostEqual(_veccor([1.0, 0.0], [0.0, 1.0], 2), 0.0)

    def test_veclen(self) -> None:
        # RMS length of [1,0] with n=2 → sqrt(0.5)
        self.assertAlmostEqual(_veclen([1.0, 0.0], 2), (0.5) ** 0.5)


if __name__ == "__main__":
    unittest.main()
