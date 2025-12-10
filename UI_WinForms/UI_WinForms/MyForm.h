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
        TextBox^ txtInput;
        Button^ btnSend;

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

            // ===== INPUT TEXTBOX =====
            txtInput = gcnew TextBox();
            txtInput->Location = Point(40, 500);
            txtInput->Size = Drawing::Size(260, 30);
            txtInput->Font = gcnew Drawing::Font("Segoe UI", 10);
            txtInput->PlaceholderText = L"Type message...";

            // ===== SEND BUTTON =====
            btnSend = gcnew Button();
            btnSend->Text = L"Send";
            btnSend->Location = Point(40, 540);
            btnSend->Size = Drawing::Size(260, 35);
            btnSend->Font = gcnew Drawing::Font("Segoe UI", 11);
            btnSend->Click += gcnew EventHandler(this, &MyForm::btnSend_Click);


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
        void btnSend_Click(Object^ sender, EventArgs^ e)
        {
            String^ message = txtInput->Text;

            if (String::IsNullOrWhiteSpace(message))
            {
                txtLog->AppendText("Cannot send empty message.\r\n");
                return;
            }

            // Додаємо у лог
            txtLog->AppendText("[You]: " + message + "\r\n");

            // Очищаємо поле вводу
            txtInput->Clear();
        }

    };
}
