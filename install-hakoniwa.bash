#!/usr/bin/env bash
# Installer for prebuilt hakoniwa-core-pro release archives.

set -euo pipefail

REPO_OWNER="hakoniwalab"
REPO_NAME="hakoniwa-core-pro"
DEFAULT_PREFIX="/usr/local/hakoniwa"
CONFIG_DIR="/etc/hakoniwa"
MMAP_DIR="/var/lib/hakoniwa/mmap"

ACTION="install"
PREFIX="${HAKONIWA_HOME:-${DEFAULT_PREFIX}}"
VERSION="${HAKONIWA_VERSION:-latest}"
ARCHIVE_URL="${HAKONIWA_ARCHIVE_URL:-}"
SHA256="${HAKONIWA_SHA256:-}"
YES=0
SETUP_SHELL=0
DRY_RUN=0

usage() {
    cat <<'USAGE'
Usage:
  install-hakoniwa.bash [install] [options]
  install-hakoniwa.bash uninstall [options]
  install-hakoniwa.bash reinstall [options]
  install-hakoniwa.bash check [options]

Options:
  --prefix PATH       Install prefix. Default: /usr/local/hakoniwa
  --version VERSION   GitHub Release tag. Default: latest
  --url URL           Download archive URL. Overrides --version.
  --sha256 DIGEST     Expected SHA-256 digest for the archive.
  --setup-shell       Append source line to ~/.bashrc or ~/.zshrc.
  --yes, -y           Do not ask for confirmation.
  --dry-run           Print actions without changing files.
  --help, -h          Show this help.

Release archive convention:
  hakoniwa-core-pro-${version}-${os}-${arch}.tar.gz

Expected archive contents:
  bin/ lib/ include/ share/ and optionally etc/

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

detect_arch() {
    case "$(uname -m)" in
        x86_64|amd64) printf 'x86_64' ;;
        arm64|aarch64) printf 'arm64' ;;
        *) die "unsupported architecture: $(uname -m)" ;;
    esac
}

latest_version() {
    need_cmd curl
    curl -fsSL "https://api.github.com/repos/${REPO_OWNER}/${REPO_NAME}/releases/latest" |
        sed -n 's/.*"tag_name"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' |
        head -n 1
}

archive_url() {
    if [ -n "${ARCHIVE_URL}" ]; then
        printf '%s' "${ARCHIVE_URL}"
        return 0
    fi

    resolved_version="${VERSION}"
    if [ "${resolved_version}" = "latest" ]; then
        resolved_version="$(latest_version)"
        [ -n "${resolved_version}" ] || die "failed to resolve latest release version"
    fi

    os_name="$(detect_os)"
    arch_name="$(detect_arch)"
    artifact="hakoniwa-core-pro-${resolved_version}-${os_name}-${arch_name}.tar.gz"
    printf 'https://github.com/%s/%s/releases/download/%s/%s' "${REPO_OWNER}" "${REPO_NAME}" "${resolved_version}" "${artifact}"
}

make_temp_dir() {
    mktemp -d 2>/dev/null || mktemp -d -t hakoniwa-install
}

verify_archive() {
    archive_path="$1"
    [ -n "${SHA256}" ] || return 0

    if command -v sha256sum >/dev/null 2>&1; then
        actual="$(sha256sum "${archive_path}" | awk '{print $1}')"
    elif command -v shasum >/dev/null 2>&1; then
        actual="$(shasum -a 256 "${archive_path}" | awk '{print $1}')"
    else
        die "SHA-256 verification requested, but sha256sum/shasum was not found"
    fi

    if [ "${actual}" != "${SHA256}" ]; then
        die "SHA-256 mismatch: expected ${SHA256}, got ${actual}"
    fi
    log "SHA-256 verified."
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

copy_payload() {
    src="$1"

    for dir in bin lib include share; do
        if [ -d "${src}/${dir}" ]; then
            as_root mkdir -p "${PREFIX}/${dir}"
            as_root cp -R "${src}/${dir}/." "${PREFIX}/${dir}/"
        fi
    done

    if [ -d "${src}/etc/hakoniwa" ]; then
        as_root mkdir -p "${CONFIG_DIR}"
        as_root cp -R "${src}/etc/hakoniwa/." "${CONFIG_DIR}/"
    elif [ -d "${src}/etc" ]; then
        as_root mkdir -p "${CONFIG_DIR}"
        as_root cp -R "${src}/etc/." "${CONFIG_DIR}/"
    fi
}

install_action() {
    need_cmd curl
    need_cmd tar
    need_cmd sed

    url="$(archive_url)"
    tmp_dir="$(make_temp_dir)"
    archive_path="${tmp_dir}/hakoniwa.tar.gz"
    extract_dir="${tmp_dir}/extract"

    log "Installing hakoniwa-core-pro"
    log "  prefix : ${PREFIX}"
    log "  archive: ${url}"
    if [ -n "${SHA256}" ]; then
        log "  sha256 : ${SHA256}"
    fi
    confirm "Continue installation?"

    if [ "${DRY_RUN}" -eq 1 ]; then
        log "[dry-run] would download archive"
        log "[dry-run] would install bin/lib/include/share into ${PREFIX}"
        log "[dry-run] would install config files into ${CONFIG_DIR} if present"
        log "[dry-run] would create ${PREFIX}/env.bash"
        log "[dry-run] would create ${MMAP_DIR}"
        return 0
    fi

    run mkdir -p "${extract_dir}"
    run curl -fL "${url}" -o "${archive_path}"
    verify_archive "${archive_path}"
    run tar -xzf "${archive_path}" -C "${extract_dir}"

    payload_dir="${extract_dir}"
    first_entry="$(find "${extract_dir}" -mindepth 1 -maxdepth 1 | head -n 1)"
    entry_count="$(find "${extract_dir}" -mindepth 1 -maxdepth 1 | wc -l | tr -d ' ')"
    if [ "${entry_count}" = "1" ] && [ -d "${first_entry}" ]; then
        payload_dir="${first_entry}"
    fi

    [ -d "${payload_dir}/bin" ] || [ -d "${payload_dir}/lib" ] || die "archive does not look like a hakoniwa install payload"

    as_root mkdir -p "${PREFIX}"
    copy_payload "${payload_dir}"
    write_env_file
    as_root mkdir -p "${MMAP_DIR}"
    as_root chmod 755 /var/lib/hakoniwa
    as_root chmod 777 "${MMAP_DIR}"

    if [ "${SETUP_SHELL}" -eq 1 ]; then
        setup_shell
    fi

    run rm -rf "${tmp_dir}"
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
        --version)
            [ "$#" -ge 2 ] || die "--version requires a value"
            VERSION="$2"
            shift 2
            ;;
        --url)
            [ "$#" -ge 2 ] || die "--url requires a value"
            ARCHIVE_URL="$2"
            shift 2
            ;;
        --sha256)
            [ "$#" -ge 2 ] || die "--sha256 requires a value"
            SHA256="$2"
            shift 2
            ;;
        --setup-shell)
            SETUP_SHELL=1
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
