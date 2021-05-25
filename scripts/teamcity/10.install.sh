
set -eux

SCRIPT_DIR="$(cd "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null && pwd)"
source "$SCRIPT_DIR/commands.sh"

echo "Installing version: $NODE_VERSION -- arch: $NODE_ARCH";

export EXPERIMENTAL_NODE_GYP_PYTHON3=yes

nvm_load
nvm install $NODE_VERSION $NODE_ARCH --experimental-gypjs