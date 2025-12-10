#include <windows.h>
#include <string>
#include <iostream>
#include <vector>

void PrintMenu()
{
    std::cout << "=== IPC CONTROLLER ===\n";
    std::cout << "Виберіть метод IPC:\n";
    std::cout << "1 — Pipes/FIFO\n";
    std::cout << "2 — Message Queue\n";
    std::cout << "3 — Shared Memory + Semaphores\n";
    std::cout << "Ваш вибір: ";
}

int main()
{
    int method = 0;
    PrintMenu();
    std::cin >> method;

    if (method < 1 || method > 3)
    {
        std::cout << "Невірний вибір.\n";
        return 1;
    }

    // Формуємо аргумент
    std::wstring arg = L" ";
    arg += std::to_wstring(method);

    // ПОВНІ ШЛЯХИ ДО EXE
    std::wstring modules[3] =
    {
        L"C:\\Users\\Admin\\Desktop\\ipc-chat-project\\IPC_Processes\\client_pipe\\Debug\\client_pipe.exe",
        L"C:\\Users\\Admin\\Desktop\\ipc-chat-project\\IPC_Processes\\client_mqueue\\Debug\\client_mqueue.exe",
        L"C:\\Users\\Admin\\Desktop\\ipc-chat-project\\IPC_Processes\\logger_shm\\Debug\\logger.exe"
    };

    PROCESS_INFORMATION pi[3];
    STARTUPINFOW si[3];

    ZeroMemory(pi, sizeof(pi));
    ZeroMemory(si, sizeof(si));

    for (int i = 0; i < 3; i++)
    {
        si[i].cb = sizeof(STARTUPINFOW);

        // Команда = повний шлях + параметр
        std::wstring cmd = modules[i] + arg;

        // Перетворюємо у LPWSTR
        std::vector<wchar_t> buffer(cmd.begin(), cmd.end());
        buffer.push_back(L'\0');

        BOOL ok = CreateProcessW(
            nullptr,
            buffer.data(),
            nullptr, nullptr,
            FALSE,
            0, nullptr, nullptr,
            &si[i], &pi[i]
        );

        if (!ok)
        {
            std::wcout << L"FAILED: " << modules[i] << L"\n";
            continue;
        }

        std::wcout << L"Launched: " << modules[i] << L"\n";
    }

    // Очікування завершення процесів
    for (int i = 0; i < 3; i++)
    {
        WaitForSingleObject(pi[i].hProcess, INFINITE);
        CloseHandle(pi[i].hProcess);
        CloseHandle(pi[i].hThread);
    }

    std::cout << "\nУсі процеси завершили роботу.\n";
    return 0;
}
