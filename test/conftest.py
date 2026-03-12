#!/usr/bin/env python3

from __future__ import annotations

import concurrent.futures
import glob
import hashlib
import os
import pytest
import re
import shutil
import socket
import subprocess
import urllib.error
import urllib.request
import yaml

from pathlib import Path
from dataclasses import dataclass, field
from typing import Dict, List, Callable, Set, Any


def pytest_exception_interact(node, call, report):
    if report.failed and node.config.getoption('--echo-log-on-failure'):
        if hasattr(node, 'funcargs') and 'tmp_path' in node.funcargs:
            tmp_path = node.funcargs['tmp_path']
            for log in glob.glob(os.path.join(tmp_path, '*.log*')):
                with open(log, 'r') as logfile:
                    logdata = logfile.read()
                    report.sections.append((log, logdata))

def pytest_runtest_teardown(item, nextitem):
    if item.config.getoption('--save-results'):
        output_path = item.config.getoption('--save-results')
        output_path = os.path.join(os.path.abspath(output_path), item.callspec.id)
        if hasattr(item, 'funcargs') and 'tmp_path' in item.funcargs:
            tmp_path = item.funcargs['tmp_path']
            shutil.copytree(tmp_path, output_path, dirs_exist_ok=True)


def pytest_addoption(parser):
    parser.addoption(
        '--data-host', action='store', default='http://gadgetrondata.blob.core.windows.net/gadgetrontestdata/',
        help='Host from which to download test data.'
    )
    parser.addoption(
        '--cache-disable', action='store_true', default=False,
        help='Disables local caching of data files.'
    )
    parser.addoption(
        '--cache-path', action='store', default=os.path.join(os.path.dirname(__file__), "data"),
        help='Location for storing cached data files.'
    )
    parser.addoption(
        '--echo-log-on-failure', action='store_true', default=False,
        help='Capture and print siemens_to_ismrmrd output on test failure.'
    )
    parser.addoption(
        '--save-results', action='store', default="",
        help='Save siemens_to_ismrmrd output in the specified directory.'
    )
    parser.addoption(
        '--download-all', action='store_true', default=False,
        help='Download all test data files before running any tests.'
    )


@pytest.fixture(scope="session")
def data_host_url(request) -> str:
    return request.config.getoption('--data-host')

@pytest.fixture(scope="session")
def cache_disable(request) -> bool:
    return request.config.getoption('--cache-disable')

@pytest.fixture(scope="session")
def cache_path(request, cache_disable) -> Path:
    if cache_disable:
        return None
    return Path(os.path.abspath(request.config.getoption('--cache-path')))

def pytest_generate_tests(metafunc: pytest.Metafunc) -> None:
    """Dynamically generates a test for each test case file"""
    all_test_specs = []
    for filename in glob.glob('cases/*.yaml'):
        spec = Spec.fromfile(filename)
        all_test_specs.append(spec)
    all_test_specs = sorted(all_test_specs, key=lambda s: s.id())
    metafunc.parametrize('spec', all_test_specs, ids=lambda s: s.id())

    if metafunc.config.getoption("--download-all"):
        if metafunc.config.getoption("--cache-disable"):
            pytest.fail("Cannot download all data files when caching is disabled")
        all_test_data = {}
        for spec in all_test_specs:
            all_test_data[spec.datfile] = spec.checksum
        host_url = metafunc.config.getoption("--data-host")
        local_dir = metafunc.config.getoption("--cache-path")
        Fetcher(host_url, local_dir).fetch(all_test_data)

@pytest.fixture
def local_test_data_path(cache_path: Path, tmp_path: Path):
    # If cache_path is disabled, the fetched data will live in the test working directory (tmp_path)
    # PyTest automatically cleans up these directories after 3 runs
    def _local_test_data_path(filename: str) -> str:
        if not cache_path:
            return os.path.join(tmp_path, filename)
        return os.path.join(cache_path, filename)
    return _local_test_data_path

@pytest.fixture
def fetch_test_data(data_host_url: str, local_test_data_path) -> Callable:
    """Fetch test data for an individual test case"""
    def _fetch_test_data(filename: str, checksum: str) -> List[str]:
        local_dir = local_test_data_path("")
        test_files = {filename: checksum}
        Fetcher(data_host_url, local_dir).fetch(test_files)
    return _fetch_test_data


class Fetcher:
    """Concurrently fetches test data from a remote host."""
    def __init__(self, host_url: str, local_dir: Path):
        self.host_url = host_url
        self.local_dir = local_dir

    def fetch(self, test_files: Dict[str,str]) -> List[str]:
        with concurrent.futures.ThreadPoolExecutor() as executor:
            results = list(executor.map(self.download_item, test_files.items()))
        return results

    def download_item(self, item):
        """Fetches test data from the remote data host and caches it locally"""
        filename, checksum = item

        destination = os.path.join(self.local_dir, filename)
        need_to_fetch = True
        if os.path.exists(destination):
            if not os.path.isfile(destination):
                raise RuntimeError(f"Destination '{destination}' exists but is not a file")

            if not self.is_valid(destination, checksum):
                print(f"Destination '{destination}' exists file but checksum does not match... Forcing download")
            else:
                need_to_fetch = False

        if need_to_fetch:
            print(f"Fetching test data: {filename}")
            os.makedirs(os.path.dirname(destination), exist_ok=True)
            url = f"{self.host_url}{filename}"
            self.urlretrieve(url, destination)

        if not self.is_valid(destination, checksum):
            raise RuntimeError(f"Downloaded file '{destination}' does not match checksum")
        return destination


    def is_valid(self, file: Path, digest: str) -> bool:
        if not os.path.isfile(file):
            return False
        def compute_checksum(file: Path) -> str:
            md5 = hashlib.new('md5')
            with open(file, 'rb') as f:
                for chunk in iter(lambda: f.read(65536), b''):
                    md5.update(chunk)
            return md5.hexdigest()
        return digest == compute_checksum(file)

    def urlretrieve(self, url: str, filename: str, retries: int = 5) -> str:
        if retries <= 0:
            raise RuntimeError("Download from {} failed".format(url))
        try:
            with urllib.request.urlopen(url, timeout=60) as connection:
                with open(filename,'wb') as f:
                    for chunk in iter(lambda : connection.read(1024*1024), b''):
                        f.write(chunk)
                return connection.headers["Content-MD5"]
        except (urllib.error.URLError, ConnectionResetError, socket.timeout) as exc:
            print("Retrying connection for file {}, reason: {}".format(filename, str(exc)))
            return self.urlretrieve(url, filename, retries=retries-1)

@dataclass
class Spec():
    """Defines a test case specification"""

    @dataclass
    class MinMax():
        min: int
        max: int

    name: str = ""
    datfile: str = ""
    checksum: str = ""
    measurement: int = 0
    parameter_map: str = ""
    parameter_xsl: str = ""
    extra_args: str = ""
    measurement_id: str = ""
    protocol_name: str = ""
    receiver_channels: int = 0
    field_strength: float = 0.0
    num_waveforms: int = 0
    num_acquisitions: int = 0
    measurement_uid: int = 0
    kspace_step1s: MinMax = None
    kspace_step2s: MinMax = None
    averages: MinMax = None
    slices: MinMax = None
    contrasts: MinMax = None
    phases: MinMax = None
    repetitions: MinMax = None
    sets: MinMax = None
    segments: MinMax = None

    def id(self):
        return f"{self.name}"

    @staticmethod
    def fromfile(filename: str) -> Spec:
        with open(filename, 'r') as file:
            parsed = yaml.safe_load(file)
            name = os.path.basename(filename)
            spec = Spec(
                name=name,
                datfile=parsed['datfile'],
                checksum=parsed['checksum'],
                measurement=parsed['measurement'],
                parameter_map=parsed['parameter_map'],
                parameter_xsl=parsed['parameter_xsl'],
                extra_args=parsed.get('extra_args', ''),
                measurement_id=parsed['measurement_id'],
                protocol_name=parsed['protocol_name'],
                receiver_channels=parsed['receiver_channels'],
                field_strength=parsed['field_strength'],
                num_waveforms=parsed['num_waveforms'],
                num_acquisitions=parsed['num_acquisitions'],
                measurement_uid=parsed['measurement_uid'],
                kspace_step1s=Spec.MinMax(*parsed['kspace_step1s']),
                kspace_step2s=Spec.MinMax(*parsed['kspace_step2s']),
                averages=Spec.MinMax(*parsed['averages']),
                slices=Spec.MinMax(*parsed['slices']),
                contrasts=Spec.MinMax(*parsed['contrasts']),
                phases=Spec.MinMax(*parsed['phases']),
                repetitions=Spec.MinMax(*parsed['repetitions']),
                sets=Spec.MinMax(*parsed['sets']),
                segments=Spec.MinMax(*parsed['segments']),
            )

            return spec