//---------------------------------------------------------------------------

#include <vcl.h>
#include <stdio.h>
#include <stdarg.h>
#pragma hdrstop

#include "cbmain.h"
#include "about.h"
#include "sys_conf.h"
#include "done.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TMainForm *MainForm;

extern "C" {
        extern int unixmain (int argc, char *argv[]);
}

static struct sysconf_t        *sysconf = NULL;
//---------------------------------------------------------------------------
__fastcall TMainForm::TMainForm(TComponent* Owner)
        : TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::BitBtnCloseClick(TObject *Sender)
{
        Application->Terminate ();        
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::BitBtnWishClick(TObject *Sender)
{
        if (OpenDialogWish->Execute ()) {
                EditWish->Text = OpenDialogWish->FileName;
        }
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::BitBtnStdinfoClick(TObject *Sender)
{
        if (OpenDialogStdinfo->Execute ()) {
                EditStdinfo->Text = OpenDialogStdinfo->FileName;
        }
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::BitBtnScoreClick(TObject *Sender)
{
        if (OpenDialogScore->Execute ()) {
                EditScore->Text = OpenDialogScore->FileName;
        }
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::BitBtnDeplimClick(TObject *Sender)
{
        if (OpenDialogDeplim->Execute ()) {
                EditDeplim->Text = OpenDialogDeplim->FileName;
        }
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::BitBtnResultClick(TObject *Sender)
{
        if (OpenDialogResult->Execute ()) {
                EditResult->Text = OpenDialogResult->FileName;
        }
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::BitBtnRunClick(TObject *Sender)
{
        static  bool    already_run = false;
        char            *outfmt = "1";

        if (already_run) {
                FormDone->ShowModal ();
                return;
        }

        already_run = true;

        TCursor Save_Cursor = Screen->Cursor;
        Screen->Cursor = crHourGlass;    // Show hourglass cursor

        BitBtnClose->Enabled = false;
        BitBtnRun->Enabled = false;

        Memo1->SetSelTextBuf ("Starting ...\r\n");
        sysconf->addstr ("student-score-file", EditScore->Text.c_str ());
        sysconf->addstr ("student-wishes-file",EditWish->Text.c_str ());
        sysconf->addstr ("student-info-file",  EditStdinfo->Text.c_str ());
        sysconf->addstr ("department-limit-file", EditDeplim->Text.c_str ());
        sysconf->addstr ("standard-file", EditStandard->Text.c_str ());
        sysconf->addstr ("output-file", EditResult->Text.c_str ());
        sysconf->addstr ("deny-list-file", EditDenylist->Text.c_str ());
        outfmt[0] = output_format + '0';
        sysconf->addint ("output-format", outfmt);
        unixmain (0, NULL);

        BitBtnClose->Enabled = true;
        BitBtnRun->Enabled = true;

        Screen->Cursor = Save_Cursor; // always restore the cursor

        FormDone->ShowModal ();
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::BitBtnAboutClick(TObject *Sender)
{
        AboutBox->ShowModal ();
}

extern "C" {
        static int      printlevel = 5;

	int set_print_level_on_memo (const int level) {
		int	retval = printlevel;

		printlevel = level;

		return retval;
	}

        int print_on_memo (const int level, const char *fmt, ...) {
                int     len;
                char buffer[4096];

        	if (level <= printlevel) {
	        	va_list	ap;

		        va_start (ap, fmt);
        		len = vsnprintf (buffer, 4095, fmt, ap);
	        	va_end (ap);

                        MainForm->Memo1->SetSelTextBuf (buffer);
        	}
                return len;
        }

        void perror_on_memo (const int level) {
                char    buffer[4096];
                int     i, len;

                if (level < printlevel) {
                        snprintf (buffer, 4095, "%s", strerror (errno));
                        len = strlen (buffer);
                        for (i = len-1; i > 0; i--) {
                                if ((buffer[i] == '\r') || (buffer[i] == '\n')) {
                                        buffer[i] = '\0';
                                } else {
                                        i++;
                                        break;
                                }
                        }
                        buffer[i++] = '\r';
                        buffer[i++] = '\n';
                        buffer[i++] = '\0';
                        MainForm->Memo1->SetSelTextBuf (buffer);
                }
        }
}
//---------------------------------------------------------------------------



void __fastcall TMainForm::FormCreate(TObject *Sender)
{
        output_format = 2;
        RadioButton1->Checked = true;
        sysconf = initial_sysconf_module (NULL);
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::BitBtnStandardClick(TObject *Sender)
{
        if (OpenDialogStandard->Execute ()) {
                EditStandard->Text = OpenDialogStandard->FileName;
        }
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::BitBtnDenylistClick(TObject *Sender)
{
        if (OpenDialogDenylist->Execute ()) {
                EditDenylist->Text = OpenDialogDenylist->FileName;
        }
}
//---------------------------------------------------------------------------


void __fastcall TMainForm::RadioButton2Click(TObject *Sender)
{
        output_format = 1;
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::RadioButton1Click(TObject *Sender)
{
        output_format = 2;        
}
//---------------------------------------------------------------------------


