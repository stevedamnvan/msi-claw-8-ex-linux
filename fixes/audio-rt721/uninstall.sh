#!/usr/bin/env bash
set -euo pipefail

package=claw-rt721-fix
version=0.1.0
dkms_dir=/usr/src/${package}-${version}

(( EUID == 0 )) || {
	printf 'Run this uninstaller as root.\n' >&2
	exit 1
}

systemctl disable --now claw-rt721-fix.service 2>/dev/null || true
rm -f -- /etc/systemd/system/claw-rt721-fix.service
rm -f -- /usr/lib/systemd/system-sleep/claw-rt721-fix

if dkms status -m "$package" -v "$version" >/dev/null 2>&1; then
	dkms remove -m "$package" -v "$version" --all
fi

if [[ $dkms_dir == /usr/src/claw-rt721-fix-0.1.0 ]]; then
	rm -rf -- "$dkms_dir"
fi

systemctl daemon-reload
printf 'Removed %s/%s. Hardware state will reset at reboot.\n' \
	"$package" "$version"
