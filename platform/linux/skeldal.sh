#!/bin/sh
# Gates of Skeldal launcher
#
# The game reads its data relative to the current directory, so run it from
# the directory where this script (and skeldal_bin) lives - launchers and
# .desktop files usually start it with $HOME as cwd. Errors are reported by
# skeldal_bin itself (stderr + SDL message box).
cd "$(dirname "$(readlink -f "$0")")" || exit 1
exec ./skeldal_bin "$@"
