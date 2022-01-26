//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop
USERES("Dispatcher.res");
USEFORM("cbmain.cpp", MainForm);
USEFORM("about.cpp", AboutBox);
USEUNIT("department.c");
USEUNIT("dispunit.c");
USEUNIT("main.c");
USEUNIT("misclib.c");
USEUNIT("stdvalue.c");
USEUNIT("student.c");
USEUNIT("studlist.c");
USEUNIT("sys_conf.c");
USEUNIT("textio.c");
USEFORM("done.cpp", FormDone);
//---------------------------------------------------------------------------
WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
        try
        {
                 Application->Initialize();
                 Application->CreateForm(__classid(TMainForm), &MainForm);
                 Application->CreateForm(__classid(TAboutBox), &AboutBox);
                 Application->CreateForm(__classid(TFormDone), &FormDone);
                 Application->Run();
        }
        catch (Exception &exception)
        {
                 Application->ShowException(&exception);
        }
        return 0;
}
//---------------------------------------------------------------------------
