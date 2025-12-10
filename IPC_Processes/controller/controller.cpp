#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include "../../Common/common.h"

void LaunchProcess(const std::wstring& path, const std::wstring& args, const std::wstring& title)
{
    std::wstring cmd = path + L" " + args;

    std::vector<wchar_t> buffer(cmd.begin(), cmd.end());
    buffer.push_back(L'\0');

    STARTUPINFOW si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);

    BOOL ok = CreateProcessW(
        nullptr,
        buffer.data(),
        nullptr, nullptr,
        FALSE,
        CREATE_NEW_CONSOLE,   // запуск у новій консолі
        nullptr, nullptr,
        &si, &pi
    );

    if (!ok)
    {
        std::wcout << L"[CONTROLLER] ERROR: Failed to launch " << title
            << L" | Error = " << GetLastError() << L"\n";
        return;
    }

    std::wcout << L"[CONTROLLER] Launched: " << title << L"\n";

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
}

int main()
{
    std::wcout << L"=== IPC CONTROLLER ===\n";
    std::wcout << L"Initializing Message Queue...\n";

    // ====================================================
    // 1. Create MQ (FileMapping + Mutex + Semaphores)
    // ====================================================

    HANDLE hMap = CreateFileMappingW(
        INVALID_HANDLE_VALUE,
        NULL,
        PAGE_READWRITE,
        0,
        sizeof(MQQueue),
        MQ_FILE_MAPPING_NAME
    );

    if (!hMap)
    {
        std::wcerr << L"[CONTROLLER] ERROR: CreateFileMapping failed. Code = " << GetLastError() << L"\n";
        return 1;
    }
    std::wcout << L"[CONTROLLER] FileMapping OK\n";

    MQQueue* queue = (MQQueue*)MapViewOfFile(
        hMap,
        FILE_MAP_ALL_ACCESS,
        0, 0,
        sizeof(MQQueue)
    );

    if (!queue)
    {
        std::wcerr << L"[CONTROLLER] ERROR: MapViewOfFile failed. Code = " << GetLastError() << L"\n";
        return 1;
    }

    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;

    std::wcout << L"[CONTROLLER] Queue initialized\n";

    HANDLE hMutex = CreateMutexW(NULL, FALSE, MQ_MUTEX_NAME);
    if (!hMutex)
    {
        std::wcerr << L"[CONTROLLER] ERROR: CreateMutex failed. Code = " << GetLastError() << L"\n";
        return 1;
    }
    std::wcout << L"[CONTROLLER] Mutex OK\n";

    HANDLE hSem = CreateSemaphoreW(NULL, 0, MQ_QUEUE_SIZE, MQ_SEMAPHORE_NAME);
    if (!hSem)
    {
        std::wcerr << L"[CONTROLLER] ERROR: CreateSemaphore failed. Code = " << GetLastError() << L"\n";
        return 1;
    }
    std::wcout << L"[CONTROLLER] Semaphore OK\n";


    // ====================================================
    // 2. Launch logger + mqueue client + pipe client
    // ====================================================

    std::wcout << L"\nLaunching modules (pipe + mqueue + logger)...\n";

    // Відносні шляхи (відносно controller.exe)
    std::wstring loggerPath = L"..\\..\\logger_shm\\Debug\\logger_shm.exe";
    std::wstring mqueuePath = L"..\\..\\client_mqueue\\Debug\\client_mqueue.exe";
    std::wstring pipePath = L"..\\..\\client_pipe\\Debug\\client_pipe.exe";

    LaunchProcess(loggerPath, L"", L"logger_shm.exe");
    LaunchProcess(mqueuePath, L"", L"client_mqueue.exe");
    LaunchProcess(pipePath, L"", L"client_pipe.exe");


    // ====================================================
    // 3. Controller "lives" until user presses ENTER
    // ====================================================
    std::wcout << L"\nAll modules launched. Controller is now waiting...\n";
    std::wcout << L"Press ENTER to exit controller.\n";

    std::wcin.get();
    std::wcin.get(); // щоб коректно пропустити символ

    UnmapViewOfFile(queue);
    CloseHandle(hMap);
    CloseHandle(hMutex);
    CloseHandle(hSem);

    std::wcout << L"Controller finished.\n";
    return 0;
}
