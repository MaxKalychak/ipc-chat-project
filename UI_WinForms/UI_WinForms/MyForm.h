#pragma once

#include <windows.h>
#include <vcclr.h>
#include "../../Common/common.h"   // тут ChatMessage, SHM_NAME, SHM_MUTEX_NAME, SHM_SEMAPHORE_NAME, SHM_SIZE

namespace controller {

    using namespace System;
    using namespace System::Windows::Forms;
    using namespace System::Drawing;
    using namespace System::Threading;

    public ref class MyForm : public Form
    {
    public:
        MyForm()
        {
            InitializeComponent();

            // початкові значення для SHM
            hShm = nullptr;
            hShmMutex = nullptr;
            hShmSem = nullptr;
            shmMessages = nullptr;
            shmReadIndex = 0;
            running = false;
            shmThread = nullptr;
        }

        ~MyForm()
        {
            // зупиняємо цикл читання
            running = false;

            // чекаємо потік (якщо він є)
            if (shmThread != nullptr && shmThread->IsAlive)
            {
                shmThread->Join(200);
            }

            // звільняємо ресурси WinAPI
            if (shmMessages)
            {
                UnmapViewOfFile(shmMessages);
                shmMessages = nullptr;
            }

            if (hShm) { CloseHandle(hShm);       hShm = nullptr; }
            if (hShmMutex) { CloseHandle(hShmMutex);  hShmMutex = nullptr; }
            if (hShmSem) { CloseHandle(hShmSem);    hShmSem = nullptr; }

            if (components)
            {
                delete components;
            }
        }

    private:
        // ================== IPC поля ==================

        HANDLE      hShm;
        HANDLE      hShmMutex;
        HANDLE      hShmSem;
        ChatMessage* shmMessages;
        int         shmReadIndex;
        bool        running;
        Thread^ shmThread;

        // ================== UI поля ===================

        int      selectedMethod;

        Label^ lblTitle;
        Button^ btnPipe;
        Button^ btnMsg;
        Button^ btnShm;
        Button^ btnStart;
        Label^ lblSelected;
        TextBox^ txtLog;
        TextBox^ txtInput;
        Button^ btnSend;

        System::ComponentModel::Container^ components;

        // ============================================================
        //                 ІНІЦІАЛІЗАЦІЯ ФОРМИ
        // ============================================================

        void InitializeComponent(void)
        {
            this->StartPosition = FormStartPosition::CenterScreen;
            this->Text = L"IPC Controller";
            this->ClientSize = Drawing::Size(360, 600);
            this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::Sizable;
            this->MinimumSize = Drawing::Size(400, 600);

            selectedMethod = 0;

            // ------- TITLE -------
            lblTitle = gcnew Label();
            lblTitle->Text = L"IPC Controller";
            lblTitle->Font = gcnew Drawing::Font(L"Segoe UI", 20);
            lblTitle->Location = Point(40, 10);
            lblTitle->Size = Drawing::Size(300, 40);

            // ------- BUTTONS -------
            btnPipe = gcnew Button();
            btnPipe->Text = L"Pipes / FIFO";
            btnPipe->Location = Point(40, 70);
            btnPipe->Size = Drawing::Size(260, 40);
            btnPipe->Click += gcnew EventHandler(this, &MyForm::btnPipe_Click);

            btnMsg = gcnew Button();
            btnMsg->Text = L"Message Queue";
            btnMsg->Location = Point(40, 120);
            btnMsg->Size = Drawing::Size(260, 40);
            btnMsg->Click += gcnew EventHandler(this, &MyForm::btnMsg_Click);

            btnShm = gcnew Button();
            btnShm->Text = L"Shared Memory";
            btnShm->Location = Point(40, 170);
            btnShm->Size = Drawing::Size(260, 40);
            btnShm->Click += gcnew EventHandler(this, &MyForm::btnShm_Click);

            btnStart = gcnew Button();
            btnStart->Text = L"Start System";
            btnStart->Location = Point(40, 230);
            btnStart->Size = Drawing::Size(260, 40);
            btnStart->Click += gcnew EventHandler(this, &MyForm::btnStart_Click);

            // ------- SELECTED LABEL -------
            lblSelected = gcnew Label();
            lblSelected->Text = L"Selected: none";
            lblSelected->Location = Point(40, 290);
            lblSelected->Size = Drawing::Size(260, 20);

            // ------- LOG BOX -------
            txtLog = gcnew TextBox();
            txtLog->Location = Point(40, 320);
            txtLog->Size = Drawing::Size(260, 160);
            txtLog->Multiline = true;
            txtLog->ReadOnly = true;
            txtLog->ScrollBars = ScrollBars::Vertical;
            txtLog->Anchor = AnchorStyles::Top | AnchorStyles::Left |
                AnchorStyles::Right | AnchorStyles::Bottom;

            // ------- INPUT TEXTBOX -------
            txtInput = gcnew TextBox();
            txtInput->Location = Point(40, 500);
            txtInput->Size = Drawing::Size(260, 30);
            txtInput->Text = L"Type message...";
            txtInput->ForeColor = Drawing::Color::Gray;
            txtInput->Anchor = AnchorStyles::Left | AnchorStyles::Right | AnchorStyles::Bottom;
            txtInput->Enter += gcnew EventHandler(this, &MyForm::txtInput_Enter);
            txtInput->Leave += gcnew EventHandler(this, &MyForm::txtInput_Leave);

            // ------- SEND BUTTON -------
            btnSend = gcnew Button();
            btnSend->Text = L"Send";
            btnSend->Location = Point(40, 540);
            btnSend->Size = Drawing::Size(260, 35);
            btnSend->Anchor = AnchorStyles::Left | AnchorStyles::Right | AnchorStyles::Bottom;
            btnSend->Click += gcnew EventHandler(this, &MyForm::btnSend_Click);

            // ------- ADD CONTROLS -------
            this->Controls->Add(lblTitle);
            this->Controls->Add(btnPipe);
            this->Controls->Add(btnMsg);
            this->Controls->Add(btnShm);
            this->Controls->Add(btnStart);
            this->Controls->Add(lblSelected);
            this->Controls->Add(txtLog);
            this->Controls->Add(txtInput);
            this->Controls->Add(btnSend);
        }

        // ============================================================
        //                      ДОПОМІЖНИЙ ЛОГЕР
        // ============================================================

    public:
        void AddLog(String^ msg)
        {
            txtLog->AppendText(msg + "\r\n");
        }

    private:
        // ============================================================
        //                  ВИБІР МЕТОДУ IPC
        // ============================================================

        void btnPipe_Click(Object^ sender, EventArgs^ e)
        {
            selectedMethod = 1;
            lblSelected->Text = L"Selected: Pipes";
        }

        void btnMsg_Click(Object^ sender, EventArgs^ e)
        {
            selectedMethod = 2;
            lblSelected->Text = L"Selected: Message Queue";
        }

        void btnShm_Click(Object^ sender, EventArgs^ e)
        {
            selectedMethod = 3;
            lblSelected->Text = L"Selected: Shared Memory";
        }

        // ============================================================
        //                    СТАРТ СИСТЕМИ
        // ============================================================

        void btnStart_Click(Object^ sender, EventArgs^ e)
        {
            if (selectedMethod == 0)
            {
                AddLog(L"ERROR: Select method first!");
                return;
            }

            // повні шляхи (ти вже їх налаштовувала — можна підправити)
            array<String^>^ procs = {
                L"C:\\Users\\Admin\\Desktop\\ipc-chat-project\\IPC_Processes\\client_pipe\\Debug\\client_pipe.exe",
                L"C:\\Users\\Admin\\Desktop\\ipc-chat-project\\IPC_Processes\\client_mqueue\\Debug\\client_mqueue.exe",
                L"C:\\Users\\Admin\\Desktop\\ipc-chat-project\\IPC_Processes\\logger_shm\\Debug\\logger.exe"
            };

            String^ arg = " " + selectedMethod.ToString();

            for each (String ^ exe in procs)
            {
                String^ fullCmd = exe + arg;
                pin_ptr<const wchar_t> wcmd = PtrToStringChars(fullCmd);

                STARTUPINFOW si;
                ZeroMemory(&si, sizeof(si));
                si.cb = sizeof(si);
                PROCESS_INFORMATION pi;

                BOOL ok = CreateProcessW(
                    nullptr,
                    (LPWSTR)wcmd,
                    nullptr, nullptr,
                    FALSE, 0,
                    nullptr, nullptr,
                    &si, &pi
                );

                if (ok)
                {
                    AddLog("Launched: " + exe);
                    CloseHandle(pi.hProcess);
                    CloseHandle(pi.hThread);
                }
                else
                {
                    AddLog("FAILED: " + exe);
                }
            }

            // підключаємось до shared memory, mutex і semaphore,
            // які створює logger (твій код вище)
            InitSharedMemory();
        }

        // ============================================================
        //               ПІДКЛЮЧЕННЯ ДО SHARED MEMORY
        // ============================================================

        void InitSharedMemory()
        {
            // кілька спроб — раптом logger ще не встиг створити об’єкти
            AddLog("Connecting to shared memory...");

            hShm = nullptr;
            for (int i = 0; i < 10 && !hShm; ++i)
            {
                hShm = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, SHM_NAME);
                if (!hShm) Thread::Sleep(200);
            }

            if (!hShm)
            {
                AddLog("ERROR: Cannot open SHM.");
                return;
            }

            shmMessages = (ChatMessage*)MapViewOfFile(
                hShm, FILE_MAP_ALL_ACCESS, 0, 0, SHM_SIZE
            );

            if (!shmMessages)
            {
                AddLog("ERROR: MapViewOfFile(SHM) failed.");
                CloseHandle(hShm); hShm = nullptr;
                return;
            }

            // mutex + semaphore
            hShmMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, SHM_MUTEX_NAME);
            hShmSem = OpenSemaphore(SEMAPHORE_MODIFY_STATE | SYNCHRONIZE,
                FALSE, SHM_SEMAPHORE_NAME);

            if (!hShmMutex || !hShmSem)
            {
                AddLog("ERROR: Cannot open SHM mutex or semaphore.");
                return;
            }

            shmReadIndex = 0;
            running = true;

            // запускаємо керований потік, який буде читати з SHM
            shmThread = gcnew Thread(gcnew ThreadStart(this, &MyForm::ShmReaderLoop));
            shmThread->IsBackground = true;
            shmThread->Start();

            AddLog("Shared memory reader started.");
        }

        // ============================================================
        //            ЦИКЛ ЧИТАННЯ ПОВІДОМЛЕНЬ З SHM (ПОТІК)
        // ============================================================

        void ShmReaderLoop()
        {
            while (running)
            {
                // чекаємо, поки logger запише нове повідомлення
                DWORD waitRes = WaitForSingleObject(hShmSem, INFINITE);
                if (waitRes != WAIT_OBJECT_0)
                    break;

                WaitForSingleObject(hShmMutex, INFINITE);

                ChatMessage msg = shmMessages[shmReadIndex];
                shmReadIndex = (shmReadIndex + 1) % 10;

                ReleaseMutex(hShmMutex);

                String^ text = gcnew String(msg.text);

                // безпечно оновлюємо UI з іншого потоку
                this->BeginInvoke(
                    gcnew Action<String^>(this, &MyForm::AddLog),
                    text
                );
            }
        }

        // ============================================================
        //                ОБРОБКА ПОЛЯ ВВОДУ ТА КНОПКИ SEND
        // ============================================================

        void txtInput_Enter(Object^ sender, EventArgs^ e)
        {
            if (txtInput->Text == L"Type message...")
            {
                txtInput->Text = "";
                txtInput->ForeColor = Drawing::Color::Black;
            }
        }

        void txtInput_Leave(Object^ sender, EventArgs^ e)
        {
            if (String::IsNullOrWhiteSpace(txtInput->Text))
            {
                txtInput->Text = L"Type message...";
                txtInput->ForeColor = Drawing::Color::Gray;
            }
        }

        void btnSend_Click(Object^ sender, EventArgs^ e)
        {
            String^ msg = txtInput->Text;

            if (String::IsNullOrWhiteSpace(msg) || msg == L"Type message...")
            {
                AddLog("Cannot send empty message.");
                return;
            }

            AddLog("[You]: " + msg);
            txtInput->Clear();

            // тут можна буде додати логіку,
            // щоб відправляти повідомлення в Message Queue / Pipes,
            // коли код інших учасників буде готовий
        }
    };
}
