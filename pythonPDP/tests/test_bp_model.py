"""Tests for the BP (backpropagation) model and its parsers."""
from __future__ import annotations

import math
import tempfile
import unittest
from pathlib import Path

from pdp.io.bp_network_parser import (
    BPNetworkSpec,
    ConstraintSpec,
    init_weights_from_spec,
    parse_bp_network_file,
    read_weight_file,
    write_weight_file,
)
from pdp.io.pattern_parser import parse_pattern_pairs_file
from pdp.models.bp import BPModel

# Locate the bp/ data directory relative to this test file
BP_DIR = Path(__file__).resolve().parents[2] / "bp"
XOR_NET = BP_DIR / "XOR.NET"
XOR_PAT = BP_DIR / "XOR.PAT"
XOR_WTS = BP_DIR / "XOR.WTS"
NET_424 = BP_DIR / "424.NET"
PAT_424 = BP_DIR / "424.PAT"


# ---------------------------------------------------------------------------
# Parser tests
# ---------------------------------------------------------------------------

class TestBPNetworkParser(unittest.TestCase):
    def test_xor_net_dimensions(self) -> None:
        spec = parse_bp_network_file(XOR_NET)
        self.assertEqual(spec.nunits, 5)
        self.assertEqual(spec.ninputs, 2)
        self.assertEqual(spec.noutputs, 1)

    def test_xor_net_connectivity(self) -> None:
        spec = parse_bp_network_file(XOR_NET)
        # XOR.NET:
        #   %r 2 2 0 2   → units 2,3 receive from units 0,1 (2 weights each)
        #   %r 4 1 2 2   → unit 4 receives from units 2,3 (2 weights)
        self.assertEqual(spec.first_weight_to[2], 0)
        self.assertEqual(spec.num_weights_to[2], 2)
        self.assertEqual(spec.first_weight_to[3], 0)
        self.assertEqual(spec.num_weights_to[3], 2)
        self.assertEqual(spec.first_weight_to[4], 2)
        self.assertEqual(spec.num_weights_to[4], 2)
        # Input units (0,1) have no incoming weights
        self.assertEqual(spec.num_weights_to[0], 0)
        self.assertEqual(spec.num_weights_to[1], 0)

    def test_xor_net_wchar(self) -> None:
        spec = parse_bp_network_file(XOR_NET)
        # All weight chars should be 'r' (random)
        for i in range(spec.ninputs, spec.nunits):
            for ch in spec.wchar[i]:
                self.assertEqual(ch.lower(), "r",
                                 f"Expected 'r' wchar for unit {i}")

    def test_xor_net_biases_wchar(self) -> None:
        spec = parse_bp_network_file(XOR_NET)
        # biases: %r 2 3 → units 2,3,4 get 'r' bias chars
        for i in range(spec.ninputs, spec.nunits):
            self.assertEqual(spec.bchar[i].lower(), "r",
                             f"Expected 'r' bchar for unit {i}")

    def test_424_net_dimensions(self) -> None:
        spec = parse_bp_network_file(NET_424)
        self.assertEqual(spec.nunits, 10)
        self.assertEqual(spec.ninputs, 4)
        self.assertEqual(spec.noutputs, 4)

    def test_424_net_definitions_overrides(self) -> None:
        spec = parse_bp_network_file(NET_424)
        self.assertEqual(spec.definitions.get("nepochs"), 500)
        # ecrit=0.04 is a float stored directly in definitions
        self.assertAlmostEqual(float(spec.definitions.get("ecrit", 0.0)), 0.04, places=5)

    def test_inline_constraints(self) -> None:
        """Explicit constraints section is correctly parsed."""
        content = (
            "definitions:\nnunits 3\nninputs 1\nnoutputs 2\nend\n"
            "constraints:\na 0.75\nb random negative\nend\n"
            "network:\n% 1 2 0 1\naa\nbb\nend\n"
            "biases:\n% 1 2\nab\nend\n"
        )
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "test.net"
            path.write_text(content)
            spec = parse_bp_network_file(path)

        self.assertIn("a", spec.constraints)
        cs_a = spec.constraints["a"]
        self.assertFalse(cs_a.random)
        self.assertAlmostEqual(cs_a.value, 0.75)

        cs_b = spec.constraints["b"]
        self.assertTrue(cs_b.random)
        self.assertTrue(cs_b.negative)

    def test_random_weights_drawn_with_seed(self) -> None:
        spec = parse_bp_network_file(XOR_NET)
        w1, b1 = init_weights_from_spec(spec, seed=42)
        w2, b2 = init_weights_from_spec(spec, seed=42)
        self.assertEqual(w1, w2)
        self.assertEqual(b1, b2)

    def test_random_weights_differ_for_different_seeds(self) -> None:
        spec = parse_bp_network_file(XOR_NET)
        w1, _ = init_weights_from_spec(spec, seed=1)
        w2, _ = init_weights_from_spec(spec, seed=2)
        all_w1 = [v for row in w1 for v in row]
        all_w2 = [v for row in w2 for v in row]
        self.assertNotEqual(all_w1, all_w2)


class TestWeightFile(unittest.TestCase):
    def test_read_xor_weights(self) -> None:
        spec = parse_bp_network_file(XOR_NET)
        weights, biases = read_weight_file(XOR_WTS, spec)
        # Should have weights for hidden (2) + output (1) units
        total_w = sum(spec.num_weights_to[i] for i in range(spec.nunits))
        flat = [v for row in weights for v in row]
        self.assertEqual(len(flat), total_w)
        self.assertEqual(len(biases), spec.nunits)

    def test_write_read_roundtrip(self) -> None:
        spec = parse_bp_network_file(XOR_NET)
        weights, biases = read_weight_file(XOR_WTS, spec)
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp) / "out.wts"
            write_weight_file(out, weights, biases, spec.nunits)
            weights2, biases2 = read_weight_file(out, spec)
        for i in range(spec.nunits):
            for j in range(len(weights[i])):
                self.assertAlmostEqual(weights[i][j], weights2[i][j], places=5)
        for i in range(spec.nunits):
            self.assertAlmostEqual(biases[i], biases2[i], places=5)


# ---------------------------------------------------------------------------
# BPModel — construction
# ---------------------------------------------------------------------------

class TestBPModelLoad(unittest.TestCase):
    def _load_xor(self) -> BPModel:
        spec = parse_bp_network_file(XOR_NET)
        model = BPModel()
        model.load_network(spec, seed=42)
        return model

    def test_architecture_xor(self) -> None:
        m = self._load_xor()
        self.assertEqual(m.nunits, 5)
        self.assertEqual(m.ninputs, 2)
        self.assertEqual(m.noutputs, 1)

    def test_arrays_allocated(self) -> None:
        m = self._load_xor()
        self.assertEqual(len(m.activation), 5)
        self.assertEqual(len(m.netinput), 5)
        self.assertEqual(len(m.error), 5)
        self.assertEqual(len(m.delta), 5)
        self.assertEqual(len(m.target), 1)

    def test_weights_allocated(self) -> None:
        m = self._load_xor()
        # Units 0,1 = inputs (no incoming weights)
        self.assertEqual(len(m.weight[0]), 0)
        self.assertEqual(len(m.weight[1]), 0)
        # Units 2,3 = hidden (2 weights each from inputs 0,1)
        self.assertEqual(len(m.weight[2]), 2)
        self.assertEqual(len(m.weight[3]), 2)
        # Unit 4 = output (2 weights from hidden units 2,3)
        self.assertEqual(len(m.weight[4]), 2)

    def test_weights_nonzero_after_init(self) -> None:
        m = self._load_xor()
        flat = [w for row in m.weight for w in row]
        self.assertTrue(any(abs(w) > 0 for w in flat),
                        "All weights are zero after init — expected random non-zero")


# ---------------------------------------------------------------------------
# BPModel — forward pass
# ---------------------------------------------------------------------------

class TestBPModelForward(unittest.TestCase):
    def _load_xor_trained(self) -> BPModel:
        """Return a fully trained XOR model (trained to convergence, not just .WTS)."""
        spec = parse_bp_network_file(XOR_NET)
        model = BPModel(lrate=0.5, momentum=0.9, nepochs=3000, ecrit=0.04, lgrain="epoch")
        model.load_network(spec, seed=42)
        pairs = parse_pattern_pairs_file(XOR_PAT, expected_inputs=2, expected_outputs=1)
        model.load_patterns(pairs)
        model.strain()
        return model

    def _load_xor_with_saved_weights(self) -> BPModel:
        spec = parse_bp_network_file(XOR_NET)
        model = BPModel()
        model.load_network(spec, seed=0)
        model.load_weights(XOR_WTS)
        return model

    def _load_xor_patterns(self, model: BPModel) -> None:
        pairs = parse_pattern_pairs_file(
            XOR_PAT,
            expected_inputs=model.ninputs,
            expected_outputs=model.noutputs,
        )
        model.load_patterns(pairs)

    def test_logistic_boundary(self) -> None:
        self.assertAlmostEqual(BPModel._logistic(0.0), 0.5, places=6)
        self.assertAlmostEqual(BPModel._logistic(100.0), 0.99999988, places=6)
        self.assertAlmostEqual(BPModel._logistic(-100.0), 0.00000012, places=6)

    def test_compute_output_shape(self) -> None:
        m = self._load_xor_with_saved_weights()
        self._load_xor_patterns(m)
        m.patno = 0
        m.setinput()
        m.compute_output()
        self.assertEqual(len(m.output_activations), 1)
        out = m.output_activations[0]
        self.assertGreater(out, 0.0)
        self.assertLess(out, 1.0)

    def test_xor_forward_p00(self) -> None:
        """XOR(0,0) should predict near 0 after training."""
        m = self._load_xor_trained()
        m.test_pattern("p00")
        out = m.output_activations[0]
        self.assertLess(out, 0.2, f"XOR(0,0) output {out:.4f} should be near 0")

    def test_xor_forward_p01(self) -> None:
        """XOR(0,1) should predict near 1 after training."""
        m = self._load_xor_trained()
        m.test_pattern("p01")
        out = m.output_activations[0]
        self.assertGreater(out, 0.8, f"XOR(0,1) output {out:.4f} should be near 1")

    def test_xor_forward_p10(self) -> None:
        """XOR(1,0) should predict near 1 after training."""
        m = self._load_xor_trained()
        m.test_pattern("p10")
        out = m.output_activations[0]
        self.assertGreater(out, 0.8, f"XOR(1,0) output {out:.4f} should be near 1")

    def test_xor_forward_p11(self) -> None:
        """XOR(1,1) should predict near 0 after training."""
        m = self._load_xor_trained()
        m.test_pattern("p11")
        out = m.output_activations[0]
        self.assertLess(out, 0.2, f"XOR(1,1) output {out:.4f} should be near 0")

    def test_tall_does_not_update_weights(self) -> None:
        spec = parse_bp_network_file(XOR_NET)
        m = BPModel()
        m.load_network(spec, seed=5)
        pairs = parse_pattern_pairs_file(XOR_PAT, expected_inputs=2, expected_outputs=1)
        m.load_patterns(pairs)
        weights_before = [list(row) for row in m.weight]
        biases_before = list(m.bias)
        m.tall()
        self.assertEqual(m.weight, weights_before)
        self.assertEqual(m.bias, biases_before)


# ---------------------------------------------------------------------------
# BPModel — training
# ---------------------------------------------------------------------------

class TestBPModelTraining(unittest.TestCase):
    def _train_xor(self, permuted: bool = False, seed: int = 0) -> BPModel:
        spec = parse_bp_network_file(XOR_NET)
        m = BPModel(lrate=0.5, momentum=0.9, nepochs=3000, ecrit=0.04,
                    lgrain="epoch")
        m.load_network(spec, seed=seed)
        pairs = parse_pattern_pairs_file(
            XOR_PAT,
            expected_inputs=m.ninputs,
            expected_outputs=m.noutputs,
        )
        m.load_patterns(pairs)
        if permuted:
            m.ptrain()
        else:
            m.strain()
        return m

    def test_xor_learns_sequential(self) -> None:
        m = self._train_xor(permuted=False)
        self.assertLess(m.tss, 0.1,
                        f"XOR tss={m.tss:.4f} should be < 0.1 after training")

    def test_xor_learns_permuted(self) -> None:
        m = self._train_xor(permuted=True, seed=99)
        self.assertLess(m.tss, 0.1,
                        f"XOR ptrain tss={m.tss:.4f} should be < 0.1 after training")

    def test_tss_decreases_over_training(self) -> None:
        """Verify tss goes down over epochs."""
        spec = parse_bp_network_file(XOR_NET)
        m = BPModel(lrate=0.5, momentum=0.9, nepochs=100, lgrain="epoch")
        m.load_network(spec, seed=7)
        pairs = parse_pattern_pairs_file(
            XOR_PAT, expected_inputs=m.ninputs, expected_outputs=m.noutputs
        )
        m.load_patterns(pairs)
        m.strain()
        tss_100 = m.tss

        # Continue training
        m.nepochs = 500
        m.strain()
        tss_600 = m.tss

        self.assertLess(tss_600, tss_100,
                        f"tss should decrease: {tss_600:.4f} vs {tss_100:.4f}")

    def test_reset_zeroes_epochno(self) -> None:
        spec = parse_bp_network_file(XOR_NET)
        m = BPModel(nepochs=10)
        m.load_network(spec, seed=1)
        pairs = parse_pattern_pairs_file(
            XOR_PAT, expected_inputs=m.ninputs, expected_outputs=m.noutputs
        )
        m.load_patterns(pairs)
        m.strain()
        self.assertGreater(m.epochno, 0)
        m.reset_weights()
        self.assertEqual(m.epochno, 0)

    def test_epoch_grain_vs_pattern_grain(self) -> None:
        """Epoch-grain and pattern-grain should both converge on XOR."""
        spec = parse_bp_network_file(XOR_NET)

        m_epoch = BPModel(lrate=0.5, momentum=0.9, nepochs=3000,
                          ecrit=0.04, lgrain="epoch")
        m_epoch.load_network(spec, seed=42)
        pairs = parse_pattern_pairs_file(
            XOR_PAT, expected_inputs=m_epoch.ninputs, expected_outputs=m_epoch.noutputs
        )
        m_epoch.load_patterns(pairs)
        m_epoch.strain()

        m_pat = BPModel(lrate=0.5, momentum=0.9, nepochs=3000,
                        ecrit=0.04, lgrain="pattern")
        m_pat.load_network(spec, seed=42)
        m_pat.load_patterns(pairs)
        m_pat.strain()

        # Both should converge
        self.assertLess(m_epoch.tss, 0.1,
                        f"epoch-grain tss={m_epoch.tss:.4f} too high")
        self.assertLess(m_pat.tss, 0.1,
                        f"pattern-grain tss={m_pat.tss:.4f} too high")

    def test_definitions_override_nepochs_ecrit(self) -> None:
        """nepochs and ecrit in the .NET definitions: block override defaults."""
        spec = parse_bp_network_file(NET_424)
        m = BPModel()
        m.load_network(spec, seed=0)
        self.assertEqual(m.nepochs, 500)
        self.assertAlmostEqual(m.ecrit, 0.04, places=5)


# ---------------------------------------------------------------------------
# BPModel — CLI session (smoke tests)
# ---------------------------------------------------------------------------

class TestBPSession(unittest.TestCase):
    def _make_session(self) -> "BPSession":  # noqa: F821
        from pdp.cli.bp_cli import BPSession
        return BPSession(data_dir=BP_DIR)

    def test_load_network_command(self) -> None:
        s = self._make_session()
        s.run_line("get network XOR.NET")
        self.assertEqual(s.model.nunits, 5)

    def test_load_patterns_command(self) -> None:
        s = self._make_session()
        s.run_line("get network XOR.NET")
        s.run_line("get patterns XOR.PAT")
        self.assertEqual(len(s.model.input_patterns), 4)

    def test_tall_command(self) -> None:
        """tall with the .WTS seed weights: tss will be ~1 (not yet trained)."""
        s = self._make_session()
        s.run_line("get network XOR.NET")
        s.run_line("get patterns XOR.PAT")
        s.run_line("get weights XOR.WTS")
        s.run_line("tall")
        # The .WTS file is a starting-point, not a solution; tss just needs to be finite
        self.assertGreater(s.model.tss, 0.0)
        self.assertLess(s.model.tss, 4.0)

    def test_set_params(self) -> None:
        s = self._make_session()
        s.run_line("get network XOR.NET")
        s.run_line("set param lrate 0.25")
        self.assertAlmostEqual(s.model.lrate, 0.25)
        s.run_line("set nepochs 200")
        self.assertEqual(s.model.nepochs, 200)
        s.run_line("set ecrit 0.01")
        self.assertAlmostEqual(s.model.ecrit, 0.01)

    def test_set_mode_lgrain_epoch(self) -> None:
        s = self._make_session()
        s.run_line("set mode lgrain epoch")
        self.assertEqual(s.model.lgrain, "epoch")

    def test_set_mode_lgrain_pattern(self) -> None:
        s = self._make_session()
        s.run_line("set mode lgrain pattern")
        self.assertEqual(s.model.lgrain, "pattern")

    def test_quit_returns_false(self) -> None:
        s = self._make_session()
        keep = s.run_line("quit y")
        self.assertFalse(keep)

    def test_script_file_424(self) -> None:
        """Run the 424.STR script through the CLI session."""
        from pdp.cli.bp_cli import BPSession
        from pdp.runtime.command_stream import CommandTokenStream
        s = BPSession(data_dir=BP_DIR)
        stream = CommandTokenStream()
        stream.feed_file(BP_DIR / "424.STR")
        s.run_stream(stream)
        self.assertGreater(s.model.nunits, 0)

    def test_script_file_xor(self) -> None:
        """Run the XOR.STR script: loads starting weights, trains 30 epochs, runs tall."""
        from pdp.cli.bp_cli import BPSession
        from pdp.runtime.command_stream import CommandTokenStream
        s = BPSession(data_dir=BP_DIR)
        stream = CommandTokenStream()
        stream.feed_file(BP_DIR / "XOR.STR")
        s.run_stream(stream)
        # XOR.STR: 30 epochs from seed weights — may or may not converge;
        # just verify tss is a finite positive number
        self.assertGreater(s.model.tss, 0.0)
        self.assertLess(s.model.tss, 4.0)


if __name__ == "__main__":
    unittest.main()
