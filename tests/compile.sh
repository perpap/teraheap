#!/usr/bin/env bash
set -euo pipefail

usage() {
  local code=$1
  cat <<'EOF'
Usage:
  ./compile.sh [-b evac|full|both] [-c clean|distclean]

Options:
  -b  Build suite: evac, full, both (default: both)
  -c  Cleanup mode: clean, distclean (default: no clean)

Examples:
  ./compile.sh
  ./compile.sh -b evac
  ./compile.sh -b evac -c clean
  ./compile.sh -b both -c distclean
EOF
  exit ${code}
}

build_suite() {
  local dir="$1"
  local clean_mode="$2"
  pushd "$dir" > /dev/null

  # Pass cleanup mode to the suite compile script
  ./compile.sh "$clean_mode"

  popd > /dev/null
}

suite="both"
cleanup="none"

while getopts ":b:c:h" opt; do
  case "$opt" in
    b) suite="$OPTARG"
      ;;
    c) cleanup="$OPTARG"
      ;;
    h) usage 0
      ;;
    :)
      echo "Error: -$OPTARG requires an argument" >&2
      usage 1
      ;;
    \?)
      echo "Error: unknown option -$OPTARG" >&2
      usage 1
      ;;
  esac
done
shift $((OPTIND - 1))

# No positional args expected
if [[ $# -ne 0 ]]; then
  echo "Error: unexpected positional arguments: $*" >&2
  usage 1
fi

case "$suite" in
  evac|full|both) ;;
  *)
    echo "Error: invalid build suite: '$suite' (expected evac|full|both)" >&2
    usage 1
    ;;
esac

case "$cleanup" in
  clean|distclean|none) ;;
  *) 
    echo "Error: invalid cleanup mode: '$cleanup' (expected clean|distclean)" >&2
    usage 1
    ;;
esac

case "$suite" in
  evac)
    build_suite "./g1_evacuations" ${cleanup}
    ;;
  full)
    build_suite "./g1_full_gc" ${cleanup}
    ;;
  both)
    build_suite "./g1_evacuations" ${cleanup}
    build_suite "./g1_full_gc" ${cleanup}
    ;;
esac
