#!/bin/sh
# macOS per-user installer for vim-cmd
# Usage: install_macos.sh <DESTDIR> <BINDIR>

set -eu

DESTDIR="${1:-}"
BINDIR_DEFAULT="${2:-/usr/local/bin}"

# Determine the invoking user's home even under sudo
if [ -n "${SUDO_USER-}" ]; then
  USER_HOME=$(eval echo "~$SUDO_USER")
  USER_NAME="$SUDO_USER"
else
  USER_HOME="${HOME}"
  USER_NAME="$(id -un)"
fi

# Choose target bindir:
#  - If no overrides and default was /usr/local/bin, install to ~/bin
#  - Otherwise honor DESTDIR/BINDIR as requested
if [ -z "${DESTDIR}" ] && [ "${BINDIR_DEFAULT}" = "/usr/local/bin" ]; then
  TARGET_BINDIR="${USER_HOME}/bin"
else
  TARGET_BINDIR="${DESTDIR}${BINDIR_DEFAULT}"
fi

# Create and install
mkdir -p "${TARGET_BINDIR}"
install -m 0755 vim-cmd "${TARGET_BINDIR}/vim-cmd"

# If we installed into the user's home via sudo, fix ownership
case "${TARGET_BINDIR}" in
  "${USER_HOME}"/*)
    if [ -n "${SUDO_USER-}" ] && [ -z "${DESTDIR}" ]; then
      CHGRP="$(id -gn "$USER_NAME" 2>/dev/null || echo staff)"
      chown "${USER_NAME}:${CHGRP}" "${TARGET_BINDIR}/vim-cmd" 2>/dev/null || true
    fi
    ;;
esac

# Add an idempotent PATH guard to ~/.zprofile (only for per-user installs)
case "${TARGET_BINDIR}" in
  "${USER_HOME}"/*)
    ZP="${USER_HOME}/.zprofile"
    MARK="# Added by vim-cmd installer (PATH guard for ${TARGET_BINDIR})"
    [ -f "${ZP}" ] || : > "${ZP}"
    if ! grep -qF "${MARK}" "${ZP}"; then
      {
        echo
        echo "${MARK}"
        echo 'case ":$PATH:" in'
        echo "  *\":${TARGET_BINDIR}:\"*) ;;"
        echo "  *) PATH=\"${TARGET_BINDIR}:\$PATH\" ;;"
        echo 'esac'
        echo 'export PATH'
      } >> "${ZP}"
      echo "Updated ${ZP} (PATH guard for ${TARGET_BINDIR})"
    else
      echo "PATH guard already present in ${ZP}"
    fi
    ;;
esac

echo "Installed to: ${TARGET_BINDIR}/vim-cmd"
