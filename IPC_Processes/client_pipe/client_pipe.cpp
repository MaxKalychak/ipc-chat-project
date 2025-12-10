#include <windows.h>
#include <iostream>
#include <string>
#include "../../Common/common.h"

int wmain()
{
    std::wcout << L"[PIPE Client] Launching client_pipe...\n";

    // ============================
    // Підключення до пайпа logger'а
    // ============================
    std::wcout << L"[PIPE Client] Waiting for pipe...\n";

    if (!WaitNamedPipeW(PIPE_CLIENT_TO_LOGGER, NMPWAIT_WAIT_FOREVER))
    {
        std::wcerr << L"[PIPE Client] ERROR: WaitNamedPipe failed, code = "
            << GetLastError() << L"\n";
        return 1;
    }

    HANDLE hPipe = CreateFileW(
        PIPE_CLIENT_TO_LOGGER,
        GENERIC_WRITE,      // Pipe → тільки запис
        0,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr
    );

    if (hPipe == INVALID_HANDLE_VALUE)
    {
        std::wcerr << L"[PIPE Client] ERROR: Cannot connect to pipe. Code = "
            << GetLastError() << L"\n";
        return 1;
    }

    std::wcout << L"[PIPE Client] Connected to logger pipe.\n";

    // =====================
    // ОСНОВНИЙ ЦИКЛ
    // =====================
    while (true)
    {
        std::wstring text;
        std::wcout << L"Enter message (exit to quit): ";
        std::getline(std::wcin, text);

        if (text == L"exit")
            break;

        if (text.empty())
            continue;

        // Формуємо ChatMessage
        ChatMessage msg{};
        msg.senderId = CLIENT_PIPE_ID;
        wcsncpy_s(msg.text, text.c_str(), MAX_TEXT - 1);

        DWORD bytesWritten = 0;

        BOOL ok = WriteFile(
            hPipe,
            &msg,
            sizeof(msg),
            &bytesWritten,
            nullptr
        );

        if (!ok)
        {
            std::wcerr << L"[PIPE Client] WriteFile failed. Code = "
                << GetLastError() << L"\n";
            break;
        }

        std::wcout << L"[PIPE Client] Message sent.\n";
    }

    CloseHandle(hPipe);
    std::wcout << L"[PIPE Client] Exiting.\n";
    return 0;
}
