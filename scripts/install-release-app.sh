#!/bin/zsh
set -euo pipefail

script_dir=${0:A:h}
repo_dir=${script_dir:h}
source_app="$repo_dir/build-release/src/mastering_app/DoziMastering_artefacts/Release/Dozi Mastering.app"
destination_app="/Applications/Dozi Mastering.app"
timestamp=$(date +%Y%m%d-%H%M%S)
backup_app="/Applications/Dozi Mastering.backup-$timestamp.app"
staged_app="/Applications/.Dozi Mastering.installing-$timestamp.app"

if [[ ! -x "$source_app/Contents/MacOS/Dozi Mastering" ]]; then
    print -u2 "Release executable not found. Build it first with:"
    print -u2 "  cmake --build '$repo_dir/build-release' --target DoziMastering -j 4"
    exit 1
fi

if pgrep -f "$destination_app/Contents/MacOS/Dozi Mastering" >/dev/null; then
    print -u2 "Dozi Mastering is running. Quit it before installing."
    exit 1
fi

print "Staging current Release build..."
sudo ditto --rsrc --extattr "$source_app" "$staged_app"
sudo touch "$staged_app"

if [[ -d "$destination_app" ]]; then
    print "Preserving the installed version at: $backup_app"
    sudo mv "$destination_app" "$backup_app"
fi

if ! sudo mv "$staged_app" "$destination_app"; then
    print -u2 "Installation failed. Restoring the previous application."
    if [[ -d "$backup_app" && ! -e "$destination_app" ]]; then
        sudo mv "$backup_app" "$destination_app"
    fi
    exit 1
fi

sudo touch "$destination_app"
print "Installed: $destination_app"
print "The Dock now points to this updated application."
if [[ -d "$backup_app" ]]; then
    print "Recoverable backup retained at: $backup_app"
fi
