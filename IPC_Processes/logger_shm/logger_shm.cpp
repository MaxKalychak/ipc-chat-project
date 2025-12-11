#include <windows.h>
#include <iostream>
#include "../../Common/common.h"

// ======================= PIPE THREAD =======================

DWORD WINAPI PipeReaderThread(LPVOID param)
{
    HANDLE hPipe = (HANDLE)param;

    std::wcout << L"[LOGGER PIPE] Waiting for PIPE client...\n";

    if (!ConnectNamedPipe(hPipe, NULL))
    {
        if (GetLastError() != ERROR_PIPE_CONNECTED)
        {
            std::wcerr << L"[LOGGER PIPE] ConnectNamedPipe failed. Code = "
                << GetLastError() << L"\n";
            return 1;
        }
    }

    std::wcout << L"[LOGGER PIPE] Client connected.\n";

    ChatMessage msg;

    while (true)
    {
        DWORD bytesRead = 0;
        BOOL ok = ReadFile(hPipe, &msg, sizeof(msg), &bytesRead, NULL);

        if (!ok || bytesRead == 0)
        {
            std::wcout << L"[LOGGER PIPE] Client disconnected.\n";
            break;
        }

        // -------------------------------
        // Write to shared memory
        // -------------------------------
        HANDLE hShm = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, SHM_NAME);
        if (!hShm) continue;

        ChatMessage* shm = (ChatMessage*)MapViewOfFile(
            hShm, FILE_MAP_ALL_ACCESS, 0, 0, SHM_SIZE
        );
        if (!shm) continue;

        HANDLE hMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, SHM_MUTEX_NAME);
        HANDLE hSem = OpenSemaphore(SEMAPHORE_MODIFY_STATE, FALSE, SHM_SEMAPHORE_NAME);

        static int idx = 0;

        WaitForSingleObject(hMutex, INFINITE);

        shm[idx] = msg;
        idx = (idx + 1) % 10;

        ReleaseMutex(hMutex);
        ReleaseSemaphore(hSem, 1, NULL);

        UnmapViewOfFile(shm);

        std::wcout << L"[LOGGER PIPE] Received (PIPE): " << msg.text << L"\n";
    }

    return 0;
}

// ============================= MAIN LOGGER =============================

int main()
{
    std::wcout << L"[LOGGER] Launch of logger...\n";

    // ---------------- MQ ----------------
    HANDLE hMap = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, MQ_FILE_MAPPING_NAME);
    if (!hMap) { std::wcerr << L"[LOGGER] ERROR MQ FileMapping\n"; return 1; }

    MQQueue* queue = (MQQueue*)MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(MQQueue));
    if (!queue) { std::wcerr << L"[LOGGER] ERROR MQ MapViewOfFile\n"; return 1; }

    HANDLE hQueueMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, MQ_MUTEX_NAME);
    HANDLE hQueueSem = OpenSemaphore(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, FALSE, MQ_SEMAPHORE_NAME);

    // ---------------- SHM ----------------
    HANDLE hShm = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, SHM_SIZE, SHM_NAME);
    if (!hShm) { std::wcerr << L"[LOGGER] ERROR CreateFileMapping(SHM)\n"; return 1; }

    ChatMessage* shmMessages = (ChatMessage*)MapViewOfFile(hShm, FILE_MAP_ALL_ACCESS, 0, 0, SHM_SIZE);
    if (!shmMessages) { std::wcerr << L"[LOGGER] ERROR MapViewOfFile(SHM)\n"; return 1; }

    HANDLE hShmMutex = CreateMutex(NULL, FALSE, SHM_MUTEX_NAME);
    HANDLE hShmSem = CreateSemaphore(NULL, 0, 1000, SHM_SEMAPHORE_NAME);

    int shmWriteIndex = 0;

    // ---------------- PIPE ----------------
    std::wcout << L"[LOGGER] Creating PIPE: " << PIPE_CLIENT_TO_LOGGER << L"\n";

    HANDLE hPipe = CreateNamedPipeW(
        PIPE_CLIENT_TO_LOGGER,
        PIPE_ACCESS_INBOUND,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1,
        sizeof(ChatMessage),
        sizeof(ChatMessage),
        0,
        NULL
    );

    if (hPipe == INVALID_HANDLE_VALUE)
    {
        std::wcerr << L"[LOGGER] ERROR CreateNamedPipe. Code = " << GetLastError() << L"\n";
        return 1;
    }

    CreateThread(NULL, 0, PipeReaderThread, hPipe, 0, NULL);

    std::wcout << L"[LOGGER] Ready for MQ + PIPE messages...\n";

    // ======================= MAIN LOOP (MQ) =======================
    while (true)
    {
        WaitForSingleObject(hQueueSem, INFINITE);
        WaitForSingleObject(hQueueMutex, INFINITE);

        if (queue->count > 0)
        {
            MQMessage& slot = queue->messages[queue->head];

            ChatMessage msg;
            msg.senderId = CLIENT_MQUEUE_ID;
            wcsncpy_s(msg.text, slot.text, MAX_TEXT);

            queue->head = (queue->head + 1) % MQ_QUEUE_SIZE;
            queue->count--;

            ReleaseMutex(hQueueMutex);

            WaitForSingleObject(hShmMutex, INFINITE);

            shmMessages[shmWriteIndex] = msg;
            shmWriteIndex = (shmWriteIndex + 1) % 10;

            ReleaseMutex(hShmMutex);
            ReleaseSemaphore(hShmSem, 1, NULL);

            std::wcout << L"[LOGGER MQ] Received: " << msg.text << L"\n";
        }
        else
        {
            ReleaseMutex(hQueueMutex);
        }
    }
    UnmapViewOfFile(queue);
    UnmapViewOfFile(shmMessages);

CloseHandlе(hMap);
    CloseHandlе(hQueueMutex);
    CloseHandlе(hQueueSem);
    CloseHandlе(hShm);
    CloseHandlе(hShmMutex);
 CloseHandle(hShmSem);

    return 0;
}

