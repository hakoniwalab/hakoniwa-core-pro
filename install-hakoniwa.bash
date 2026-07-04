#!/usr/bin/env bash
# Installer that clones, builds, and installs hakoniwa-core-pro.

set -euo pipefail

DEFAULT_REPO_URL="https://github.com/hakoniwalab/hakoniwa-core-pro.git"
DEFAULT_REF="main"
DEFAULT_PREFIX="/usr/local/hakoniwa"
CONFIG_DIR="/etc/hakoniwa"
MMAP_DIR="/var/lib/hakoniwa/mmap"

ACTION="install"
REPO_URL="${HAKONIWA_REPO_URL:-${DEFAULT_REPO_URL}}"
REF="${HAKONIWA_REF:-${DEFAULT_REF}}"
PREFIX="${HAKONIWA_HOME:-${DEFAULT_PREFIX}}"
YES=0
SETUP_SHELL=0
DRY_RUN=0
KEEP_SOURCE=0

usage() {
    cat <<'USAGE'
Usage:
  install-hakoniwa.bash [install] [options]
  install-hakoniwa.bash uninstall [options]
  install-hakoniwa.bash reinstall [options]
  install-hakoniwa.bash check [options]

Options:
  --prefix PATH       Install prefix. Default: /usr/local/hakoniwa
  --repo URL          Git repository URL.
  --ref REF           Git branch or tag to install. Default: main
  --version VERSION   Alias for --ref, intended for release tags.
  --setup-shell       Append source line to ~/.bashrc or ~/.zshrc.
  --keep-source       Keep the temporary cloned source tree.
  --yes, -y           Do not ask for confirmation.
  --dry-run           Print actions without changing files.
  --help, -h          Show this help.

Examples:
  curl -fsSL https://raw.githubusercontent.com/hakoniwalab/hakoniwa-core-pro/main/install-hakoniwa.bash | bash
  curl -fsSL https://raw.githubusercontent.com/hakoniwalab/hakoniwa-core-pro/main/install-hakoniwa.bash | bash -s -- --version v1.3.0
  curl -fsSL https://raw.githubusercontent.com/hakoniwalab/hakoniwa-core-pro/main/install-hakoniwa.bash | bash -s -- uninstall
USAGE
}

log() {
    printf '%s\n' "$*"
}

die() {
    printf 'ERROR: %s\n' "$*" >&2
    exit 1
}

run() {
    if [ "${DRY_RUN}" -eq 1 ]; then
        printf '[dry-run]'
        printf ' %q' "$@"
        printf '\n'
        return 0
    fi
    "$@"
}

need_cmd() {
    command -v "$1" >/dev/null 2>&1 || die "required command not found: $1"
}

as_root() {
    if [ "$(id -u)" -eq 0 ]; then
        run "$@"
    else
        need_cmd sudo
        run sudo "$@"
    fi
}

confirm() {
    [ "${YES}" -eq 1 ] && return 0
    if [ ! -r /dev/tty ]; then
        die "confirmation requires a terminal. Re-run with --yes to skip prompts."
    fi
    printf '%s [y/N] ' "$1" > /dev/tty
    read -r answer < /dev/tty
    case "${answer}" in
        y|Y|yes|YES) return 0 ;;
        *) die "cancelled" ;;
    esac
}

detect_os() {
    case "$(uname -s)" in
        Darwin) printf 'macos' ;;
        Linux)
            if [ -r /etc/os-release ]; then
                os_id="$(. /etc/os-release && printf '%s' "${ID:-}")"
                [ "${os_id}" = "ubuntu" ] || die "unsupported Linux distribution: ${os_id:-unknown}. This installer supports Ubuntu and macOS."
                printf 'ubuntu'
            else
                die "unsupported Linux distribution: missing /etc/os-release"
            fi
            ;;
        *) die "unsupported OS: $(uname -s)" ;;
    esac
}

make_temp_dir() {
    mktemp -d 2>/dev/null || mktemp -d -t hakoniwa-install
}

write_env_file() {
    tmp_file="${TMPDIR:-/tmp}/hakoniwa-env.$$.bash"
    cat > "${tmp_file}" <<EOF
export HAKONIWA_HOME="${PREFIX}"
export PATH="\${HAKONIWA_HOME}/bin:\${PATH}"
export CMAKE_PREFIX_PATH="\${HAKONIWA_HOME}:\${CMAKE_PREFIX_PATH:-}"
export PKG_CONFIG_PATH="\${HAKONIWA_HOME}/lib/pkgconfig:\${PKG_CONFIG_PATH:-}"

case "\$(uname -s)" in
    Linux)
        export LD_LIBRARY_PATH="\${HAKONIWA_HOME}/lib:\${LD_LIBRARY_PATH:-}"
        ;;
    Darwin)
        export DYLD_LIBRARY_PATH="\${HAKONIWA_HOME}/lib:\${DYLD_LIBRARY_PATH:-}"
        ;;
esac
EOF
    as_root mkdir -p "${PREFIX}"
    as_root cp "${tmp_file}" "${PREFIX}/env.bash"
    rm -f "${tmp_file}"
}

setup_shell() {
    shell_rc=""
    shell_name="$(basename "${SHELL:-}")"
    case "${shell_name}" in
        zsh) shell_rc="${HOME}/.zshrc" ;;
        bash) shell_rc="${HOME}/.bashrc" ;;
        *) shell_rc="${HOME}/.profile" ;;
    esac

    line="source ${PREFIX}/env.bash"
    if [ -f "${shell_rc}" ] && grep -Fq "${line}" "${shell_rc}"; then
        log "Shell profile already contains: ${line}"
        return 0
    fi
    if [ "${DRY_RUN}" -eq 1 ]; then
        log "[dry-run] append '${line}' to ${shell_rc}"
        return 0
    fi
    printf '\n# hakoniwa-core-pro\n%s\n' "${line}" >> "${shell_rc}"
    log "Updated shell profile: ${shell_rc}"
}

install_action() {
    os_name="$(detect_os)"
    need_cmd git
    need_cmd cmake

    if [ "${DRY_RUN}" -eq 1 ]; then
        tmp_dir="${TMPDIR:-/tmp}/hakoniwa-install.dry-run"
    else
        tmp_dir="$(make_temp_dir)"
    fi
    src_dir="${tmp_dir}/hakoniwa-core-pro"

    log "Installing hakoniwa-core-pro"
    log "  os     : ${os_name}"
    log "  repo   : ${REPO_URL}"
    log "  ref    : ${REF}"
    log "  prefix : ${PREFIX}"
    confirm "Clone, build, and install?"

    if [ "${DRY_RUN}" -eq 1 ]; then
        log "[dry-run] would git clone ${REPO_URL} (${REF}) into ${src_dir}"
        log "[dry-run] would run install.bash with INSTALL_PREFIX=${PREFIX}"
        log "[dry-run] would create ${PREFIX}/env.bash"
        return 0
    fi

    run git clone --depth 1 --branch "${REF}" --recurse-submodules "${REPO_URL}" "${src_dir}"
    (
        cd "${src_dir}"
        INSTALL_PREFIX="${PREFIX}" bash ./install.bash
    )

    write_env_file
    if [ "${SETUP_SHELL}" -eq 1 ]; then
        setup_shell
    fi

    if [ "${KEEP_SOURCE}" -eq 1 ]; then
        log "Source tree kept at: ${src_dir}"
    else
        run rm -rf "${tmp_dir}"
    fi

    check_action
    log ""
    log "Installation completed."
    log "To use this install in the current shell:"
    log "  source ${PREFIX}/env.bash"
}

uninstall_action() {
    log "Uninstalling hakoniwa-core-pro"
    log "  prefix : ${PREFIX}"
    log "  config : ${CONFIG_DIR}"
    log "  mmap   : /var/lib/hakoniwa"
    confirm "Remove installed files?"

    as_root rm -rf "${PREFIX}"
    as_root rm -rf "${CONFIG_DIR}"
    as_root rm -rf /var/lib/hakoniwa
    log "Uninstall completed."
}

check_action() {
    missing=0
    for path in \
        "${PREFIX}" \
        "${PREFIX}/env.bash" \
        "${PREFIX}/include/hakoniwa" \
        "${PREFIX}/lib" \
        "${PREFIX}/share/hakoniwa/offset" \
        "${MMAP_DIR}"
    do
        if [ -e "${path}" ]; then
            log "  [ EXISTS ] ${path}"
        else
            log "  [ MISSING] ${path}"
            missing=1
        fi
    done

    if [ "${missing}" -ne 0 ]; then
        die "installation check failed"
    fi
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        install|uninstall|reinstall|check)
            ACTION="$1"
            shift
            ;;
        clean)
            log "WARNING: 'clean' is deprecated for uninstalling. Use 'uninstall' instead."
            ACTION="uninstall"
            shift
            ;;
        --prefix)
            [ "$#" -ge 2 ] || die "--prefix requires a value"
            PREFIX="$2"
            shift 2
            ;;
        --repo)
            [ "$#" -ge 2 ] || die "--repo requires a value"
            REPO_URL="$2"
            shift 2
            ;;
        --ref)
            [ "$#" -ge 2 ] || die "--ref requires a value"
            REF="$2"
            shift 2
            ;;
        --version)
            [ "$#" -ge 2 ] || die "--version requires a value"
            REF="$2"
            shift 2
            ;;
        --setup-shell)
            SETUP_SHELL=1
            shift
            ;;
        --keep-source)
            KEEP_SOURCE=1
            shift
            ;;
        --yes|-y)
            YES=1
            shift
            ;;
        --dry-run)
            DRY_RUN=1
            shift
            ;;
        --help|-h)
            usage
            exit 0
            ;;
        *)
            die "unknown argument: $1"
            ;;
    esac
done

case "${ACTION}" in
    install)
        install_action
        ;;
    uninstall)
        uninstall_action
        ;;
    reinstall)
        uninstall_action
        install_action
        ;;
    check)
        check_action
        ;;
    *)
        die "unknown action: ${ACTION}"
        ;;
esac
