#include <windows.h>
#include <iostream>
#include "../../Common/common.h"

int main()
{
    std::wcout << L"[LOGGER] Launch of logger...\n";

    // ================================
    // 1. ³�������� FileMapping �����
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

    // ³�������� Mutex �����
    HANDLE hQueueMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, MQ_MUTEX_NAME);

    // ³�������� Semaphore ����� (������ �� �볺���)
    HANDLE hQueueSem = OpenSemaphore(SEMAPHORE_MODIFY_STATE | SYNCHRONIZE, FALSE, MQ_SEMAPHORE_NAME);

    if (!hQueueMutex || !hQueueSem)
    {
        std::wcerr << L"[LOGGER] ERROR: Can't open Mutex or Semaphore for a queue\n";
        return 1;
    }

    // ===========================================
    // 2. ��������� SharedMemory ��� WinForms UI
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

    // ��������� ������������� UI
    HANDLE hShmMutex = CreateMutex(NULL, FALSE, SHM_MUTEX_NAME);
    HANDLE hShmSem = CreateSemaphore(NULL, 0, 1000, SHM_SEMAPHORE_NAME);

    int shmWriteIndex = 0; // ���� ������ �������� �����������

    std::wcout << L"[LOGGER] is ready for messages...\n";

    // ==========================
    //   �������� ���� ������
    // ==========================
    while (true)
    {
        // ������ ���� �볺�� ������� ����������� � �����
        WaitForSingleObject(hQueueSem, INFINITE);

        // ������� ������ �� �����
        WaitForSingleObject(hQueueMutex, INFINITE);

        if (queue->count > 0)
        {
            MQMessage& slot = queue->messages[queue->head];

            // ��������� ChatMessage ��� shared memory
            ChatMessage msg;
            msg.senderId = CLIENT_MQUEUE_ID;
            wcsncpy_s(msg.text, slot.text, MAX_TEXT);

            // ��������� head
            queue->head = (queue->head + 1) % MQ_QUEUE_SIZE;
            queue->count--;

            ReleaseMutex(hQueueMutex);

            // ������ � SHM
            WaitForSingleObject(hShmMutex, INFINITE);

            shmMessages[shmWriteIndex] = msg;
            shmWriteIndex = (shmWriteIndex + 1) % 10;

            ReleaseMutex(hShmMutex);

            // ������ WinForms, �� ���� ����������� ������
            ReleaseSemaphore(hShmSem, 1, NULL);

            // ���� � ������� ������
            std::wcout << L"[LOGGER] Message delivered: " << msg.text << L"\n";

        }
        else
        {
            ReleaseMutex(hQueueMutex);
        }
    }

    // (������ �� ����������, ��� �� ���������)
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