#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "efi.h"

void println(EFI_SYSTEM_TABLE *st, uint16_t *str) {
	st->ConOut->OutputString(st->ConOut, (int16_t *)str);
	st->ConOut->OutputString(st->ConOut, (int16_t *)L"\n\r");
}

#define panic(x, y) do { println((x), (y)); return 1; } while (false);

#define LOADER_BUFFER_SIZE (1 * 1024 * 1024)
#define KERNEL_BUFFER_SIZE (1 * 1024 * 1024)

#define MEM_MAP_BUFFER_SIZE (16 * 1024)
char mem_map_buffer[MEM_MAP_BUFFER_SIZE];

typedef struct {
	uintptr_t base, pages;
} MemRegion;

#define MAX_MEM_REGIONS (1024)
MemRegion mem_regions[MAX_MEM_REGIONS];

void *memset(void *buffer, int c, size_t n) {
	uint8_t *buf = (uint8_t *)buffer;
	for (size_t i = 0; i < n; i++) {
		buf[i] = c;
	}
	return (void *)buf;
}
void *memcpy(void *dest, const void *src, size_t n) {
	uint8_t *d = (uint8_t *)dest;
	uint8_t *s = (uint8_t *)src;
	for (size_t i = 0; i < n; i++) {
		d[i] = s[i];
	}
	return (void *)dest;
}

EFI_STATUS efi_main(EFI_HANDLE img_handle, EFI_SYSTEM_TABLE *st) {
	EFI_STATUS status;

	status = st->ConOut->ClearScreen(st->ConOut);
	println(st, L"Beginning EFI Boot...");

	EFI_PHYSICAL_ADDRESS loader_addr = 0;
	// Load the kernel and loader from disk
	{
		status = st->BootServices->AllocatePages(AllocateAnyPages, EfiLoaderData, 0x200, &loader_addr);
		if (status != 0) {
			panic(st, L"Failed to allocate space for loader + kernel!");
		}

		EFI_GUID loaded_img_proto_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
		EFI_LOADED_IMAGE_PROTOCOL *loaded_img_proto;
		status = st->BootServices->OpenProtocol(img_handle, &loaded_img_proto_guid, (void **)&loaded_img_proto, img_handle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
		if (status != 0) {
			panic(st, L"Failed to load img protocol!");
		}

		EFI_GUID simple_fs_proto_guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
		EFI_HANDLE dev_handle = loaded_img_proto->DeviceHandle;
		EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *simple_fs_proto;
		status = st->BootServices->OpenProtocol(dev_handle, &simple_fs_proto_guid, (void **)&simple_fs_proto, img_handle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
		if (status != 0) {
			panic(st, L"Failed to load fs protocol!");
		}

		EFI_FILE *fs_root;
		status = simple_fs_proto->OpenVolume(simple_fs_proto, &fs_root);
		if (status != 0) {
			panic(st, L"Failed to open fs root!");
		}

		size_t size = LOADER_BUFFER_SIZE;
		EFI_FILE *loader_file;
		status = fs_root->Open(fs_root, &loader_file, (int16_t *)L"loader.bin", EFI_FILE_MODE_READ, 0);
		if (status != 0) {
			panic(st, L"Failed to open loader.bin!");
		}

		char *loader_buffer = (char *)loader_addr;
		loader_file->Read(loader_file, &size, loader_buffer);
		if (size == LOADER_BUFFER_SIZE) {
			panic(st, L"Loader too large to fit into buffer!");
		}

		EFI_FILE *kernel_file;
		status = fs_root->Open(fs_root, &kernel_file, (int16_t *)L"kernel.o", EFI_FILE_MODE_READ, 0);
		if (status != 0) {
			panic(st, L"Failed to open kernel.o!");
		}

		size = KERNEL_BUFFER_SIZE;
		char *kernel_buffer = (char *)loader_addr + LOADER_BUFFER_SIZE;
		kernel_file->Read(kernel_file, &size, kernel_buffer);
		if (size == KERNEL_BUFFER_SIZE) {
			panic(st, L"Kernel too large to fit into buffer!");
		}

		println(st, L"Loaded the loader and kernel!");
	}

	// This block kicks off the kernel, not fully tested!
	// Memory map loading must be followed immediately with ExitBootServices

	size_t map_key;
	// Load latest memory map
	{
		size_t desc_size, size = MEM_MAP_BUFFER_SIZE;
		uint32_t desc_version;
		status = st->BootServices->GetMemoryMap(&size, (EFI_MEMORY_DESCRIPTOR *)mem_map_buffer, &map_key, &desc_size, &desc_version);
		if (status != 0) {
			panic(st, L"Failed to get memory map!");
		}

		size_t desc_count = size / desc_size;
		size_t mem_region_count = 0;
		for (int i = 0; i < desc_count && mem_region_count < MAX_MEM_REGIONS; i++) {
			EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR *)(mem_map_buffer + (i * desc_size));

			if (desc->Type == EfiConventionalMemory && desc->PhysicalStart >= 0x300000) {
				mem_regions[mem_region_count].base = desc->PhysicalStart;
				mem_regions[mem_region_count].pages = desc->NumberOfPages;
				mem_region_count++;
			}
		}
		mem_regions[mem_region_count].base = 0;
	}

	status = st->BootServices->ExitBootServices(img_handle, map_key);
	if (status != 0) {
		panic(st, L"Failed to exit EFI");
	}

	println(st, L"Starting the loader");
	// Boot the loader
	((void (*)()) loader_addr)();

	return 0;
}
