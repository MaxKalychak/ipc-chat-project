#ifndef COMMON_H
#define COMMON_H

#include <windows.h>

#define MAX_TEXT 256

struct ChatMessage
{
    int senderId;             
    wchar_t text[MAX_TEXT];
};


#define PIPE_CLIENT_TO_LOGGER  L"\\\\.\\pipe\\ChatPipe_ClientToLogger"
#define PIPE_LOGGER_TO_CLIENT  L"\\\\.\\pipe\\ChatPipe_LoggerToClient"


#define MQ_FILE_MAPPING_NAME   L"Local\\ChatMessageQueue"
#define MQ_MUTEX_NAME          L"Local\\ChatMQ_Mutex"
#define MQ_SEMAPHORE_NAME      L"Local\\ChatMQ_Semaphore"
#define MQ_QUEUE_SIZE          10  

struct MQSlot
{
    bool used;
    ChatMessage msg;
};

#define SHM_NAME               L"Local\\ChatSharedMemory"
#define SHM_SIZE               (sizeof(ChatMessage) * 10)

#define SHM_SEMAPHORE_NAME     L"Local\\ChatSHM_Sem"
#define SHM_MUTEX_NAME         L"Local\\ChatSHM_Mutex"


#define CLIENT_PIPE_ID         1
#define CLIENT_MQUEUE_ID       2

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


#endif
