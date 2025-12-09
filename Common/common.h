#ifndef COMMON_H
#define COMMON_H

#include <windows.h>

// ============================================================
//                 СПІЛЬНІ ПАРАМЕТРИ IPC ЧАТУ
// ============================================================

// Максимальна довжина повідомлення
#define MAX_TEXT 256

// Внутрішній формат повідомлення для обміну
struct ChatMessage
{
    int senderId;              // 1 = client_pipe, 2 = client_mqueue
    wchar_t text[MAX_TEXT];    // сам текст повідомлення
};

// ============================================================
//                    NAMED PIPE (Pipes/FIFO)
// ============================================================
//
// Канали для interaction між client_pipe ? logger або controller.
// У Windows pipes називаються через \\.\pipe\
// ============================================================

#define PIPE_CLIENT_TO_LOGGER  L"\\\\.\\pipe\\ChatPipe_ClientToLogger"
#define PIPE_LOGGER_TO_CLIENT  L"\\\\.\\pipe\\ChatPipe_LoggerToClient"

// Можна зробити кілька каналів, якщо треба більше процесів

// ============================================================
//                    MESSAGE QUEUE (Windows-styled)
// ============================================================
// У Windows НІМАЄ POSIX msgget/msgsnd,
// тому message queue моделюється через:
// 1) Named Pipe (як альтернатива POSIX msgqueue) або
// 2) File Mapping + Mutex/Semaphore.
//
// Тут ми зробимо QUEUE через FileMapping (кроспроцесна черга).
// ============================================================

#define MQ_FILE_MAPPING_NAME   L"Local\\ChatMessageQueue" //
#define MQ_MUTEX_NAME          L"Local\\ChatMQ_Mutex" //
#define MQ_SEMAPHORE_NAME      L"Local\\ChatMQ_Semaphore" //
#define MQ_QUEUE_SIZE          10  // кількість повідомлень у черзі

struct MQSlot
{
    bool used;
    ChatMessage msg;
};

// ============================================================
//            SHARED MEMORY + SEMAPHORE (LOGGER)
// ============================================================
//
// Логер і клієнти спільно використовують сегмент пам’яті.
// ============================================================

#define SHM_NAME               L"Global\\ChatSharedMemory"
#define SHM_SIZE               (sizeof(ChatMessage) * 10)  // буфер для 10 повідомлень

#define SHM_SEMAPHORE_NAME    L"Global\\ChatSHM_Sem"
#define SHM_MUTEX_NAME        L"Global\\ChatSHM_Mutex"

// ============================================================
//                   ІДЕНТИФІКАТОРИ ПРОЦЕСІВ
// ============================================================
//
// Щоб controller знав, хто що робить
// ============================================================

#define CLIENT_PIPE_ID         1
#define CLIENT_MQUEUE_ID       2

// ============================================================
//                    НАЗВИ EXE ФАЙЛІВ
// ============================================================
//
// Це потрібно controller.exe для CreateProcess()
// ============================================================

#define EXE_CONTROLLER         L"controller.exe"
#define EXE_PIPE_CLIENT        L"client_pipe.exe"
#define EXE_MQUEUE_CLIENT      L"client_mqueue.exe"
#define EXE_LOGGER             L"logger_shm.exe"

struct MQMessage {
    wchar_t text[256];
};

struct MQQueue {
    MQMessage messages[MQ_QUEUE_SIZE];
    int head;
    int tail;
    int count;
};


#endif // COMMON_H
