#include <windows.h>
#include <iostream>
#include "../../Common/common.h"

int main()
{
    std::wcout << L"[LOGGER] Launch of logger...\n";

    // ================================
    // 1. Відкриваємо FileMapping черги
    // ================================
    HANDLE hMap = OpenFileMapping(
        FILE_MAP_ALL_ACCESS,
        FALSE,
        MQ_FILE_MAPPING_NAME
    );

    if (!hMap)
    {
        std::wcerr << L"[LOGGER] ERROR: Can't open MessageQueue FileMapping\n";
        return 1;
    }

    MQQueue* queue = (MQQueue*)MapViewOfFile(
        hMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(MQQueue)
    );

    if (!queue)
    {
        std::wcerr << L"[LOGGER] ERROR: MapViewOfFile\n";
        return 1;
    }

    // Відкриваємо Mutex черги
    HANDLE hQueueMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, MQ_MUTEX_NAME);

    // Відкриваємо Semaphore черги (сигнал від клієнтів)
    HANDLE hQueueSem = OpenSemaphore(SEMAPHORE_MODIFY_STATE | SYNCHRONIZE, FALSE, MQ_SEMAPHORE_NAME);

    if (!hQueueMutex || !hQueueSem)
    {
        std::wcerr << L"[LOGGER] ERROR: Can't open Mutex or Semaphore for a queue\n";
        return 1;
    }

    // ===========================================
    // 2. Створюємо SharedMemory для WinForms UI
    // ===========================================
    HANDLE hShm = CreateFileMapping(
        INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
        0, SHM_SIZE, SHM_NAME
    );

    if (!hShm)
    {
        std::wcerr << L"[LOGGER] ERROR: CreateFileMapping(SHM)\n";
        return 1;
    }

    ChatMessage* shmMessages = (ChatMessage*)MapViewOfFile(
        hShm, FILE_MAP_ALL_ACCESS, 0, 0, SHM_SIZE
    );

    if (!shmMessages)
    {
        std::wcerr << L"[LOGGER] ERROR: MapViewOfFile(SHM)\n";
        return 1;
    }

    // Створюємо синхронізацію UI
    HANDLE hShmMutex = CreateMutex(NULL, FALSE, SHM_MUTEX_NAME);
    HANDLE hShmSem = CreateSemaphore(NULL, 0, 1000, SHM_SEMAPHORE_NAME);

    int shmWriteIndex = 0; // куди писати наступне повідомлення

    std::wcout << L"[LOGGER] is ready for messages...\n";

    // ==========================
    //   ОСНОВНИЙ ЦИКЛ ЛОГЕРА
    // ==========================
    while (true)
    {
        // Чекаємо поки клієнт додасть повідомлення у чергу
        WaitForSingleObject(hQueueSem, INFINITE);

        // Блокуємо доступ до черги
        WaitForSingleObject(hQueueMutex, INFINITE);

        if (queue->count > 0)
        {
            MQMessage& slot = queue->messages[queue->head];

            // Створюємо ChatMessage для shared memory
            ChatMessage msg;
            msg.senderId = CLIENT_MQUEUE_ID;
            wcsncpy_s(msg.text, slot.text, MAX_TEXT);

            // Переміщуємо head
            queue->head = (queue->head + 1) % MQ_QUEUE_SIZE;
            queue->count--;

            ReleaseMutex(hQueueMutex);

            // Пишемо у SHM
            WaitForSingleObject(hShmMutex, INFINITE);

            shmMessages[shmWriteIndex] = msg;
            shmWriteIndex = (shmWriteIndex + 1) % 10;

            ReleaseMutex(hShmMutex);

            // Сигнал WinForms, що нове повідомлення готове
            ReleaseSemaphore(hShmSem, 1, NULL);

            // Вивід у консоль логера
            std::wcout << L"[LOGGER] Message delivered: " << msg.text << L"\n";

        }
        else
        {
            ReleaseMutex(hQueueMutex);
        }
    }

    // (ніколи не виконується, але по стандарту)
    UnmapViewOfFile(queue);
    UnmapViewOfFile(shmMessages);

    CloseHandle(hMap);
    CloseHandle(hQueueMutex);
    CloseHandle(hQueueSem);
    CloseHandle(hShm);
    CloseHandle(hShmMutex);
    CloseHandle(hShmSem);

    return 0;
}
