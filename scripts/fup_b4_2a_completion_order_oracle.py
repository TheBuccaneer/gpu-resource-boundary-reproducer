#!/usr/bin/env python3
"""Offline completion/destructor ordering oracle for FUP-B4-2A T1 logs."""

from __future__ import annotations

import argparse
import hashlib
import re
import sys
from dataclasses import dataclass
from pathlib import Path


EXIT_PASS = 0
EXIT_FAIL = 10
EXIT_INCONCLUSIVE = 20
EXIT_INVALID = 64
APPROVED_SOURCE_SHA256 = (
    "bdf2948b6329b302130f2912983e8054b7e1ae34af0b53756b6fd4968bfb985a"
)

KV_RE = re.compile(r"([A-Za-z0-9_]+)=([^\s]+)")


class InvalidEvidence(ValueError):
    pass


@dataclass(frozen=True)
class TrialResult:
    trial: int
    result: str
    evidence: str
    reason: str


def fields(line: str) -> dict[str, str]:
    return dict(KV_RE.findall(line))


def integer(values: dict[str, str], key: str) -> int:
    try:
        return int(values[key], 10)
    except (KeyError, ValueError) as exc:
        raise InvalidEvidence(f"missing_or_invalid_{key}") from exc


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def classify_trial(
    trial: int, control: dict[str, str], order: dict[str, str]
) -> TrialResult:
    control_result = control.get("RESULT")
    observed = integer(control, "observed_event_status")
    query_error = (
        0 if control_result == "PASS" else integer(control, "query_error")
    )

    destructor_started = integer(order, "destructor_started")
    destructor_sequence = integer(order, "destructor_sequence")
    complete_started = integer(order, "complete_callback_started")
    complete_sequence = integer(order, "complete_sequence")
    complete_status = integer(order, "complete_callback_status")

    # This is the direct T1 violation oracle. CL_QUEUED, CL_SUBMITTED, and
    # CL_RUNNING are all numerically greater than CL_COMPLETE (zero).
    if (
        control_result == "FAIL"
        and control.get("oracle") == "destructor_before_event_completion"
        and query_error == 0
        and observed > 0
        and destructor_started == 1
        and destructor_sequence > 0
    ):
        return TrialResult(
            trial,
            "FAIL",
            "DIRECT_NONTERMINAL_EVENT_AFTER_DESTRUCTOR",
            f"destructor observed while event status was {observed}",
        )

    # The approved T1 source emits this shorter record only after all bounded
    # cleanup conditions succeeded. Its source hash is checked by evaluate().
    if (
        control_result == "PASS"
        and control.get("oracle") == "event_completion_observed_before_destructor"
        and observed == 0
        and integer(control, "final_event_status") == 0
        and destructor_started == 1
        and complete_started == 1
        and complete_status == 0
        and complete_sequence > 0
        and complete_sequence < destructor_sequence
    ):
        return TrialResult(
            trial,
            "PASS",
            "DIRECT_COMPLETION_WINDOW",
            "host observed CL_COMPLETE before destructor callback start",
        )

    normal_cleanup = (
        query_error == 0
        and observed == 0
        and integer(control, "cleanup_terminal") == 1
        and integer(control, "final_event_status") == 0
        and integer(control, "final_query_error") == 0
        and integer(control, "destructor_seen") == 1
        and integer(control, "complete_callback_seen") == 1
        and destructor_started == 1
        and complete_started == 1
        and complete_status == 0
        and destructor_sequence > 0
        and complete_sequence > 0
        and destructor_sequence != complete_sequence
    )

    # In T1 each callback sets its *_started flag before acquiring a sequence
    # number. Therefore sequence order alone does not prove callback-entry
    # order: a destructor callback may set its flag, be preempted, and acquire
    # sequence 2 after the completion callback acquired sequence 1.
    if normal_cleanup and complete_sequence < destructor_sequence:
        return TrialResult(
            trial,
            "INCONCLUSIVE",
            "SUPPORTED_COMPLETION_SEQUENCE_POINT_FIRST",
            "normal completion sequence point preceded destructor sequence point, "
            "but T1 records callback-start flags before sequence assignment",
        )

    if observed < 0 or complete_status < 0:
        return TrialResult(
            trial,
            "INCONCLUSIVE",
            "TERMINAL_ERROR",
            "negative event or callback status does not establish normal completion",
        )

    if normal_cleanup and destructor_sequence < complete_sequence:
        return TrialResult(
            trial,
            "INCONCLUSIVE",
            "DESTRUCTOR_SEQUENCE_POINT_FIRST_WITHOUT_DIRECT_ORACLE",
            "sequence-point order is suspicious but no nonterminal event status was captured",
        )

    return TrialResult(
        trial,
        "INCONCLUSIVE",
        "INCOMPLETE_OR_AMBIGUOUS_EVIDENCE",
        "required status, cleanup, callback, or ordering evidence is incomplete",
    )


def evaluate(path: Path, expected_trials: int, expected_sha256: str | None) -> int:
    raw = path.read_bytes()
    actual_hash = hashlib.sha256(raw).hexdigest()
    if expected_sha256 and actual_hash != expected_sha256.lower():
        raise InvalidEvidence(
            f"sha256_mismatch expected={expected_sha256.lower()} actual={actual_hash}"
        )

    controls: dict[int, dict[str, str]] = {}
    orders: dict[int, dict[str, str]] = {}
    c0: dict[str, str] | None = None
    c2: dict[str, str] | None = None
    config: dict[str, str] | None = None
    provenance_sync: str | None = None
    source_hash: str | None = None
    runtime_label: str | None = None

    for line in raw.decode("utf-8").splitlines():
        values = fields(line)
        if line.startswith("PROVENANCE rusticl_debug_sync="):
            provenance_sync = values.get("rusticl_debug_sync")
        elif line.startswith("PROVENANCE source_sha256="):
            source_hash = values.get("source_sha256")
        elif line.startswith("PROVENANCE runtime_label="):
            runtime_label = values.get("runtime_label")
        elif line.startswith("CONFIG "):
            config = values
        elif line.startswith("CONTROL=C0 "):
            c0 = values
        elif line.startswith("CONTROL=C2 "):
            c2 = values
        elif line.startswith("CONTROL=C1 "):
            trial = integer(values, "TRIAL")
            if trial in controls:
                raise InvalidEvidence(f"duplicate_C1_control_trial_{trial}")
            controls[trial] = values
        elif line.startswith("ORDER control=C1 "):
            trial = integer(values, "trial")
            if trial in orders:
                raise InvalidEvidence(f"duplicate_C1_order_trial_{trial}")
            orders[trial] = values

    if provenance_sync != "0":
        raise InvalidEvidence("rusticl_debug_sync_not_zero")
    if source_hash != APPROVED_SOURCE_SHA256:
        raise InvalidEvidence("unexpected_or_missing_T1_source_sha256")
    if config is None or integer(config, "trials") != expected_trials:
        raise InvalidEvidence("unexpected_or_missing_trial_configuration")
    if c0 is None or c0.get("RESULT") != "PASS" or integer(c0, "event_status") != 0:
        raise InvalidEvidence("C0_not_passed_with_CL_COMPLETE")
    if c2 is None or c2.get("RESULT") != "PASS":
        raise InvalidEvidence("C2_not_passed")

    expected_numbers = set(range(1, expected_trials + 1))
    if set(controls) != expected_numbers or set(orders) != expected_numbers:
        raise InvalidEvidence("missing_or_unexpected_C1_trials")

    results = [
        classify_trial(number, controls[number], orders[number])
        for number in sorted(expected_numbers)
    ]

    for result in results:
        print(
            f"TRIAL={result.trial} RESULT={result.result} "
            f"EVIDENCE={result.evidence} REASON={result.reason}"
        )

    if any(result.result == "FAIL" for result in results):
        aggregate = "FAIL"
        aggregate_evidence = "CONFIRMED_LIFETIME_VIOLATION"
        exit_code = EXIT_FAIL
    elif all(result.result == "PASS" for result in results):
        aggregate = "PASS"
        aggregate_evidence = "CONFIRMED_COMPLETION_BEFORE_DESTRUCTOR"
        exit_code = EXIT_PASS
    elif all(
        result.evidence == "SUPPORTED_COMPLETION_SEQUENCE_POINT_FIRST"
        for result in results
    ):
        aggregate = "INCONCLUSIVE"
        aggregate_evidence = "CONSISTENT_SUPPORT_FOR_FIX"
        exit_code = EXIT_INCONCLUSIVE
    else:
        aggregate = "INCONCLUSIVE"
        aggregate_evidence = "MIXED_OR_INCOMPLETE_EVIDENCE"
        exit_code = EXIT_INCONCLUSIVE

    print(
        f"ORACLE RESULT={aggregate} EVIDENCE={aggregate_evidence} "
        f"TRIALS={expected_trials} SHA256={actual_hash} "
        f"RUNTIME_LABEL={runtime_label or 'UNKNOWN'}"
    )
    return exit_code


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("stdout", type=Path)
    parser.add_argument("--expected-trials", type=int, default=3)
    parser.add_argument("--expected-sha256")
    args = parser.parse_args()

    try:
        return evaluate(args.stdout, args.expected_trials, args.expected_sha256)
    except (OSError, UnicodeError, InvalidEvidence) as exc:
        print(f"ORACLE RESULT=INCONCLUSIVE EVIDENCE=INVALID_INPUT REASON={exc}")
        return EXIT_INVALID


if __name__ == "__main__":
    sys.exit(main())
