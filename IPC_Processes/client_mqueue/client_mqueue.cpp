#include <iostream>
#include <windows.h>
#include <string>
#include "../../Common/common.h"

int main()
{
    std::wcout << L"[MQ Client] Launching the client message queue...\n";

    // 1. ³???????? file mapping ??? ?????
    HANDLE hMap = OpenFileMapping(
        FILE_MAP_ALL_ACCESS,
        FALSE,
        MQ_FILE_MAPPING_NAME
    );

    if (!hMap) {
        std::wcerr << L"[MQ Client] Can't open FileMapping\n";
        return 1;
    }

    MQQueue* queue = (MQQueue*)MapViewOfFile(
        hMap,
        FILE_MAP_ALL_ACCESS,
        0, 0,
        sizeof(MQQueue)
    );

    if (!queue) {
        std::wcerr << L"[MQ Client] Can't change FileView\n";
        return 1;
    }

    // 2. ³???????? ??????
    HANDLE hMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, MQ_MUTEX_NAME);

    // 3. ³???????? ??????? (?????? ????????? ????? ??????????)
    HANDLE hSem = OpenSemaphore(SEMAPHORE_MODIFY_STATE, FALSE, MQ_SEMAPHORE_NAME);

    if (!hMutex || !hSem) {
        std::wcerr << L"[MQ Client] IPC synchronization is not open\n";
        return 1;
    }

    std::wstring text;
    while (true)
    {
        std::wcout << L"Enter a message (exit - to end program): ";
        std::getline(std::wcin, text);

        if (text == L"exit") break;

        if (text.size() >= 256) {
            std::wcout << L"The message is too long!\n";
            continue;
        }

        // ??????? ?????
        WaitForSingleObject(hMutex, INFINITE);

        // ?????????? ?? ? ????
        if (queue->count == MQ_QUEUE_SIZE) {
            std::wcout << L"[MQ Client] The queue is full!\n";
            ReleaseMutex(hMutex);
            continue;
        }

        // ???????? ???????????
        MQMessage& msg = queue->messages[queue->tail];
        wcsncpy_s(msg.text, text.c_str(), 255);

        queue->tail = (queue->tail + 1) % MQ_QUEUE_SIZE;
        queue->count++;

        std::wcout << L"[MQ Client] The message is added to the queue\n";

        ReleaseMutex(hMutex);

        // ?????????? logger, ?? ??????????? ?
        ReleaseSemaphore(hSem, 1, NULL);
    }

    UnmapViewOfFile(queue);
    CloseHandle(hMap);
    CloseHandle(hMutex);
    CloseHandle(hSem);

    return 0;
}