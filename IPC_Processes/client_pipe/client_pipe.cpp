// client_pipe.cpp
// Учасник 2 — чат-клієнт на Pipes/FIFO (Named Pipes, Windows)
//
// Обов'язки:
//  - читає з консолі повідомлення користувача
//  - надсилає їх у пайп (to server)
//  - в окремому потоці отримує повідомлення з пайпа (from server)
//  - коректно обробляє закриття пайпа та команду "exit"
//
// Запуск (через твій контролер):
//    client_pipe.exe 1
// де 1 = обраний метод IPC "Pipes/FIFO".
// Якщо прийшло 2 або 3 — програма чемно завершується.

#include <windows.h>
#include <iostream>
#include <string>

// Імена пайпів (можеш перенести в common.h, якщо хочеш спільні константи)
#define PIPE_C2S L"\\\\.\\pipe\\chat_pipe_c2s"  // client -> server
#define PIPE_S2C L"\\\\.\\pipe\\chat_pipe_s2c"  // server -> client

// ---------------- Потік читання з пайпа ----------------

struct ReaderContext
{
    HANDLE hPipe;   // пайп для читання
    bool* running; // спільний прапорець, щоб зупинити потік
};

DWORD WINAPI ReaderThread(LPVOID param)
{
    ReaderContext* ctx = static_cast<ReaderContext*>(param);
    HANDLE hPipe = ctx->hPipe;
    bool* running = ctx->running;

    wchar_t buffer[256];

    while (*running)
    {
        DWORD bytesRead = 0;
        BOOL ok = ReadFile(
            hPipe,
            buffer,
            sizeof(buffer) - sizeof(wchar_t),
            &bytesRead,
            nullptr
        );

        if (!ok || bytesRead == 0)
        {
            std::wcout << L"[Pipes] Connection closed or read error.\n";
            break;
        }

        // Гарантуємо 0-термінатор (байти прийшли кратно sizeof(wchar_t))
        buffer[bytesRead / sizeof(wchar_t)] = L'\0';

        std::wcout << L"[Remote]: " << buffer << L"\n";
    }

    *running = false;
    return 0;
}

// ------------------------- main -------------------------

int wmain(int argc, wchar_t* argv[])
{
    // 1. Перевіряємо обраний метод IPC
    int method = 1; // за замовчуванням 1

    if (argc >= 2)
    {
        try
        {
            method = std::stoi(argv[1]);
        }
        catch (...)
        {
            method = 1;
        }
    }

    if (method != 1)
    {
        std::wcout << L"[Pipes client] Method = " << method
            << L", це не Pipes/FIFO. Клієнт завершується.\n";
        return 0;
    }

    std::wcout << L"[Pipes client] Старт клієнта на Named Pipes.\n";

    // 2. Підключаємось до пайпа "server -> client" (для читання)
    std::wcout << L"[Pipes client] Очікування пайпа (server -> client)...\n";

    if (!WaitNamedPipeW(PIPE_S2C, NMPWAIT_WAIT_FOREVER))
    {
        std::wcerr << L"[Pipes client] WaitNamedPipe (S2C) failed, error = "
            << GetLastError() << L"\n";
        return 1;
    }

    HANDLE hPipeRead = CreateFileW(
        PIPE_S2C,
        GENERIC_READ,
        0,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr
    );

    if (hPipeRead == INVALID_HANDLE_VALUE)
    {
        std::wcerr << L"[Pipes client] Cannot open pipe (S2C), error = "
            << GetLastError() << L"\n";
        return 1;
    }

    std::wcout << L"[Pipes client] Підключено до пайпа (server -> client).\n";

    // 3. Підключаємось до пайпа "client -> server" (для запису)
    std::wcout << L"[Pipes client] Очікування пайпа (client -> server)...\n";

    if (!WaitNamedPipeW(PIPE_C2S, NMPWAIT_WAIT_FOREVER))
    {
        std::wcerr << L"[Pipes client] WaitNamedPipe (C2S) failed, error = "
            << GetLastError() << L"\n";
        CloseHandle(hPipeRead);
        return 1;
    }

    HANDLE hPipeWrite = CreateFileW(
        PIPE_C2S,
        GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr
    );

    if (hPipeWrite == INVALID_HANDLE_VALUE)
    {
        std::wcerr << L"[Pipes client] Cannot open pipe (C2S), error = "
            << GetLastError() << L"\n";
        CloseHandle(hPipeRead);
        return 1;
    }

    std::wcout << L"[Pipes client] Підключено до пайпа (client -> server).\n";

    // 4. Стартуємо потік читання
    bool running = true;
    ReaderContext ctx{ hPipeRead, &running };

    HANDLE hThread = CreateThread(
        nullptr,
        0,
        ReaderThread,
        &ctx,
        0,
        nullptr
    );

    if (!hThread)
    {
        std::wcerr << L"[Pipes client] Cannot create reader thread, error = "
            << GetLastError() << L"\n";
        CloseHandle(hPipeRead);
        CloseHandle(hPipeWrite);
        return 1;
    }

    // 5. Основний цикл: введення з консолі -> запис у пайп
    std::wcout << L"Введіть повідомлення (\"exit\" — вихід):\n";

    std::wstring line;

    // Позбутися можливого залишку '\n' у wcin після контролера
    std::wcin.sync();

    while (running)
    {
        std::getline(std::wcin, line);

        if (!running) break;

        if (line == L"exit")
        {
            std::wcout << L"[Pipes client] Команда exit. Завершення.\n";
            running = false;
            break;
        }

        if (line.empty())
            continue;

        DWORD bytesWritten = 0;
        BOOL ok = WriteFile(
            hPipeWrite,
            line.c_str(),
            static_cast<DWORD>((line.size() + 1) * sizeof(wchar_t)), // з 0-термінатором
            &bytesWritten,
            nullptr
        );

        if (!ok)
        {
            std::wcerr << L"[Pipes client] WriteFile failed, error = "
                << GetLastError() << L"\n";
            running = false;
            break;
        }
    }

    // 6. Прибираємося
    running = false;
    // Дозволяємо ReaderThread вийти з ReadFile
    CancelSynchronousIo(hThread);
    WaitForSingleObject(hThread, 2000);
    CloseHandle(hThread);

    CloseHandle(hPipeWrite);
    CloseHandle(hPipeRead);

    std::wcout << L"[Pipes client] Завершено.\n";
    return 0;
}
