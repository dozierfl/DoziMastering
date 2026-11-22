#!/bin/zsh
set -euo pipefail

if (( $# < 1 )); then
    print -u2 "Usage: $0 \"commit message\" [--skip-tests]"
    exit 2
fi

commit_message=$1
skip_tests=${2:-}
script_dir=${0:A:h}
repo_dir=${script_dir:h}
cd "$repo_dir"

if [[ -n $(git diff --cached --name-only) ]]; then
    print -u2 "The index already contains staged changes. Commit or unstage them first."
    exit 1
fi

git diff --check

if [[ "$skip_tests" != "--skip-tests" ]]; then
    cmake --build build-release --target DoziMastering dozi_hardware_control_tests -j 4
    ctest --test-dir build-release --output-on-failure
fi

git add -- \
    src/mastering_app/GlueCompressorHardwarePanel.h \
    src/mastering_app/MasteringChainView.cpp \
    src/mastering_app/MasteringChainView.h \
    src/mastering_app/MasteringComponent.cpp \
    src/mastering_app/MasteringComponent.h \
    src/mastering_app/MasteringHardwareControls.cpp \
    src/mastering_app/MasteringHardwareControls.h \
    src/mastering_app/MasteringModuleInspector.cpp \
    src/mastering_app/MasteringModuleInspector.h \
    tests/HardwareControlTests.cpp \
    scripts/install-release-app.sh \
    scripts/commit-and-push.sh

if git diff --cached --quiet; then
    print "Nothing to commit."
    exit 0
fi

git commit -m "$commit_message"
current_branch=$(git branch --show-current)
if [[ -z "$current_branch" ]]; then
    print -u2 "Cannot push from a detached HEAD."
    exit 1
fi
git push origin "$current_branch"
print "Committed and pushed $current_branch to origin."
