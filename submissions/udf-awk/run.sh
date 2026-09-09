#!/usr/bin/env bash
set -e

if [[ $# -ne 1 ]]; then
  echo "Usage: $0 <input-file>" >&2
  exit 1
fi

cd "$(dirname "$0")"
parallel --pipepart -a "$1" --block 100M mawk -f chunk.awk | mawk -f final.awk
