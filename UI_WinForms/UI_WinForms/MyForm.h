#pragma once

#include <windows.h>
#include <vcclr.h>

namespace controller {

    using namespace System;
    using namespace System::Windows::Forms;
    using namespace System::Drawing;

    public ref class MyForm : public Form
    {
    public:
        MyForm()
        {
            InitializeComponent();
        }

    protected:
        ~MyForm()
        {
            if (components)
                delete components;
        }

    private:
        int selectedMethod = 0;

        Label^ lblTitle;
        Button^ btnPipe;
        Button^ btnMsg;
        Button^ btnShm;
        Button^ btnStart;
        Label^ lblSelected;
        TextBox^ txtLog;

        System::ComponentModel::Container^ components;

        void InitializeComponent(void)
        {
            this->Text = L"IPC Controller";
            this->ClientSize = Drawing::Size(360, 520);
            this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::FixedSingle;
            this->MaximizeBox = false;

            lblTitle = gcnew Label();
            lblTitle->Text = L"IPC Controller";
            lblTitle->Font = gcnew Drawing::Font(L"Segoe UI", 20);
            lblTitle->Location = Point(40, 10);
            lblTitle->Size = Drawing::Size(300, 40);

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

            lblSelected = gcnew Label();
            lblSelected->Text = L"Selected: none";
            lblSelected->Location = Point(40, 290);
            lblSelected->Size = Drawing::Size(260, 20);

            txtLog = gcnew TextBox();
            txtLog->Location = Point(40, 320);
            txtLog->Size = Drawing::Size(260, 160);
            txtLog->Multiline = true;
            txtLog->ReadOnly = true;
            txtLog->ScrollBars = ScrollBars::Vertical;

            this->Controls->Add(lblTitle);
            this->Controls->Add(btnPipe);
            this->Controls->Add(btnMsg);
            this->Controls->Add(btnShm);
            this->Controls->Add(btnStart);
            this->Controls->Add(lblSelected);
            this->Controls->Add(txtLog);
        }

        // ====== IPC selection ======

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

        // ====== Start processes ======

        void btnStart_Click(Object^ sender, EventArgs^ e)
        {
            if (selectedMethod == 0)
            {
                txtLog->AppendText("ERROR: Select method first!\r\n");
                return;
            }

            array<String^>^ procs = {
                "client_pipe.exe",
                "client_msg.exe",
                "logger.exe"
            };

            for each(String ^ exe in procs)
            {
                String^ cmd = exe + " " + selectedMethod.ToString();
                pin_ptr<const wchar_t> wcmd = PtrToStringChars(cmd);

                STARTUPINFO si = { sizeof(si) };
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
                    txtLog->AppendText("Launched: " + exe + "\r\n");
                    CloseHandle(pi.hProcess);
                    CloseHandle(pi.hThread);
                }
                else
                {
                    txtLog->AppendText("FAILED: " + exe + "\r\n");
                }
            }
        }
    };
}
