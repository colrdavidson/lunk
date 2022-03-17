set -x -o pipefail

dd if=/dev/zero of=bin/efi.img bs=1k count=1440
mformat -i bin/efi.img -f 1440 ::
mmd -i bin/efi.img ::/EFI
mmd -i bin/efi.img ::/EFI/BOOT
mcopy -i bin/efi.img bin/BOOTX64.EFI ::/EFI/BOOT
mcopy -i bin/efi.img startup.nsh ::/
#mcopy -i bin/efi.img bin/kernel.o ::/
#mcopy -i bin/efi.img bin/loader.bin ::/

rm -rf bin/iso bin/cdimage.iso
mkdir bin/iso
cp bin/efi.img bin/iso
xorriso -as mkisofs -o bin/cdimage.iso -iso-level 3 -V UEFI bin/iso bin/efi.img -e efi.img -no-emul-boot
