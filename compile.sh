#!/bin/env bash
set -e

ARG=$1

if [ -z "$1" ]; then

    ARG="release"

fi

export CC=gcc-13
export CXX=g++-13

buildtype=""
DEBUG="false"

if [ "$ARG" == "--help" ] || [ "$ARG" == "-h" ]; then

    cat <<'EOF'
Usage: ./compile.sh [options] [command] [subcommand]

Compile script for building aegisub and additional tools

Options:
    -h, --help         display help for command

Commands:
    release
    debug
    clean
    dev
    runner
    flatpak

SubCommands:
    ignore
EOF

    exit 0

elif [ "$ARG" == "release" ]; then
    buildtype="release"
elif [ "$ARG" == "debug" ]; then
    buildtype="debugoptimized"
elif [ "$ARG" == "clean" ]; then
    rm -rf build/
    exit 0
elif [ "$ARG" == "dev" ]; then
    buildtype="debugoptimized"
    DEBUG="true"
elif [ "$ARG" == "runner" ]; then

    ACT=$(which act)

    sudo "$ACT" -P ubuntu-latest=ghcr.io/catthehacker/ubuntu:act-22.04
    exit 0
elif [ "$ARG" == "flatpak" ]; then
    BUILD_DIR="flatpak-build"
    UNIQUE_ID="org.aegisub.Aegisub"
    flatpak-builder --gpg-sign=E90FB7C6F9097B980187EAE9C919C07E0A9371D6 --user --force-clean "$BUILD_DIR" --repo=./.repo "$UNIQUE_ID.yml"

    flatpak install --user --reinstall --assumeyes "$(pwd)/.repo" "$UNIQUE_ID.Debug"
    flatpak install --user --reinstall --assumeyes "$(pwd)/.repo" "$UNIQUE_ID.Locale"
    flatpak install --user --reinstall --assumeyes "$(pwd)/.repo" "$UNIQUE_ID"

    flatpak build-bundle .repo/ aegisub.flatpak "$UNIQUE_ID"

    flatpak run "$UNIQUE_ID"

    exit 0
else

    echo "NOT supported option: $ARG"
    exit 1

fi

# CONFIGURE

bash -c "meson setup build -Dbuildtype=$buildtype -Dwx_version=3.2.3 -Dcredit='Totto local build'"

if [ $DEBUG == "true" ]; then
    nodemon --watch src/ -e .cpp,.h --exec "sudo meson compile -C build && ./build/aegisub || exit 1"
    exit 0
fi

## COMPILE
meson compile -C build

## run tests
meson test -C build --verbose "gtest main"

if [ ! -f "build/aegisub" ]; then
    echo "Failed to build aegisub. Aborting"
    exit 4
fi

meson compile -C build linux-dependency-control
meson compile -C build aegisub.desktop
meson compile -C build ubuntu.aegsiub.deb
meson compile -C build ubuntu.assdraw.deb

# INSTALL if no second parameter is given, not the best, but it works
if [ -z "$2" ]; then
    sudo dpkg -i "build/$(ls build/ | grep "aegisub_.*deb")" || sudo apt-get -f install
    sudo dpkg -i "build/$(ls build/ | grep "aegisub-l10n_.*deb")" || sudo apt-get -f install
    sudo dpkg -i "build/$(ls build/ | grep "assdraw.*deb")" || sudo apt-get -f install
else

    #noop
    bash -c ""

    # enable if needed
    # sudo meson install -C build

fi

#TODO make icons for ass work!
#icons for .ass .ssa
#mime type,

# TODO compile each dependency local with the newest (stable?) version and then distruibute them as deb, and their also gonna be neede for flatpak support!
