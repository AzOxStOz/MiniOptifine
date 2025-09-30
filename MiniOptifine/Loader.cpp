#include <Windows.h>
#include <iostream>
#include <thread>
#include "Util/GameUtil.h"

bool g_isZoomToggled = false;
bool g_isReachToggled = false;

const char* REACH_SIGNATURE = "74 0A F3 0F 5D 35 ? ? ? ?";

void init() {
    float zoomVal = 11.0f;
    float defaultFov = 0;

    float reachVal = 6.0f;
    float defaultReach = 3.0f;
    byte originalBytes[2];

    uint32_t process_id = GameUtil::getGameProcessId();
    if (process_id == 0) {
        system("color 4");
        std::cout << "[ERROR] Minecraft process not found.\n\n";
        system("pause");
        return;
    }

    HANDLE process_handle = OpenProcess(PROCESS_ALL_ACCESS, NULL, process_id);
    uintptr_t gameId = GameUtil::getGameModule(process_id);

    if (gameId == 0 || process_handle == NULL) {
        system("color 4");
        std::cout << "[ERROR] Can't get game handler.\n\n";
        system("pause");
        return;
    }

    uintptr_t optifineZoombaseAddress = gameId + 0x095F2F18;
    uintptr_t __optifineZoomPtr = GameUtil::findAddressFromPointer(process_handle, optifineZoombaseAddress, { 0x10, 0x8, 0x1A0, 0x18 });

    std::cout << "[NOTICE] Checking if game window is present...\n\n";
    HWND gameWindow = FindWindow(NULL, L"Minecraft");
    if (!gameWindow) {
        system("color 4");
        std::cout << "[ERROR] Game not found, please launch Minecraft Bedrock Edition first.\n\n";
        system("pause");
        return;
    } else {
        system("color b");
        std::cout << "[NOTICE] Game found, to enable/disable Zoom press C, to enable/disable Reach press R.\n";
    }

    ReadProcessMemory(process_handle, (LPCVOID)__optifineZoomPtr, &defaultFov, sizeof(defaultFov), nullptr);

    uintptr_t reachInstructionAddress = GameUtil::scanSignature(gameId, REACH_SIGNATURE);
    uintptr_t reachValueAddress = 0;

    if (reachInstructionAddress == 0) {
        system("color 4");
        std::cout << "[ERROR] Reach Sig not found, please update me.\n";
    } else {
        std::cout << "[NOTICE] Reach Sig found.\n";
        GameUtil::readMemory(process_handle, (void*)reachInstructionAddress, originalBytes, 2);

        uintptr_t addrBytesCREATIVE = reachInstructionAddress + 2;
        int relativeOffset;
        GameUtil::readMemory(process_handle, (void*)(addrBytesCREATIVE + 5), &relativeOffset, sizeof(relativeOffset));
        reachValueAddress = addrBytesCREATIVE + 9 + relativeOffset;
    }

    while (true) {
        Sleep(100);
        if (GetAsyncKeyState('C') & 0x8000) {
            g_isZoomToggled = !g_isZoomToggled;

            if (g_isZoomToggled) {
                WriteProcessMemory(process_handle, (LPVOID)__optifineZoomPtr, &zoomVal, sizeof(zoomVal), nullptr);
            } else {
                WriteProcessMemory(process_handle, (LPVOID)__optifineZoomPtr, &defaultFov, sizeof(defaultFov), nullptr);
            }
            Sleep(200);
        }

        if (reachInstructionAddress != 0 && (GetAsyncKeyState('R') & 0x8000)) {
            g_isReachToggled = !g_isReachToggled;

            if (g_isReachToggled) {
                GameUtil::nopBytes(process_handle, (void*)reachInstructionAddress, 2);
                GameUtil::patchBytes(process_handle, (void*)reachValueAddress, &reachVal, sizeof(reachVal));
                std::cout << "[+] Reach Enable\n";
            } else {
                GameUtil::patchBytes(process_handle, (void*)reachInstructionAddress, originalBytes, 2);
                GameUtil::patchBytes(process_handle, (void*)reachValueAddress, &defaultReach, sizeof(defaultReach));
                std::cout << "[-] Reach Disable\n";
            }
            Sleep(200);
        }
    }
}

int main() {
    init();
    return 0;
}