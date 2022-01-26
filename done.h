//---------------------------------------------------------------------------

#ifndef doneH
#define doneH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <Buttons.hpp>
//---------------------------------------------------------------------------
class TFormDone : public TForm
{
__published:	// IDE-managed Components
        TBitBtn *BitBtn1;
private:	// User declarations
public:		// User declarations
        __fastcall TFormDone(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TFormDone *FormDone;
//---------------------------------------------------------------------------
#endif
