//---------------------------------------------------------------------------

#ifndef cbmainH
#define cbmainH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <Buttons.hpp>
#include <ExtCtrls.hpp>
#include <Dialogs.hpp>
#include <Menus.hpp>
//---------------------------------------------------------------------------
class TMainForm : public TForm
{
__published:	// IDE-managed Components
        TPanel *Panel1;
        TLabel *Label1;
        TMemo *Memo1;
        TOpenDialog *OpenDialogWish;
        TEdit *EditStdinfo;
        TLabel *Label2;
        TBitBtn *BitBtnStdinfo;
        TLabel *Label3;
        TLabel *Label4;
        TEdit *EditScore;
        TBitBtn *BitBtnScore;
        TLabel *Label5;
        TLabel *Label6;
        TEdit *EditDeplim;
        TBitBtn *BitBtnDeplim;
        TLabel *Label7;
        TEdit *EditWish;
        TBitBtn *BitBtnWish;
        TEdit *EditResult;
        TBitBtn *BitBtnResult;
        TBitBtn *BitBtnRun;
        TBitBtn *BitBtnClose;
        TBitBtn *BitBtnAbout;
        TRadioGroup *RadioGroup1;
        TRadioButton *RadioButton1;
        TRadioButton *RadioButton2;
        TLabel *Label8;
        TEdit *EditStandard;
        TLabel *Label9;
        TEdit *EditDenylist;
        TBitBtn *BitBtnStandard;
        TBitBtn *BitBtnDenylist;
        TOpenDialog *OpenDialogStdinfo;
        TOpenDialog *OpenDialogScore;
        TOpenDialog *OpenDialogDeplim;
        TOpenDialog *OpenDialogStandard;
        TOpenDialog *OpenDialogDenylist;
        TOpenDialog *OpenDialogResult;
        void __fastcall BitBtnCloseClick(TObject *Sender);
        void __fastcall BitBtnWishClick(TObject *Sender);
        void __fastcall BitBtnStdinfoClick(TObject *Sender);
        void __fastcall BitBtnScoreClick(TObject *Sender);
        void __fastcall BitBtnDeplimClick(TObject *Sender);
        void __fastcall BitBtnResultClick(TObject *Sender);
        void __fastcall BitBtnRunClick(TObject *Sender);
        void __fastcall BitBtnAboutClick(TObject *Sender);
        void __fastcall FormCreate(TObject *Sender);
        void __fastcall BitBtnStandardClick(TObject *Sender);
        void __fastcall BitBtnDenylistClick(TObject *Sender);
        void __fastcall RadioButton2Click(TObject *Sender);
        void __fastcall RadioButton1Click(TObject *Sender);
private:	// User declarations
        int     output_format;
public:		// User declarations
        __fastcall TMainForm(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TMainForm *MainForm;
//---------------------------------------------------------------------------
#endif
