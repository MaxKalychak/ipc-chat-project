#include <iostream>
#include <windows.h>
#include "C:\Users\Max\source\repos\ipc-chat-project\Common\common.h"

int main()
{
    std::wcout << L"[MQ Client] Запуск клієнта message queue...\n";

    // 1. Відкриваємо file mapping для черги
    HANDLE hMap = OpenFileMapping(
        FILE_MAP_ALL_ACCESS,
        FALSE,
        MQ_FILE_MAPPING_NAME
    );

    if (!hMap) {
        std::wcerr << L"[MQ Client] Не вдалося відкрити FileMapping\n";
        return 1;
    }

    MQQueue* queue = (MQQueue*)MapViewOfFile(
        hMap,
        FILE_MAP_ALL_ACCESS,
        0, 0,
        sizeof(MQQueue)
    );

    if (!queue) {
        std::wcerr << L"[MQ Client] Не вдалося змінити FileView\n";
        return 1;
    }

    // 2. Відкриваємо м’ютекс
    HANDLE hMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, MQ_MUTEX_NAME);

    // 3. Відкриваємо семафор (показує наявність нових повідомлень)
    HANDLE hSem = OpenSemaphore(SEMAPHORE_MODIFY_STATE, FALSE, MQ_SEMAPHORE_NAME);

    if (!hMutex || !hSem) {
        std::wcerr << L"[MQ Client] IPC синхронізація не відкрита\n";
        return 1;
    }

    std::wstring text;
    while (true)
    {
        std::wcout << L"Введіть повідомлення (exit - вихід): ";
        std::getline(std::wcin, text);

        if (text == L"exit") break;

        if (text.size() >= 256) {
            std::wcout << L"Повідомлення надто довге!\n";
            continue;
        }

        // Блокуємо чергу
        WaitForSingleObject(hMutex, INFINITE);

        // Перевіряємо чи є місце
        if (queue->count == MQ_QUEUE_SIZE) {
            std::wcout << L"[MQ Client] Черга переповнена!\n";
            ReleaseMutex(hMutex);
            continue;
        }

        // Записуємо повідомлення
        MQMessage& msg = queue->messages[queue->tail];
        wcsncpy_s(msg.text, text.c_str(), 255);

        queue->tail = (queue->tail + 1) % MQ_QUEUE_SIZE;
        queue->count++;

        std::wcout << L"[MQ Client] Повідомлення додане в чергу\n";

        ReleaseMutex(hMutex);

        // Сигналізуємо logger, що повідомлення є
        ReleaseSemaphore(hSem, 1, NULL);
    }

    UnmapViewOfFile(queue);
    CloseHandle(hMap);
    CloseHandle(hMutex);
    CloseHandle(hSem);

    return 0;
}
