#include "GameUtil.h"
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <vector>
#include <string>

uintptr_t GameUtil::findAddressFromPointer(HANDLE proc, uintptr_t provided_ptr, std::vector<uint32_t> provided_offsets)
{
    uintptr_t address = provided_ptr;

    for (uint32_t i = 0; i < provided_offsets.size(); ++i) {
        ReadProcessMemory(proc, (BYTE*)address, &address, sizeof(address), 0);
        address += provided_offsets[i];
    }


    return address;
}

uint32_t GameUtil::getGameProcessId()
{
    uint32_t processID = 0;
    HANDLE snapshotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (snapshotHandle != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 processEntry;
        processEntry.dwSize = sizeof(processEntry);
        if (Process32First(snapshotHandle, &processEntry)) {
            do {
                if (!_wcsicmp(processEntry.szExeFile, (const wchar_t*)L"Minecraft.Windows.exe")) {
                    processID = processEntry.th32ProcessID;

                    break;
                }
            } while (Process32Next(snapshotHandle, &processEntry));

        }
    }

    CloseHandle(snapshotHandle);
    return processID;
}

uintptr_t GameUtil::getGameModule(uint32_t processID)
{
    uintptr_t gameModuleAddress = 0;
    const wchar_t* gameName = L"Minecraft.Windows.exe";
    HANDLE snapshotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processID);

    if (snapshotHandle != INVALID_HANDLE_VALUE) {
        MODULEENTRY32 modEntry;
        modEntry.dwSize = sizeof(modEntry);


        if (Module32First(snapshotHandle, &modEntry)) {

            do {

                if (!_wcsicmp(modEntry.szModule, gameName)) {
                    gameModuleAddress = (uintptr_t)modEntry.modBaseAddr;
                    break;
                }
            } while (Module32Next(snapshotHandle, &modEntry));

        }
    }
    CloseHandle(snapshotHandle);
    return gameModuleAddress;
}

std::pair<std::vector<uint8_t>, std::string> sigToBytes(const char* signature) {
    std::vector<uint8_t> bytes;
    std::string mask;
    for (const char* p = signature; *p; ++p) {
        if (*p == ' ') continue;
        if (*p == '?') {
            bytes.push_back(0);
            mask += '?';
            if (*(p + 1) == '?') p++;
        } else {
            char* endPtr;
            bytes.push_back(static_cast<uint8_t>(std::strtol(p, &endPtr, 16)));
            mask += 'x';
            p = endPtr - 1;
        }
    }
    return std::make_pair(bytes, mask);
}

uintptr_t GameUtil::scanSignature(uintptr_t moduleBase, const char* signature) {
    auto result = sigToBytes(signature);
    std::vector<uint8_t> patternBytes = result.first;
    std::string mask = result.second;
    size_t patternSize = patternBytes.size();

    const size_t scanSize = 200 * 1024 * 1024;
    char* buffer = new char[scanSize];

    ReadProcessMemory(OpenProcess(PROCESS_VM_READ, FALSE, getGameProcessId()), (LPCVOID)moduleBase, buffer, scanSize, nullptr);

    for (uintptr_t i = 0; i < scanSize - patternSize; i++) {
        bool found = true;
        for (size_t j = 0; j < patternSize; j++) {
            if (mask[j] != '?' && patternBytes[j] != (uint8_t)buffer[i + j]) {
                found = false;
                break;
            }
        }
        if (found) {
            delete[] buffer;
            return moduleBase + i;
        }
    }
    delete[] buffer;
    return 0;
}

void GameUtil::patchBytes(HANDLE proc, void* destination, void* source, size_t size) {
    DWORD oldProtect;
    VirtualProtectEx(proc, destination, size, PAGE_EXECUTE_READWRITE, &oldProtect);
    WriteProcessMemory(proc, destination, source, size, nullptr);
    VirtualProtectEx(proc, destination, size, oldProtect, &oldProtect);
}

void GameUtil::nopBytes(HANDLE proc, void* destination, size_t size) {
    uint8_t* nopArray = new uint8_t[size];
    std::memset(nopArray, 0x90, size);

    DWORD oldProtect;
    VirtualProtectEx(proc, destination, size, PAGE_EXECUTE_READWRITE, &oldProtect);
    WriteProcessMemory(proc, destination, nopArray, size, nullptr);
    VirtualProtectEx(proc, destination, size, oldProtect, &oldProtect);

    delete[] nopArray;
}

void GameUtil::readMemory(HANDLE proc, void* address, void* buffer, size_t size) {
    ReadProcessMemory(proc, address, buffer, size, nullptr);
}