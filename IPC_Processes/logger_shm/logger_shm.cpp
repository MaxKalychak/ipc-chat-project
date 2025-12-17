#include <windows.h>
#include <iostream>
#include <string>
#include "../../Common/common.h"

// Define a special ID for system logs
const int LOGGER_ID = 99;

// ======================= GLOBALS (For easier logging) =======================
HANDLE hGlobalShmMap = NULL;
ChatMessage* pGlobalShm = NULL;
HANDLE hGlobalMutex = NULL;
HANDLE hGlobalSem = NULL;
int globalWriteIdx = 0;

// ======================= HELPER: LOG TO CONSOLE & UI =======================
void LogToBoth(const std::wstring& text)
{
    // 1. Print to Console (Standard Logger)
    std::wcout << text << L"\n";

    // 2. Send to UI (via Shared Memory)
    if (pGlobalShm && hGlobalMutex && hGlobalSem)
    {
        WaitForSingleObject(hGlobalMutex, INFINITE);

        // Write to the circular buffer
        ChatMessage msg;
        msg.senderId = LOGGER_ID; // Special ID for the Logger
        wcsncpy_s(msg.text, text.c_str(), MAX_TEXT);

        pGlobalShm[globalWriteIdx] = msg;
        globalWriteIdx = (globalWriteIdx + 1) % 10; // Wrap around (Circular Buffer)

        ReleaseMutex(hGlobalMutex);
        ReleaseSemaphore(hGlobalSem, 1, NULL); // Signal UI to read
    }
}

// ======================= PIPE THREAD =======================
DWORD WINAPI PipeReaderThread(LPVOID param)
{
    HANDLE hPipe = (HANDLE)param;
    ChatMessage msg;

    LogToBoth(L"[PIPE] New client connected.");

    while (true)
    {
        DWORD bytesRead = 0;
        BOOL ok = ReadFile(hPipe, &msg, sizeof(msg), &bytesRead, NULL);
        if (!ok || bytesRead == 0) break;

        // Log the raw message to Console/UI
        std::wstring output = L"[PIPE Client]: " + std::wstring(msg.text);
        LogToBoth(output);
    }

    LogToBoth(L"[PIPE] Client disconnected.");
    DisconnectNamedPipe(hPipe);
    CloseHandle(hPipe);
    return 0;
}

// ======================= PIPE LISTENER =======================
DWORD WINAPI PipeListenerThread(LPVOID)
{
    LogToBoth(L"[SYSTEM] Pipe Server Listener Started.");
    while (true)
    {
        HANDLE hPipe = CreateNamedPipeW(
            PIPE_CLIENT_TO_LOGGER, PIPE_ACCESS_INBOUND,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES, sizeof(ChatMessage), sizeof(ChatMessage), 0, NULL
        );

        if (hPipe != INVALID_HANDLE_VALUE) {
            if (ConnectNamedPipe(hPipe, NULL) || GetLastError() == ERROR_PIPE_CONNECTED) {
                CreateThread(NULL, 0, PipeReaderThread, hPipe, 0, NULL);
            }
            else {
                CloseHandle(hPipe);
            }
        }
        else {
            Sleep(1000); // Prevent CPU spike on error
        }
    }
    return 0;
}

// ============================= MAIN =============================
int main()
{
    // ---------------- INIT SHM (FOR UI) ----------------
    // We create this FIRST so we can log startup errors to the UI
    hGlobalShmMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, SHM_SIZE, SHM_NAME);
    pGlobalShm = (ChatMessage*)MapViewOfFile(hGlobalShmMap, FILE_MAP_ALL_ACCESS, 0, 0, SHM_SIZE);
    hGlobalMutex = CreateMutex(NULL, FALSE, SHM_MUTEX_NAME);
    hGlobalSem = CreateSemaphore(NULL, 0, 1000, SHM_SEMAPHORE_NAME);

    // Now we can use the helper!
    LogToBoth(L"[SYSTEM] Logger Launching...");

    // ---------------- INIT MQ (FROM UI) ----------------
    HANDLE hMap = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, MQ_FILE_MAPPING_NAME);
    int retries = 0;
    while (!hMap && retries < 5) {
        LogToBoth(L"[SYSTEM] Waiting for MQ Map...");
        Sleep(1000);
        hMap = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, MQ_FILE_MAPPING_NAME);
        retries++;
    }

    if (!hMap) {
        LogToBoth(L"[ERROR] MQ Map not found. Is UI running?");
        system("pause");
        return 1;
    }

    MQQueue* queue = (MQQueue*)MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(MQQueue));
    HANDLE hQueueMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, MQ_MUTEX_NAME);
    HANDLE hQueueSem = OpenSemaphore(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, FALSE, MQ_SEMAPHORE_NAME);

    // ---------------- START THREADS ----------------
    CreateThread(NULL, 0, PipeListenerThread, NULL, 0, NULL);

    LogToBoth(L"[SYSTEM] Ready. Logging active.");

    // ---------------- MAIN MQ LOOP ----------------
    while (true)
    {
        WaitForSingleObject(hQueueSem, INFINITE);
        WaitForSingleObject(hQueueMutex, INFINITE);

        if (queue && queue->count > 0)
        {
            MQMessage& slot = queue->messages[queue->head];

            // Format and Log
            std::wstring txt = slot.text;
            std::wstring output = L"[MQ Client]: " + txt;

            // This helper writes to console AND sends to UI
            LogToBoth(output);

            queue->head = (queue->head + 1) % MQ_QUEUE_SIZE;
            queue->count--;
        }

        ReleaseMutex(hQueueMutex);
    }

    return 0;
}