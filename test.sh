#!/bin/bash
set -eou pipefail

make dev
#./example fetch
passh -L .example ./example ${@:-dev}
