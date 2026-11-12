#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
engine=${CONTAINER_ENGINE:-docker}
# Multi-architecture images from kobuild v0.5.3.
qt5_image=ghcr.io/nicoverbruggen/kobuild:qt5-v0.5.3@sha256:cfb68715d110d02e91df589f1e8940e9781a91d88b2aaee536f5f3830876fb25
qt6_image=ghcr.io/nicoverbruggen/kobuild:qt6-v0.5.3@sha256:b939301323e1bed0bf6b0bee43b8791db2efdf01231d4245c1f972d57433644a
qts='5 6'
version=${NHM_VERSION:-}
jobs=${JOBS:-2}

usage() {
    echo 'Usage: sh tools/build.sh [--qt 5|6] [--version VERSION]'
}
while [ "$#" -gt 0 ]; do
    case "$1" in
        --help|-h) usage; exit 0 ;;
        --qt|--version)
            [ "$#" -ge 2 ] || { usage >&2; exit 2; }
            case "$1" in
                --qt)
                    case "$2" in 5|6) qts=$2 ;; *) usage >&2; exit 2 ;; esac ;;
                --version) version=$2 ;;
            esac
            shift 2 ;;
        *) usage >&2; exit 2 ;;
    esac
done
if [ -z "$version" ]; then
    version=$(git -C "$root" describe --tags --always --dirty)
fi
case "$version" in
    ''|*[!A-Za-z0-9._+-]*) echo "Invalid version: $version" >&2; exit 2 ;;
esac
case "$jobs" in
    ''|0|*[!0-9]*) echo 'JOBS must be a positive integer' >&2; exit 2 ;;
esac
[ -f "$root/NickelHook/NickelHook.mk" ] || {
    echo 'Initialize NickelHook with git submodule update --init first.' >&2
    exit 1
}

for qt in $qts; do
    case "$qt" in 5) image=$qt5_image ;; 6) image=$qt6_image ;; esac
    output="$root/build/qt$qt"
    mkdir -p "$output"
    set --
    if [ "$("$engine" info --format '{{.Host.Security.Rootless}}' 2>/dev/null || true)" = true ]; then
        set -- --userns=keep-id
    fi
    "$engine" run --rm --network none "$@" --user "$(id -u):$(id -g)" -e HOME=/tmp \
        -v "$root:/src:ro,z" -v "$output:/work:rw,z" -w /work --entrypoint sh "$image" -ec '
        qt=$1 version=$2 jobs=$3
        case "$qt" in 5) target=koboroot; package=KoboRoot.tgz ;; 6) target=kobo; package=Kobo.tgz ;; esac
        # Each target builds a fresh copy, including its pinned NickelHook.
        # Never reuse objects compiled with another Qt version or SDK.
        scratch=$(mktemp -d /work/source.XXXXXX)
        trap '\''rm -rf "$scratch"'\'' EXIT HUP INT TERM
        rm -f "/work/$package" /work/libnickelhome.so
        cp -R /src/Makefile /src/src /src/res /src/NickelHook "$scratch/"
        cd "$scratch"
        set -- NH_QT_MAJOR="$qt" VERSION="$version"
        if [ "$qt" = 5 ]; then
            export PKG_CONFIG_LIBDIR=/tc/qt5/sysroot/usr/lib/pkgconfig
            set -- "$@" CROSS_COMPILE=/tc/qt5/bin/ PKG_CONFIG=pkg-config \
                MOC=/tc/qt5/sysroot/usr/bin/moc STRIP=arm-linux-gnueabihf-strip
        fi
        make "$@" clean
        make "$@" -j"$jobs" all
        make "$@" strip
        make "$@" "$target"
        python3 /src/test/package.py "$package" --qt "$qt" --readelf arm-linux-gnueabihf-readelf
        cp src/libnickelhome.so "$package" /work/
        ' sh "$qt" "$version" "$jobs"
    echo "Built Qt$qt package in $output"
done
