#!/usr/bin/env python3
"""Optional real XTEST/RECORD regression; injects only into private Xvfb servers.
Requires g++, Xvfb, libxcb-record, libxcb-xinput, libXi, libXtst, libX11 and libxkbcommon development files.
Run from any directory with: python3 tests/xtest_mouse_smoke.py [--sanitize]
"""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--sanitize", action="store_true", help="run with AddressSanitizer")
args = parser.parse_args()
flags = ["-O1", "-g", "-fsanitize=address", "-fno-omit-frame-pointer"] if args.sanitize else ["-O2"]
root = Path(__file__).resolve().parent.parent
with tempfile.TemporaryDirectory(prefix="gamescope-xtest-") as directory:
    binaries = []
    for fixture in ("xtest_mouse_smoke", "xtest_capture_smoke", "xtest_dispatch_smoke"):
        binary = Path(directory) / fixture
        subprocess.run(["g++", "-std=c++20", *flags, "-I" + str(root / "src"),
                        str(root / "tests/fixtures" / (fixture + ".cpp")), "-o", str(binary),
                        "-lxcb-record", "-lxcb-xinput", "-lxcb", "-lXtst", "-lXi", "-lX11", "-lxkbcommon"], check=True)
        binaries.append(binary)
    for screens in (1, 2):
        read_fd, write_fd = os.pipe()
        command = ["Xvfb", "-displayfd", str(write_fd), "-nolisten", "tcp"]
        for screen in range(screens):
            command += ["-screen", str(screen), "640x480x24"]
        with open(Path(directory) / f"xvfb-{screens}.log", "w") as log:
            server = subprocess.Popen(command, pass_fds=(write_fd,), stdout=log, stderr=log)
            os.close(write_fd)
            try:
                with os.fdopen(read_fd) as reader:
                    display = ":" + reader.readline().strip()
                if display == ":":
                    raise RuntimeError("private Xvfb did not start")
                for binary in binaries:
                    subprocess.run([str(binary), display], check=True, timeout=15)
            finally:
                server.terminate()
                server.wait(timeout=5)
