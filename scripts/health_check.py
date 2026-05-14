#!/usr/bin/env python3
"""
health_check.py — Check status of all FsNext services and dependencies.

Checks:
  1. Build directory exists and has executable
  2. Dependencies available (cmake, ninja, qt, vcpkg)
  3. API endpoint reachable (api.fshare.vn)
  4. Disk space sufficient
  5. Source files integrity (no missing expected files)

Exit code: 0 = all pass, 1 = failures detected
"""

import os
import sys
import shutil
import subprocess
import urllib.request
import json

PASS = "[PASS]"
FAIL = "[FAIL]"
WARN = "[WARN]"
INFO = "[INFO]"

failures = 0
project_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def check(label, condition, fail_msg="", warn_only=False):
    global failures
    if condition:
        print(f"  {PASS} {label}")
        return True
    else:
        if warn_only:
            print(f"  {WARN} {label}: {fail_msg}")
        else:
            print(f"  {FAIL} {label}: {fail_msg}")
            failures += 1
        return False


def run_cmd(cmd):
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=10)
        return result.returncode == 0, result.stdout.strip()
    except Exception:
        return False, ""


def main():
    global failures

    print("=" * 50)
    print("  FsNext Health Check")
    print("=" * 50)
    print()

    # 1. Build status
    print("1. Build Status")
    build_dir = os.path.join(project_dir, "build")
    check("Build directory exists", os.path.isdir(build_dir),
          "Run cmake configure first", warn_only=True)

    exe_path = os.path.join(build_dir, "Release", "fsharetool.exe")
    if not os.path.exists(exe_path):
        exe_path = os.path.join(build_dir, "fsharetool.exe")
    check("Executable exists", os.path.isfile(exe_path),
          f"Not found at {exe_path}", warn_only=True)
    print()

    # 2. Dependencies
    print("2. Dependencies")
    ok, ver = run_cmd(["cmake", "--version"])
    check("CMake", ok, "Not found in PATH")

    ok, ver = run_cmd(["ninja", "--version"])
    check("Ninja", ok, "Not found in PATH")

    ok, ver = run_cmd(["git", "--version"])
    check("Git", ok, "Not found in PATH", warn_only=True)

    vcpkg_root = os.environ.get("VCPKG_ROOT", "")
    check("VCPKG_ROOT set", bool(vcpkg_root), "Environment variable not set")
    print()

    # 3. API Connectivity
    print("3. API Connectivity")
    try:
        req = urllib.request.Request(
            "https://api.fshare.vn/api/service/getlatestversion?type=windows",
            headers={"Content-Type": "application/json"}
        )
        resp = urllib.request.urlopen(req, timeout=10)
        check("api.fshare.vn reachable", resp.status == 200, f"HTTP {resp.status}")
    except Exception as e:
        check("api.fshare.vn reachable", False, str(e))
    print()

    # 4. Disk Space
    print("4. Disk Space")
    drive = os.path.splitdrive(project_dir)[0] or "C:"
    total, used, free = shutil.disk_usage(drive + "\\")
    free_gb = free / (1024 ** 3)
    check(f"Free space on {drive} ({free_gb:.1f} GB)", free_gb > 1.0,
          f"Less than 1 GB free ({free_gb:.1f} GB)")
    print()

    # 5. Source Files
    print("5. Source File Integrity")
    expected_docs = [
        "docs/00_module_map.md",
        "docs/01_features.md",
        "docs/02_api_contracts.md",
        "docs/03_data_models.md",
        "docs/04_business_workflows.md",
        "docs/05_dependency_graph.md",
        "docs/06_edge_cases.md",
        "docs/07_architecture.md",
        "docs/BACKLOG.md",
        "CHECKLIST.md",
        "PLAN.md",
        "STATUS.md",
    ]
    for f in expected_docs:
        full = os.path.join(project_dir, f)
        check(f, os.path.isfile(full), "File missing")
    print()

    # Summary
    print("=" * 50)
    if failures == 0:
        print(f"  Result: ALL CHECKS PASSED")
    else:
        print(f"  Result: {failures} FAILURE(S) DETECTED")
    print("=" * 50)

    return 1 if failures > 0 else 0


if __name__ == "__main__":
    sys.exit(main())
