#!/usr/bin/env bash
set -euo pipefail

git -C Libraries/nanovg fetch --no-tags origin refs/pull/31/head
revision="$(git -C Libraries/nanovg rev-parse FETCH_HEAD)"
if [[ "$revision" != "8b98892c329b27c6a0c52d4bd4dd429cea2eef27" ]]; then
  printf '::error::NanoVG PR head changed; review the dependency before updating CI\n' >&2
  exit 1
fi
git -C Libraries/nanovg switch --detach "$revision"
