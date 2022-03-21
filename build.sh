set -x

rm -rf bin
mkdir bin

# build efi stub
clang -I efi -target x86_64-pc-win32-coff -fno-stack-protector -nostdlib -fshort-wchar -mno-red-zone -c efi_stub.c -o bin/uefi.o
lld-link -subsystem:efi_application -nodefaultlib -dll -entry:efi_main bin/uefi.o -out:bin/BOOTX64.EFI

# build kernel
clang -o bin/kernel.o -fno-stack-protector -fpic -mno-red-zone -nostdlib -c kernel.c
nasm -f bin -o bin/loader.bin loader.s
