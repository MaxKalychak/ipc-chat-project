#pragma once

#include <windows.h>
#include <vcclr.h>
#include <vector>
#include <string>
#include "../../Common/common.h" 

namespace controller {

    using namespace System;
    using namespace System::Windows::Forms;
    using namespace System::Drawing;
    using namespace System::Threading;
    using namespace System::IO;

    public ref class MyForm : public Form
    {
    public:
        MyForm()
        {
            InitializeComponent();

            // Init IPC Pointers
            hShm = nullptr; hShmMutex = nullptr; hShmSem = nullptr; shmMessages = nullptr;
            hMqMap = nullptr; hMqMutex = nullptr; hMqSem = nullptr; mqQueue = nullptr;
            shmReadIndex = 0; running = false; shmThread = nullptr;

            childProcesses = new std::vector<HANDLE>();
        }

        ~MyForm()
        {
            running = false;

            if (hShmSem != nullptr) {
                ReleaseSemaphore(hShmSem, 1, NULL);
            }

            if (shmThread && shmThread->IsAlive) {
                shmThread->Join(500);
            }

            if (shmMessages) { UnmapViewOfFile(shmMessages); shmMessages = nullptr; }

            SafeCloseHandle(hShm);
            SafeCloseHandle(hShmMutex);
            SafeCloseHandle(hShmSem);

            if (mqQueue) { UnmapViewOfFile(mqQueue); mqQueue = nullptr; }
            SafeCloseHandle(hMqMap);
            SafeCloseHandle(hMqMutex);
            SafeCloseHandle(hMqSem);

            if (childProcesses) {
                delete childProcesses;
                childProcesses = nullptr;
            }

            if (components) delete components;
        }

    protected:
        void SafeCloseHandle(HANDLE% h) {
            if (h != nullptr && h != INVALID_HANDLE_VALUE) {
                CloseHandle(h);
                h = nullptr;
            }
        }

        void OnFormClosing(FormClosingEventArgs^ e) override
        {
            if (childProcesses && !childProcesses->empty()) {
                for (HANDLE h : *childProcesses) {
                    if (h != nullptr && h != INVALID_HANDLE_VALUE) {
                        TerminateProcess(h, 0);
                        CloseHandle(h);
                    }
                }
                childProcesses->clear();
            }
            Form::OnFormClosing(e);
        }

    private:
        HANDLE hShm, hShmMutex, hShmSem;
        ChatMessage* shmMessages;
        int shmReadIndex;
        bool running;
        Thread^ shmThread;

        HANDLE hMqMap, hMqMutex, hMqSem;
        MQQueue* mqQueue;

        std::vector<HANDLE>* childProcesses;

        // UI Controls
        Label^ lblTitle;
        Button^ btnStart;
        TextBox^ txtLog;
        Label^ lblStatus;
        Panel^ headerPanel;
        Panel^ spacerPanel; 
        System::Windows::Forms::Timer^ connectTimer;
        System::ComponentModel::Container^ components;

        void InitializeComponent(void)
        {
            this->components = gcnew System::ComponentModel::Container();
            this->StartPosition = FormStartPosition::CenterScreen;
            this->Text = L"IPC Central System";
            this->ClientSize = Drawing::Size(600, 500);
            this->BackColor = Drawing::Color::FromArgb(30, 30, 30);
            this->ForeColor = Drawing::Color::White;

            headerPanel = gcnew Panel();
            headerPanel->Dock = DockStyle::Top;
            headerPanel->Height = 80;
            headerPanel->BackColor = Drawing::Color::FromArgb(45, 45, 48);
            this->Controls->Add(headerPanel);

            lblTitle = gcnew Label();
            lblTitle->Text = L"IPC PROCESS CONTROLLER";
            lblTitle->Font = gcnew Drawing::Font(L"Segoe UI", 14, FontStyle::Bold);
            lblTitle->ForeColor = Drawing::Color::White;
            lblTitle->Location = Point(20, 15);
            lblTitle->AutoSize = true;
            headerPanel->Controls->Add(lblTitle);

            lblStatus = gcnew Label();
            lblStatus->Text = L"[STATUS] System Stopped";
            lblStatus->Font = gcnew Drawing::Font(L"Segoe UI", 10);
            lblStatus->ForeColor = Drawing::Color::Gray;
            lblStatus->Location = Point(22, 45);
            lblStatus->AutoSize = true;
            headerPanel->Controls->Add(lblStatus);

            btnStart = gcnew Button();
            btnStart->Text = L"LAUNCH SYSTEM";
            btnStart->Font = gcnew Drawing::Font(L"Segoe UI", 9, FontStyle::Bold);
            btnStart->Location = Point(420, 20);
            btnStart->Size = Drawing::Size(150, 40);
            btnStart->FlatStyle = FlatStyle::Flat;
            btnStart->BackColor = Drawing::Color::FromArgb(0, 122, 204);
            btnStart->ForeColor = Drawing::Color::White;
            btnStart->FlatAppearance->BorderSize = 0;
            btnStart->Cursor = Cursors::Hand;
            btnStart->Anchor = static_cast<AnchorStyles>(AnchorStyles::Top | AnchorStyles::Right);
            btnStart->Click += gcnew EventHandler(this, &MyForm::btnStart_Click);
            headerPanel->Controls->Add(btnStart);

            spacerPanel = gcnew Panel();
            spacerPanel->Dock = DockStyle::Top;
            spacerPanel->Height = 15; 
            spacerPanel->BackColor = this->BackColor; 
            this->Controls->Add(spacerPanel);

            txtLog = gcnew TextBox();
            txtLog->Multiline = true;
            txtLog->ReadOnly = true;

            txtLog->ScrollBars = ScrollBars::Vertical;
            txtLog->WordWrap = true;

            txtLog->BorderStyle = BorderStyle::None;
            txtLog->BackColor = Drawing::Color::Black;
            txtLog->ForeColor = Drawing::Color::FromArgb(0, 255, 0);
            txtLog->Font = gcnew Drawing::Font(L"Consolas", 10);

            txtLog->Dock = DockStyle::Fill;
            this->Controls->Add(txtLog);

            headerPanel->BringToFront();
            spacerPanel->BringToFront();
            txtLog->BringToFront();
           
            this->connectTimer = gcnew System::Windows::Forms::Timer(this->components);
            this->connectTimer->Interval = 500;
            this->connectTimer->Tick += gcnew EventHandler(this, &MyForm::OnConnectTimerTick);
        }

        void AddLog(String^ msg) {
            txtLog->AppendText(msg + Environment::NewLine);
            txtLog->SelectionStart = txtLog->Text->Length;
            txtLog->ScrollToCaret();
        }

        void btnStart_Click(Object^ sender, EventArgs^ e)
        {
            if (!hMqMap) {
                hMqMap = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(MQQueue), MQ_FILE_MAPPING_NAME);
                if (hMqMap) mqQueue = (MQQueue*)MapViewOfFile(hMqMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(MQQueue));
                if (mqQueue) { mqQueue->head = 0; mqQueue->tail = 0; mqQueue->count = 0; }
                hMqMutex = CreateMutexW(NULL, FALSE, MQ_MUTEX_NAME);
                hMqSem = CreateSemaphoreW(NULL, 0, MQ_QUEUE_SIZE, MQ_SEMAPHORE_NAME);
                AddLog(L"[SYS] MQ Environment Created.");
            }

            String^ rootDir = nullptr;
            String^ checkPath = Directory::GetCurrentDirectory();
            for (int i = 0; i < 6; i++) {
                if (Directory::Exists(Path::Combine(checkPath, "IPC_Processes"))) { rootDir = checkPath; break; }
                DirectoryInfo^ p = Directory::GetParent(checkPath);
                if (!p) break; checkPath = p->FullName;
            }

            if (!rootDir) { AddLog("[ERR] Can't find 'IPC_Processes' folder."); return; }

            array<String^>^ modules = { "logger_shm", "client_pipe", "client_mqueue" };

            for each (String ^ name in modules) {
                String^ path = Path::Combine(rootDir, "IPC_Processes", name, "x64", "Debug", name + ".exe");
                if (!File::Exists(path)) { AddLog("[ERR] Missing: " + path); continue; }

                pin_ptr<const wchar_t> wpath = PtrToStringChars("\"" + path + "\"");
                std::vector<wchar_t> buf(path->Length + 10);
                wcscpy_s(buf.data(), buf.size(), wpath);

                STARTUPINFOW si; ZeroMemory(&si, sizeof(si)); si.cb = sizeof(si);
                PROCESS_INFORMATION pi;

                DWORD creationFlags = CREATE_NEW_CONSOLE;
                if (name == "logger_shm") creationFlags = CREATE_NO_WINDOW;

                if (CreateProcessW(nullptr, buf.data(), nullptr, nullptr, FALSE, creationFlags, nullptr, nullptr, &si, &pi)) {
                    AddLog("[SYS] Launched: " + name);
                    childProcesses->push_back(pi.hProcess);
                    CloseHandle(pi.hThread);
                }
            }

            lblStatus->Text = L"[STATUS] Waiting for Logger...";
            lblStatus->ForeColor = Drawing::Color::Orange;
            btnStart->Enabled = false;
            btnStart->BackColor = Drawing::Color::Gray;

            connectTimer->Start();
        }

        void OnConnectTimerTick(Object^ sender, EventArgs^ e)
        {
            hShm = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, SHM_NAME);
            if (hShm)
            {
                connectTimer->Stop();
                shmMessages = (ChatMessage*)MapViewOfFile(hShm, FILE_MAP_ALL_ACCESS, 0, 0, SHM_SIZE);
                hShmMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, SHM_MUTEX_NAME);
                hShmSem = OpenSemaphore(SEMAPHORE_MODIFY_STATE | SYNCHRONIZE, FALSE, SHM_SEMAPHORE_NAME);

                if (shmMessages && hShmMutex && hShmSem) {
                    running = true;
                    shmThread = gcnew Thread(gcnew ThreadStart(this, &MyForm::ShmReaderLoop));
                    shmThread->IsBackground = true;
                    shmThread->Start();

                    AddLog("[SYS] Connection Established.");
                    lblStatus->Text = L"[STATUS] Online & Monitoring";
                    lblStatus->ForeColor = Drawing::Color::LimeGreen;
                }
                else {
                    AddLog("[ERR] SHM found, but sync objects missing.");
                }
            }
        }

        void ShmReaderLoop()
        {
            const int LOGGER_ID = 99;
            while (running) {
                if (WaitForSingleObject(hShmSem, INFINITE) != WAIT_OBJECT_0) break;
                if (!running) break;

                WaitForSingleObject(hShmMutex, INFINITE);
                ChatMessage msg = shmMessages[shmReadIndex];
                shmReadIndex = (shmReadIndex + 1) % 10;
                ReleaseMutex(hShmMutex);

                String^ text = gcnew String(msg.text);
                String^ fullLog;
                if (msg.senderId == LOGGER_ID) fullLog = text;
                else fullLog = "[Unknown]: " + text;

                this->BeginInvoke(gcnew Action<String^>(this, &MyForm::AddLog), fullLog);
            }
        }
    };
}