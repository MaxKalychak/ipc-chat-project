#include <windows.h>
#include <iostream>

#define PIPE_C2S L"\\\\.\\pipe\\chat_pipe_c2s" // client -> server
#define PIPE_S2C L"\\\\.\\pipe\\chat_pipe_s2c" // server -> client

int main()
{
    std::wcout << L"[PIPE SERVER] Starting...\n";

    // ---------- CREATE PIPE: client -> server ----------
    HANDLE pipeC2S = CreateNamedPipeW(
        PIPE_C2S,
        PIPE_ACCESS_INBOUND,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1, 512, 512, 0, nullptr
    );

    if (pipeC2S == INVALID_HANDLE_VALUE)
    {
        std::wcerr << L"[SERVER] Error creating C2S pipe: " << GetLastError() << L"\n";
        return 1;
    }

    std::wcout << L"[SERVER] Waiting for client (C2S)...\n";
    ConnectNamedPipe(pipeC2S, nullptr);

    std::wcout << L"[SERVER] Client connected to C2S pipe.\n";

    // ---------- CREATE PIPE: server -> client ----------
    HANDLE pipeS2C = CreateNamedPipeW(
        PIPE_S2C,
        PIPE_ACCESS_OUTBOUND,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1, 512, 512, 0, nullptr
    );

    if (pipeS2C == INVALID_HANDLE_VALUE)
    {
        std::wcerr << L"[SERVER] Error creating S2C pipe: " << GetLastError() << L"\n";
        return 1;
    }

    std::wcout << L"[SERVER] Waiting for client (S2C)...\n";
    ConnectNamedPipe(pipeS2C, nullptr);

    std::wcout << L"[SERVER] Client connected to S2C pipe.\n";

    // ---------- MAIN LOOP ----------
    wchar_t buffer[256];

    while (true)
    {
        DWORD read = 0;
        BOOL ok = ReadFile(pipeC2S, buffer, sizeof(buffer), &read, nullptr);

        if (!ok || read == 0)
        {
            std::wcout << L"[SERVER] Client disconnected.\n";
            break;
        }

        buffer[read / sizeof(wchar_t)] = L'\0';

        std::wcout << L"[SERVER RECEIVED]: " << buffer << L"\n";

        // Відправляємо відповідь клієнту
        std::wstring reply = L"Server received: ";
        reply += buffer;

        DWORD written = 0;
        WriteFile(pipeS2C, reply.c_str(),
            (reply.size() + 1) * sizeof(wchar_t), &written, nullptr);
    }

    CloseHandle(pipeC2S);
    CloseHandle(pipeS2C);

    return 0;
}
