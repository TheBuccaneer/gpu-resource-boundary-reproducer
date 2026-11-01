#!/usr/bin/env python3
"""Offline data oracle. Standard library only; never imports or calls OpenCL."""
import argparse
import json
from pathlib import Path


def expected(mode):
    return [((i ^ 0x12345678) + 1) if mode == 'kernel' else 0x01234567
            for i in range(64)] + [0xa55aa55a] * (4096 - 64)


def matches(data, mode):
    return data == expected(mode)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--self-check', action='store_true')
    p.add_argument('--log', type=Path)
    p.add_argument('--mode', choices=['kernel', 'buffer'])
    a = p.parse_args()
    if a.self_check:
        for mode in ['kernel', 'buffer']:
            valid = expected(mode)
            assert matches(valid, mode)
            for index in [0, 63, 64, 4095]:
                mutated = valid.copy()
                mutated[index] ^= 1
                assert not matches(mutated, mode)
            assert not matches(valid[:-1], mode)
        print('PASS: CPU-only oracle controls detect payload, boundary-guard, last-guard and length mutations.')
        return 0
    if not a.log or not a.mode:
        p.error('Choose --self-check or --log FILE --mode kernel|buffer.')
    rows = [json.loads(line) for line in a.log.read_text().splitlines() if line.strip()]
    blobs = [r['data_u32_hex'] for r in rows if 'data_u32_hex' in r]
    if len(blobs) != 1:
        raise SystemExit('FAIL: expected exactly one full data snapshot from one fresh-process case.')
    blob = blobs[0]
    if len(blob) != 4096 * 8:
        raise SystemExit('FAIL: wrong full data length.')
    data = [int(blob[i:i + 8], 16) for i in range(0, len(blob), 8)]
    if not matches(data, a.mode):
        raise SystemExit('FAIL: data/guard mismatch.')
    if any(r.get('pass') is False or ('expected' in r and r.get('status') != r['expected']) for r in rows):
        raise SystemExit('FAIL: API/check failure present despite valid data.')
    if not rows or rows[-1].get('outcome') != 'expected_observations_complete':
        raise SystemExit('FAIL: missing successful case terminator.')
    print('PASS: complete stored data and recorded checks agree. Assess process/provenance records separately.')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
