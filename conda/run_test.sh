#!/bin/bash

set -euo pipefail

# Print help text, which will fail if dependencies are missing
siemens_to_ismrmrd -h
siemens_to_ismrmrd -v

cd test/
pytest --download-all --echo-log-on-failure