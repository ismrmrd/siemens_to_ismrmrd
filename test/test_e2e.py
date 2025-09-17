from __future__ import annotations

import os
import itertools
import subprocess
from typing import Dict, List, Callable, Set, Any

import pytest
import ismrmrd

from conftest import Spec


def test_e2e(spec: Spec, request, fetch_test_data, convert_datfile, validate_output):
    """The main test function for each test case.

    One instance is created for each case file in ./cases/.
    These instances and related fixtures are defined in conftest.py.
    """
    if not request.config.getoption("--download-all"):
        fetch_test_data(spec.datfile, spec.checksum)

    output_file = convert_datfile(spec)
    validate_output(spec, output_file)

@pytest.fixture
def convert_datfile(local_test_data_path, tmp_path):
    """Runs siemens_to_ismrmrd on the input test data, producing an output file."""
    def _convert_datfile(spec):
        input_file = local_test_data_path(spec.datfile)
        output_file = os.path.join(tmp_path, spec.name + ".output.ismrmrd")

        cmd = f"siemens_to_ismrmrd -m {spec.parameter_map} -x {spec.parameter_xsl} -z {spec.measurement} {spec.extra_args}"
        cmd += f" < {input_file} > {output_file}"
        command = ['bash', '-c', cmd]
        # print(f"Run: {command}")

        log_stderr_filename = os.path.join(tmp_path, f"siemens_to_ismrmrd_{spec.name}.log.err")
        with open(log_stderr_filename, 'w') as log_stderr:
            result = subprocess.run(command, stderr=log_stderr, cwd=tmp_path)
            if result.returncode != 0:
                pytest.fail(f"siemens_to_ismrmrd failed with return code {result.returncode}. See {log_stderr_filename} for details.")

        return output_file

    return _convert_datfile

@pytest.fixture
def validate_output(local_test_data_path):
    """Validates each image (data and header) in the output file against the reference file."""
    def _validate_output(spec: Spec, output_file: str) -> None:
        with ismrmrd.ProtocolDeserializer(output_file) as reader:
            stream = reader.deserialize()
            header = next(stream, None)
            if not isinstance(header, ismrmrd.xsd.ismrmrdHeader):
                pytest.fail(f"First item in output file is not an ismrmrdHeader, but {type(header)}")

            measInfo = header.measurementInformation
            assert measInfo.measurementID == spec.measurement_id
            assert measInfo.protocolName == spec.protocol_name

            acqInfo = header.acquisitionSystemInformation
            assert acqInfo.systemVendor == "SIEMENS"
            assert acqInfo.receiverChannels == spec.receiver_channels
            assert acqInfo.systemFieldStrength_T == pytest.approx(spec.field_strength, rel=1e-7)

            num_acquisitions = 0
            num_waveforms = 0
            measurement_uids = set()
            ke1 = set()
            ke2 = set()
            average = set()
            slice_ = set()
            contrast = set()
            phase = set()
            repetition = set()
            set_ = set()
            segment = set()
            for item in stream:
                if isinstance(item, ismrmrd.Acquisition):
                    num_acquisitions += 1
                    measurement_uids.add(item.measurement_uid)
                    ke1.add(item.idx.kspace_encode_step_1)
                    ke2.add(item.idx.kspace_encode_step_2)
                    average.add(item.idx.average)
                    slice_.add(item.idx.slice)
                    contrast.add(item.idx.contrast)
                    phase.add(item.idx.phase)
                    repetition.add(item.idx.repetition)
                    set_.add(item.idx.set)
                    segment.add(item.idx.segment)

                elif isinstance(item, ismrmrd.Waveform):
                    num_waveforms += 1
                else:
                    pytest.fail(f"Unexpected item type in output file: {type(item)}")

            assert num_acquisitions == spec.num_acquisitions, f"Number of acquisitions ({num_acquisitions}) does not match expected ({spec.num_acquisitions})"
            assert num_waveforms == spec.num_waveforms, f"Number of waveforms ({num_waveforms}) does not match expected ({spec.num_waveforms})"
            assert len(measurement_uids) == 1
            assert measurement_uids.pop() == spec.measurement_uid
            assert min(ke1) == spec.kspace_step1s.min
            assert max(ke1) == spec.kspace_step1s.max
            assert min(ke2) == spec.kspace_step2s.min
            assert max(ke2) == spec.kspace_step2s.max
            assert min(average) == spec.averages.min
            assert max(average) == spec.averages.max
            assert min(slice_) == spec.slices.min
            assert max(slice_) == spec.slices.max
            assert min(contrast) == spec.contrasts.min
            assert max(contrast) == spec.contrasts.max
            assert min(phase) == spec.phases.min
            assert max(phase) == spec.phases.max
            assert min(repetition) == spec.repetitions.min
            assert max(repetition) == spec.repetitions.max
            assert min(set_) == spec.sets.min
            assert max(set_) == spec.sets.max
            assert min(segment) == spec.segments.min
            assert max(segment) == spec.segments.max

    return _validate_output