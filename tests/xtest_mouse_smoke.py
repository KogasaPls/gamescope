#!/usr/bin/env python3
"""Optional real XTEST/RECORD regression; injects only into private Xvfb servers.
Requires g++, Xvfb, libxcb-record, libXtst and libX11 development files.
Run from any directory with: python3 tests/xtest_mouse_smoke.py
"""
import os
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
with tempfile.TemporaryDirectory(prefix="gamescope-xtest-") as directory:
    binary = Path(directory) / "smoke"
    subprocess.run(["g++", "-std=c++20", "-O2", "-I" + str(root / "src"),
                    str(root / "tests/fixtures/xtest_mouse_smoke.cpp"), "-o", str(binary),
                    "-lxcb-record", "-lxcb", "-lXtst", "-lX11"], check=True)
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
                subprocess.run([str(binary), display], check=True, timeout=15)
            finally:
                server.terminate()
                server.wait(timeout=5)
