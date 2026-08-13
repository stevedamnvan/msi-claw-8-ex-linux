#!/usr/bin/env bash
set -euo pipefail

package=claw-rt721-fix
version=0.1.0
source_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
dkms_dir=/usr/src/${package}-${version}
kernel_release=${KERNELRELEASE:-$(uname -r)}
architecture=$(uname -m)

(( EUID == 0 )) || {
	printf 'Run this installer as root.\n' >&2
	exit 1
}

[[ $(< /sys/class/dmi/id/product_name) == 'Claw 8 EX AI+ CG3EM' ]] || {
	printf 'Refusing unexpected product.\n' >&2
	exit 1
}
[[ $(< /sys/class/dmi/id/board_name) == 'MS-1T91' ]] || {
	printf 'Refusing unexpected board.\n' >&2
	exit 1
}

for command_name in clang dkms make systemctl; do
	command -v "$command_name" >/dev/null || {
		printf 'Missing required command: %s\n' "$command_name" >&2
		exit 1
	}
done

[[ -d /lib/modules/$kernel_release/build ]] || {
	printf 'Missing headers for %s.\n' "$kernel_release" >&2
	exit 1
}

source_files=(claw_rt721_amp.c Makefile dkms.conf README.md)
if [[ -e $dkms_dir ]]; then
	for source_name in "${source_files[@]}"; do
		cmp -s "$source_dir/$source_name" "$dkms_dir/$source_name" || {
			printf 'Refusing mismatched staged file: %s\n' "$source_name" >&2
			exit 1
		}
	done
else
	install -d -m 0755 "$dkms_dir"
	for source_name in "${source_files[@]}"; do
		install -m 0644 "$source_dir/$source_name" "$dkms_dir/"
	done
fi

if ! dkms status -m "$package" -v "$version" 2>/dev/null |
	grep -Fq "$package/$version"; then
	dkms add -m "$package" -v "$version"
fi

if ! dkms status -m "$package" -v "$version" 2>/dev/null |
	grep -Fq "$package/$version, $kernel_release, $architecture: installed"; then
	dkms build -m "$package" -v "$version" -k "$kernel_release"
	dkms install -m "$package" -v "$version" -k "$kernel_release"
fi

install -m 0644 "$source_dir/claw-rt721-fix.service" \
	/etc/systemd/system/claw-rt721-fix.service
install -m 0755 "$source_dir/claw-rt721-fix-sleep" \
	/usr/lib/systemd/system-sleep/claw-rt721-fix

systemctl daemon-reload
systemctl enable claw-rt721-fix.service
systemctl restart claw-rt721-fix.service

printf 'Installed %s/%s for %s.\n' "$package" "$version" "$kernel_release"
