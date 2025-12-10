// controller.cpp
// Учасник 1 — головний процес контролер IPC

#include <windows.h>
#include <string>
#include <iostream>

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

    // --- формуємо аргумент для процесів ---
    std::wstring arg = L" ";
    arg += std::to_wstring(method);

    // --- список дочірніх модулів ---
    const wchar_t* modules[3] =
    {
        L"client_pipe.exe",     // Учасник 2
        L"client_msg.exe",      // Учасник 3
        L"logger.exe"           // Учасник 4
    };

    PROCESS_INFORMATION pi[3];
    STARTUPINFOW si[3];

    ZeroMemory(pi, sizeof(pi));
    ZeroMemory(si, sizeof(si));

    // --- запуск процесів ---
    for (int i = 0; i < 3; i++)
    {
        si[i].cb = sizeof(STARTUPINFOW);

        std::wstring cmd = modules[i];
        cmd += arg;    // додаємо IPC метод (1/2/3)

        BOOL ok = CreateProcessW(
            nullptr,
            cmd.data(),   // команда разом з параметром
            nullptr, nullptr,
            FALSE,
            0, nullptr, nullptr,
            &si[i], &pi[i]
        );

        if (!ok)
        {
            std::cout << "Помилка запуску " << i + 1 << " процесу.\n";
            return 1;
        }

        std::cout << "Запущено процес: " << i + 1 << "\n";
    }

    // --- очікування завершення клієнтів ---
    WaitForSingleObject(pi[0].hProcess, INFINITE);
    WaitForSingleObject(pi[1].hProcess, INFINITE);

    // --- logger завершується останнім ---
    WaitForSingleObject(pi[2].hProcess, INFINITE);

    // --- очищення ---
    for (int i = 0; i < 3; i++)
    {
        CloseHandle(pi[i].hProcess);
        CloseHandle(pi[i].hThread);
    }

    std::cout << "\nУсі процеси завершили роботу.\n";
    return 0;
}
