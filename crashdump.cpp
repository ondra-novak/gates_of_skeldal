#define WINVER 0x0500
#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>


#ifndef __specstrings
#define __specstrings

#define PSTR char *
#define PCSTR const char *
#define PULONG_PTR DWORD *
#define ULONG_PTR DWORD
 #define __in
 #define __out
 #define __inout
 #define __in_opt
 #define __out_opt
 #define __inout_opt
 #define __in_ecount(x)
 #define __out_ecount(x)
 #define __inout_ecount(x)
 #define __in_bcount(x)
 #define __out_bcount(x)
 #define __inout_bcount(x)
 #define __deref_opt_out
 #define __deref_out
 #define __out_xcount(x)
#endif


#include <DbgHelp.h>
typedef BOOL (WINAPI  *MiniDumpWriteDump_Type)(
  HANDLE hProcess,
  DWORD ProcessId,
  HANDLE hFile,
  MINIDUMP_TYPE DumpType,
  PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
  PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
  PMINIDUMP_CALLBACK_INFORMATION CallbackParam
);

static MiniDumpWriteDump_Type MiniDumpWriteDumpFn;

static DWORD WINAPI SafeMsgBox(LPVOID text)
{
  MessageBox(NULL,(LPCTSTR)text,NULL,MB_OK|MB_SYSTEMMODAL);
  return 0;
}


static bool GenerateMinidump(MINIDUMP_EXCEPTION_INFORMATION *ExceptionInfo, MINIDUMP_TYPE flags,const char *sufix)
{
  char buff[MAX_PATH+50];
  HANDLE hFile;
  GetModuleFileName(NULL,buff,MAX_PATH);
  strcat(buff,sufix);
  hFile=CreateFile(buff,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,0,NULL);
  if (hFile==NULL)
  {
	char *text=(char *)alloca(strlen(buff)+200);
	sprintf(text,"Nemohu vytvorit soubor zaznamu '%s', zkontroluje prava zapisu do adresare hry",buff);
    MessageBox(0,text,0,MB_SYSTEMMODAL);
    return false;
  }
  else
  {
    if (MiniDumpWriteDumpFn(GetCurrentProcess(),GetCurrentProcessId(),hFile,flags,ExceptionInfo,NULL,NULL)!=TRUE)
    {
	  char *text=(char *)alloca(strlen(buff)+200);
	  sprintf(text,"Selhalo volani MiniDumpWriteDump. Nelze vygenerovat zaznam do souboru '%s'",buff);
      MessageBox(0,text,0,MB_SYSTEMMODAL);
	  return false;
    }
    CloseHandle(hFile);
  }
  return true;
}

static BOOL WINAPI CloseOneWindow(HWND hWnd, LPARAM lParam)
{
  CloseWindow(hWnd);
  return TRUE;
}

static void CloseAllWindows()
{
  EnumThreadWindows(GetCurrentThreadId(),CloseOneWindow,NULL);
}


extern "C" { int GetExeVersion();}

static DWORD WINAPI PostError(LPVOID p)
{
  char buff[MAX_PATH+50];
  char dmp[MAX_PATH+50];
  char product[MAX_PATH];
  char *tmp;
  GetModuleFileName(NULL,buff,MAX_PATH);    
  tmp=strrchr(buff,'\\');
  if (tmp==0) tmp=buff;else tmp++;
  strcpy(product,tmp);
  strcpy(dmp,buff);
  strcpy(strrchr(buff,'\\')+1,"poslichybu.html");
  strcat(dmp,".short.dmp");
  DeleteFile(buff);
  HANDLE h=CreateFile(buff,GENERIC_WRITE,0,0,CREATE_ALWAYS,0,0);
  if (h!=INVALID_HANDLE_VALUE)
  {
    DWORD wrt;
    DWORD rd;
    const char *form1=    
      "<?xml version=\"1.0\" encoding=\"Windows-1250\"?>"
      "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">"
      "<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"cs\" lang=\"cs\">"
      "<head>"
      "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=windows-1250\" />"
      "<title>Br�ny Skeldalu - formul�� k odesl�n� chyby</title>"
      "<style type=\"text/css\">\n"
      "@import url(\"http://skeldal.jinak.cz/style.css\");\n"
      "</style>"
      "</head>"
      "<body>"
      "<div style=\"float: left; width: 155px;\"><img src=\"http://skeldal.jinak.cz/logo.jpg\" alt=\"logo\" width=\"150\" height=\"82\"/></div>"
      "<div style=\"margin-left: 180px;margin-right: 50px;\">"
      "<h1>Br�ny Skeldalu - formul�� k odesl�n� chyby</h1>"
      "<p>Autor programu se omlouv� za chybu a pros� V�s, abyste pomohl p�i odhalov�n� chyb.<br />"
      "Sta�� kdy� pravdiv� vypln�te ode�lete n�sleduj�c� formul��.<br />"
      "Pokuste se pros�m napsat co nejv�ce informac�.<br />"
      "<b>K formul��i je p�ilo�en soubor obsahuj�c� z�znam o chyb�.</b><br /><br />"
      "D�kujeme za spolupr�ci</p>"
      "<hr />"
      "<form enctype=\"multipart/form-data\" action=\"http://skeldal.jinak.cz/bugreport.php\" method=\"post\"><div>"
      "<div>V� e-mail pro p��pad, �e bysme v�s cht�li kontaktovat: (nepovinn�)</div>"
      "<input type=\"text\" name=\"email\" value=\"\" size=\"50\"/>"
      "<div>Napi�te pros�m dal�� informace o tom, za jak�ch podm�nek chyba vznikla, pop��pad� jak chybu znovu vyvolat</div>"
      "<textarea cols=\"50\" rows=\"10\" name=\"popis\" style=\"width:100%\"></textarea>"
      "<div>P��padn� p�ilo�te ulo�enou pozici nebo jin� soubory souvisej�c� s chybou</div>"
      "<input name=\"userfile\" type=\"file\" size=\"50\" />"
      "<hr /><div>"
      "Pozor: Formul�� tak� obsahuje informace o posledn� chyb�.<br />"
      "Nepou��vejte tento formul�� k odesl�n� jin�ch chybov�ch hl�en�.</div>"
      "<p><a href=\"http://skeldal.jinak.cz/main.php?page=soukrom�\">Prohl�en� o ochran� soukrom�ch �daj�</a></p>"
      "<hr />"
      "<input type=\"submit\" value=\"Odeslat z�znam o chyb�\" /><br />"
      "<input type=\"hidden\" name=\"data\" value=\"";
    WriteFile(h,(void *)(form1),strlen(form1),&wrt,0);
    HANDLE v=CreateFile(dmp,GENERIC_READ,0,0,OPEN_EXISTING,0,0);
    if (v==INVALID_HANDLE_VALUE)
    {
      MessageBox(0,"Nebylo mozne otevrit soubor se zaznamem o chybe. Soubor je nepristupny",0,MB_OK|MB_SYSTEMMODAL);
      return 0;
    }
    unsigned char buff[256];
    do
    {
      ReadFile(v,buff,128,&rd,0);
      for (int i=127;i>=0;i--)
      {
        char tmp[3];
        sprintf(tmp,"%02X",buff[i]);
        buff[i*2]=tmp[0];
        buff[i*2+1]=tmp[1];
      }
      WriteFile(h,buff,rd*2,&wrt,0);
    }while (rd);
    CloseHandle(v);
    const char *form2="\" /><input type=\"hidden\" name=\"version\" value=\"";
    WriteFile(h,(void *)(form2),strlen(form2),&wrt,0);
    wsprintf((char *)buff,"%d",GetExeVersion());
    WriteFile(h,buff,strlen((char *)buff),&wrt,0);
    const char *form3="\" /><input type=\"hidden\" name=\"product\" value=\"";
    WriteFile(h,(void *)(form3),strlen(form3),&wrt,0);
    WriteFile(h,(void *)(product),strlen(product),&wrt,0);
    const char *form4="\" /></div></form>  <p>"
      "<img "
          "src=\"http://www.w3.org/Icons/valid-xhtml10\" "
          "alt=\"Valid XHTML 1.0!\" height=\"31\" width=\"88\" /></p></div></body></html>";
    WriteFile(h,(void *)(form4),strlen(form4),&wrt,0);
    CloseHandle(h);
  }

  int i=MessageBox(NULL,"Zaznam o chybe dokoncen. Soubory se zaznamy byly ulozeny do slozky hry. "
    "Prosim nemazte tyto soubory, je mozne, ze budou potrebne k hlubsi analyze chyby. "
    "Hlaseni o chybe je nyni mozne odeslat po internetu k dalsimu prozkoumani. "
    "Pokud neni pocitac pripojen k internetu trvale, bude nutne jej pripojit nyni.\r\n\r\n"
    "Chcete zaznam o chybe odeslat?",0,MB_YESNO|MB_SYSTEMMODAL);
  if (i==IDYES)
  {
    if (h==INVALID_HANDLE_VALUE || (UINT)ShellExecute(0,0,buff,0,0,SW_NORMAL)<32)
      MessageBox(0,"Nastala chyba pri otevirani formulare. Formular prosim otevrete rucne - soubor 'poslichybu.html'",0,MB_OK|MB_SYSTEMMODAL);
  }
  else
  {
     MessageBox(0,"Informace o chybe lze odeslat pozdeji pomoci formulare 'poslichybu.html', jenz byl vytvoren ve slozce hry",0,MB_OK|MB_SYSTEMMODAL);
  }
  return 0;
}

static DWORD WINAPI CrashReportGenerateThread(LPVOID data)
{
  MINIDUMP_EXCEPTION_INFORMATION *ExceptionInfo=(MINIDUMP_EXCEPTION_INFORMATION *)data;
  HWND hwnd;
  int res;

  if (MiniDumpWriteDumpFn!=NULL)
  {
	hwnd=CreateWindowEx(WS_EX_TOPMOST,"STATIC","V programu nastala chyba\r\n\r\nSystem Windows nyni sbira informace o chybe a generuje potrebne soubory.\r\nProsim cekejte, tato operace muze trvat trochu dele...",WS_POPUP|
	  WS_DLGFRAME|WS_VISIBLE|SS_CENTER,0,0,640,80,NULL,NULL,GetModuleHandle(NULL),NULL);
	UpdateWindow(hwnd);
    if (GenerateMinidump(ExceptionInfo,MiniDumpNormal,".short.dmp")==false) return 0;
    GenerateMinidump(ExceptionInfo,MiniDumpWithDataSegs,".int32_t.dmp");
    GenerateMinidump(ExceptionInfo,MiniDumpWithFullMemory,".full.dmp");
    PostError(0);
	DestroyWindow(hwnd);
  }
  else
  {
	res=MessageBox(0,"V programu nastala chyba. Bohuzel neni pritomen soubor DbgHelp.dll v adresari hry, "
				"neni tedy mozne vytvorit zaznam o chybe. Ze stranek http://skeldal.jinak.cz je mozne"
				"tento soubor stahnout a tim pomoci autorovi odhalit a opravit tyto zaludne pady.\r\n\r\n"
				"Chcete prejit na stranky obsahujici posledni verze potrebnych souboru?",0,MB_YESNO|MB_SYSTEMMODAL);
	if (res==IDYES)
	  ShellExecute(0,0,"http://skeldal.jinak.cz/main.php?page=download",0,0,SW_NORMAL);
  }
  return 0;
}


static LONG WINAPI CrashReportGenerate(EXCEPTION_POINTERS *ExceptionInfo)
{
  HANDLE msg;

  MINIDUMP_EXCEPTION_INFORMATION nfo;
  nfo.ThreadId=GetCurrentThreadId();
  nfo.ExceptionPointers=ExceptionInfo;
  nfo.ClientPointers=FALSE;

//  CloseAllWindows();

  msg=CreateThread(NULL,0,CrashReportGenerateThread,(void *)&nfo,0,NULL);
  WaitForSingleObject(msg,INFINITE);
  ExitProcess(1);
  return 0;
}

extern "C"
{
  void CrashReportOnBuffOverrun()
  {
    __try
    {
      RaiseException(EXCEPTION_ARRAY_BOUNDS_EXCEEDED,EXCEPTION_NONCONTINUABLE,0,0);
    }
    __except(CrashReportGenerate(GetExceptionInformation()))
    {
    }
  }


extern char __report_gsfailure();
void InitCrashDump()
{
  char *p=(char *)(&__report_gsfailure);
  DWORD oldProtect;
  ::VirtualProtect(p,150,PAGE_EXECUTE_WRITECOPY,&oldProtect);
  *p=0xCC;
  ::VirtualProtect(p,150,PAGE_EXECUTE_READ,&oldProtect);
  SetUnhandledExceptionFilter(CrashReportGenerate);

  HMODULE lib=LoadLibrary("DbgHelp.dll");
  MiniDumpWriteDumpFn=(MiniDumpWriteDump_Type)GetProcAddress(lib,"MiniDumpWriteDump");
}
}