#pragma once
#include <Windows.h>
#include <Psapi.h>
#include <vector>

#include "tlhelp32.h"

class GameUtil
{
public:
	static uintptr_t findAddressFromPointer(HANDLE proc, uintptr_t provided_ptr, std::vector<uint32_t> provided_offsets);
	static uint32_t	 getGameProcessId();
	static uintptr_t getGameModule(uint32_t processId);
	static uintptr_t scanSignature(uintptr_t moduleBase, const char* signature);
	static void patchBytes(HANDLE proc, void* destination, void* source, size_t size);
	static void nopBytes(HANDLE proc, void* destination, size_t size);
	static void readMemory(HANDLE proc, void* address, void* buffer, size_t size);
};
