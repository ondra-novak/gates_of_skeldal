#include <skeldal_win.h>
#include <debug.h>
#include <dos.h>
#include <io.h>
#include <bios.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <conio.h>
#include <malloc.h>
#include <mem.h>
#include <dos.h>
#include <direct.h>
#include <types.h>
#include <bgraph.h>
#include <event.h>
#include <devices.h>
#include <bmouse.h>
#include <memman.h>
#include <zvuk.h>
#include <strlite.h>
#include <gui.h>
#include <basicobj.h>
#include <time.h>
#include "globals.h"
#include <resource.h>
#include <windowsx.h>

void kamenik2windows(const char *src, int size, char *trg);

#define BREAK

static HWND hWizardDlg=NULL;
static HWND hWizardText;
static HFONT hfCourier;

static void wzprintf(const char *text,...)
  {
  int len=GetWindowTextLength(hWizardText);
  char *c=(char *)alloca(len+1024);
  char *d;
  va_list args;

  GetWindowText(hWizardText,c,len+1024);
  if (len>10000) c+=1024;
  d=strchr(c,0);
  va_start(args,text);
  _vsnprintf(d,1020,text,args);
  SetWindowText(hWizardText,c);
  len=strlen(c);
  SendMessage(hWizardText,EM_SETSEL,len,len);
  SendMessage(hWizardText,EM_SCROLLCARET,0,0);
  }

static void wzputs(const char *text)
  {
  wzprintf(text);
  wzprintf("\r\n");
  }

static void wzcls()
  {
  SetWindowText(hWizardText,"");
  }

static LRESULT InputWindow(HWND hDlg,UINT msg, WPARAM wParam,LPARAM lParam)
  {
  static char *buff;
  switch (msg)
	{
	case WM_INITDIALOG:
	  buff=(char *)lParam;
	  SetDlgItemText(hDlg,IDC_PROMPT,buff);
	  hDlg=GetWindow(hDlg,GW_OWNER);
	  EnableWindow(hDlg,TRUE);
	  return 0;
	case WM_COMMAND:
	  switch (LOWORD(wParam))
		{
		case IDCANCEL: EndDialog(hDlg,0);break;
		case IDOK: 
		  GetDlgItemText(hDlg,IDC_VALUE,buff,1020);
		  EndDialog(hDlg,1);
		  break;
		}
	  break;
	default: return 0;
	}
  return 1;
  }

static LRESULT CALLBACK ListWindow(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
  case WM_COMMAND:
    switch (LOWORD(wParam))
    {
    case IDOK:
    case IDCANCEL: PostQuitMessage(LOWORD(wParam));break;
    case IDC_LIST: if (HIWORD(wParam)==LBN_DBLCLK) PostQuitMessage(IDOK);break;
    default: return 0;
    }
  default: return 0;
  }
  return 1;
}

static HWND PrepareListWindow(HWND parent)
{
  HWND res;
  RECT rc1,rc2;
  GetWindowRect(parent,&rc1);
  EnableWindow(parent,0);
  res=CreateDialog(GetModuleHandle(0),MAKEINTRESOURCE(IDD_LISTDIALOG),parent,(DLGPROC)ListWindow);
  GetWindowRect(res,&rc2);
  rc2.right=rc2.right-rc2.left;
  rc2.bottom=rc2.bottom-rc2.top;
  rc2.left=(rc1.left+rc1.right-rc2.right)/2;
  rc2.top=(rc1.top+rc1.bottom-rc2.right)/2;
  SetWindowPos(res,0,rc2.left,rc2.top,rc2.right,rc2.bottom,SWP_NOZORDER);
  ShowWindow(res,SW_SHOW);

  return res;
}

static void CloseListWindow(HWND wnd)
{
  HWND parent=GetParent(wnd);
  DestroyWindow(wnd);
  EnableWindow(parent,1);
}

static int PumpDialogMessages(HWND dlg)
{
  MSG msg;
  while (GetMessage(&msg,0,0,0))
  {
    if (!IsDialogMessage(dlg,&msg)) 
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
  return msg.wParam;
}


static int wzscanf(const char *prompt, const char *format,...)
  {
  char buff[1024];
  va_list args;
  unsigned long data[10];
  int i;

  static char notallowed=0;

  if (notallowed) return 0;
  notallowed=1;

  strcpy(buff,prompt);
  if (DialogBoxParam(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_INPUTWINDOW),GetActiveWindow(),(DLGPROC)InputWindow,(LPARAM)buff)==0) 
	{
	notallowed=0;
	return 0;
	}

  notallowed=0;

  va_start(args,format);
  for (i=0;i<10;i++) data[i]=va_arg(args,unsigned long);
  return sscanf(buff,format,data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9]);  
  }

char *side_flags[]=
  {
  "AUTOMAP",
  "!PLR",
  "!MONST",
  "!THING",
  "!SOUND",
  "ALARM",
  "PASS",
  "TRANSPARENT",
  "P_ANIM",
  "P_VIS",
  "P_PING",
  "P_FORW",
  "S_ANIM",
  "S_VIS",
  "S_PING",
  "S_FORW",
  "LEFT_A",
  "RIGHT_A",
  "ALT_SIDES",
  "SPEC",
  "COPY",
  "SEND",
  "APLY2ND",
  "AUTOANIM",
  "",
  "",
  "",
  "",
  "",
  "SECRET",
  "TRUESEE",
  "INVIS"
  };

char *obl_flags[]=
  {
  "RECESS",
  "UP_SIDE",
  "DOWN_SIDE",
  "ITPUSH"
  };

char *mc_flags[]=
  {
  "MAPPED",
  "FLY_OBJECT",
  "PLAYER",
  "DEAD_PLAYER",
  "SPECTXTR",
  "SAFEPLACE",
  "UNUSED",
  "MARKED!",
  "SHADED",
  "STAIRS",
  "DOWN",
  "!AUTOMAP",
  "!SUMMON"
  };

void mman_scan(int action)
  {
  extern char screenstate;
  static pos=0,zavora=0;
  MEMORYSTATUS mmi;
  char c[10];
  if (screenstate && !zavora)
     {
     zavora=1;
     switch(action)
        {
        case MMA_SWAP:curcolor=RGB555(31,0,0);break;
        case MMA_READ:curcolor=RGB555(0,31,0);break;
        case MMA_SWAP_READ:curcolor=RGB555(0,0,31);break;
        case MMA_FREE:curcolor=RGB555(31,31,31);break;
        }
     bar(pos,0,pos+16,16);
     get_mem_info(&mmi);
     set_font(H_FBOLD,RGB555(31,31,31));
     curcolor=0;
     bar(16,0,66,16);
     position(16,0);sprintf(c,"%d",mmi.dwAvailPageFile/1024);
     outtext(c);
     showview(pos,0,66,16);
     zavora=0;
     }
  }

void show_flags(int number,char **flags,char nums)
  {
  int i=0;
  while (nums--)
     {
     if (number & 1) wzprintf("%s ",flags[i]);
     i++;
     number>>=1;
     }
  }

void spell_group_invis()
  {
  int i;
  char ok=1;

  for(i=0;i<POCET_POSTAV;i++) if (postavy[i].sektor==viewsector && !(postavy[i].stare_vls[VLS_KOUZLA] & SPL_INVIS))
        {
        postavy[i].stare_vls[VLS_KOUZLA]|=SPL_INVIS;
        ok=0;
        }
  if (ok)
  for(i=0;i<POCET_POSTAV;i++) if (postavy[i].sektor==viewsector)
        {
        postavy[i].stare_vls[VLS_KOUZLA]&=~SPL_INVIS;
        }
  for(i=0;i<POCET_POSTAV;i++) prepocitat_postavu(postavy+i);
  build_all_players();
  }



static void advence_player(int player,int level,char auto_advance)
  {
  THUMAN *h;
  float mh,mv;

  if (level<2) return;
  h=postavy+player;
  mh=(float)human_selected->jidlo/MAX_HLAD(human_selected);
  mv=(float)human_selected->voda/MAX_ZIZEN(human_selected);
  human_selected=h;
  h->exp=level_map[level-2];
  check_player_new_level(h);
  if (auto_advance)
     {
     int vlssuma=h->vlastnosti[VLS_SILA]+
                 h->vlastnosti[VLS_OBRAT]+
                 h->vlastnosti[VLS_POHYB]+
                 h->vlastnosti[VLS_SMAGIE];
     int b,i;

     for(i=0;i<4;i++)
        {
        b=h->vlastnosti[i]*h->bonus/vlssuma;
        h->bonus-=b;vlssuma-=h->vlastnosti[i];
        while (b--) advance_vls(i);
        }
     prepocitat_postavu(human_selected);
     }
  human_selected->jidlo=(int)(mh*MAX_HLAD(human_selected));
  human_selected->voda=(int)(mv*MAX_ZIZEN(human_selected));
  wzprintf("%s ziskal%s uroven cislo %d\r\n",h->jmeno,h->female?"a":"",level);
  }


extern char folow_mode;
extern char folow_mob;
void macro_drop_item();

static char take_money()
  {
  int i;
  if (!wzscanf("Kolik: (0 - zrusit):","%d",&i)) return 0;
  money+=i;
  if (i)
     {
     SEND_LOG("(WIZARD) Take Money %d, total %d",i,money);
     }
  return (i!=0);
  }

#define ALL "ALL"
static char purge_map()
  {
  char buffer[200];
  char *c;

  STOP();
/*  struct find_t rc;
  int rs;

  concat(c,pathtable[SR_TEMP],"*.TMP");
  rs=_dos_findfirst(c,_A_NORMAL,&rc);
  while (rs==0)
     {
     if (rc.name[0]!='~') wzputs(rc.name);
     rs=_dos_findnext(&rc);
     }
  _dos_findclose(&rc);*/
  wzprintf("\r\n Zadej jmeno tempu (all - vse):");gets(buffer);
  if (buffer[0]==0) return 0;
  strupr(buffer);
  concat(c,pathtable[SR_TEMP],buffer);
  if (strcmp(buffer,ALL) && access(c,0))
     {
     wzputs("Soubor nenalezen!");
     return 0;
     }
  SEND_LOG("(WIZARD) Purge Map: '%s'",buffer,0);
  if (!strcmp(buffer,ALL)) purge_temps(0);
  else remove(c);
  return 1;
  }

static char heal_meditate(void)
  {
  int a,b,i;
  THUMAN *p;

  if (!wzscanf("Obnovit postavu c: (0 - vsechny, -1 - zrusit):","%d",&b)) return 0;
  if (b==-1) return 0;
  if (b) a=b-1;else a=0,b=POCET_POSTAV;
  p=postavy+a;
  for(i=a;i<b;i++,p++) if (p->used && p->lives)
     {
     p->lives=p->vlastnosti[VLS_MAXHIT];
     p->mana=p->vlastnosti[VLS_MAXMANA];
     p->kondice=p->vlastnosti[VLS_KONDIC];
     p->jidlo=MAX_HLAD(p);
     p->voda=MAX_ZIZEN(p);
     SEND_LOG("(WIZARD) Restoring character '%s'",p->jmeno,0);
     bott_draw(1);
     }
  return 1;
  }

static char raise_death(void)
  {
  int b;
  THUMAN *p;
  char *c,*d;

  if (!wzscanf("Obzivit postavu c: (0 a -1 - zrusit):","%d",&b)) return 0;
  b--;
  if (b<0) return 0;
  p=postavy+b;
  p->lives=p->vlastnosti[VLS_MAXHIT];
  p->mana=p->vlastnosti[VLS_MAXMANA];
  p->kondice=p->vlastnosti[VLS_KONDIC];
  c="(WIZARD) '%s' has been returned to game by gods power!";d=strchr(c,'\'');
  wzprintf(d,p->jmeno);putchar('\r\n');
  bott_draw(1);
  return 0;
  }

  static char raise_killed_monster(HWND hDlg)
  {
    HWND listdlg=PrepareListWindow(hDlg);
    HWND list=GetDlgItem(listdlg,IDC_LIST);
    char buff[256];
    int i;
    int res;

    for (i=0;i<MAX_MOBS;i++) if (~mobs[i].vlajky & MOB_LIVE && mobs[i].cislo_vzoru!=0) 
    {
      int p;
      _snprintf(buff,sizeof(buff),"%4d. %s (sector: %d home %d)",i,mobs[i].name,mobs[i].sector,mobs[i].home_pos);
      kamenik2windows(buff,strlen(buff),buff);
      p=ListBox_AddString(list,buff);
      ListBox_SetItemData(list,p,i);
    }
    res=PumpDialogMessages(listdlg);
    while (res==IDOK)
    {
      int cnt;
      for (i=0,cnt=ListBox_GetCount(list);i<cnt;i++) if (ListBox_GetSel(list,i))
      {
        int idx=ListBox_GetItemData(list,i);
        mobs[idx].vlajky|=MOB_LIVE;
        mobs[idx].lives=mobs[idx].vlastnosti[VLS_MAXHIT];
        wzprintf("%s znovu povstal(a)\r\n",mobs[idx].name);
        SEND_LOG("(WIZARD) '%s' has been raised",mobs[idx].name,0);
      }
      res=PumpDialogMessages(listdlg);
    }
    CloseListWindow(listdlg);
    return 1;
  }

void unaffect();
extern char immortality;
extern char nohassle;

static char set_immortality()
  {
  immortality=!immortality;
  SEND_LOG("(WIZARD) Immortality has been turned %s.",immortality?"on":"off",0);
  wzprintf("Nesmrtelnost byla %s.\r\n",immortality?"zapnuta":"vypnuta");
  return 0;
  }

static char set_nohassle()
  {
  nohassle=!nohassle;
  SEND_LOG("(WIZARD) Nohassle has been turned %s.",nohassle?"on":"off",0);
  wzprintf("Nevycititelnost byla %s.\r\n",nohassle?"zapnuta":"vypnuta");
  return 0;
  }


static char advance_weapon()
  {
  int p,i;
  char buff[128];
  THUMAN *h;
  if (!wzscanf("Cislo postavy: (0 = Zpet)","%d",&p)) return 0;
  if (p==0) return 0;
  h=postavy+p-1;
  do
     {
     int bonus, value;
     for(i=0;i<TPW_MAX;i++) wzprintf("%d. %-15s: %2d Exp %5d\r\n",i+1,texty[91+i],h->bonus_zbrani[i],h->weapon_expy[i]);
     if (!wzscanf("<Zbran> <Hodnota>","%[^\n]",buff)) return 0;
     if (buff[0]==0) return 0;
     if (sscanf(buff,"%d %d",&bonus,&value)!=2) wzputs("Huh?!");
     else
        {
        bonus--;
        if (bonus<0 || bonus>=TPW_MAX) wzputs("Spatna zbran");
        else
           if (value<0 || value>=10) wzputs("Spatna hodnota");
              else
                 h->bonus_zbrani[bonus]=value;
        }
     }
  while(1);
  }

static reload_mobs()
  {
  extern char reset_mobiles;
  reset_mobiles=1;
  strncpy(loadlevel.name,level_fname,12);
  loadlevel.start_pos=viewsector;
  loadlevel.name[12]=0;
  loadlevel.dir=viewdir;
  send_message(E_CLOSE_MAP);
  }

static char display_game_status(void)
  {
  short *v;
  THUMAN *p;
  TSTENA *s;
  TSECTOR *ss;
  register i,cn,astr;

  wzcls();
  SEND_LOG("(WIZARD) Starting wizard window at Sect %d Side %d",viewsector,viewdir);
  wzprintf("Sektor: %5d  Smer: %d  Skupina %d \r\n",viewsector,viewdir,cur_group);
  for(i=0,p=postavy;i<POCET_POSTAV;i++,p++)
     if (p->used)
        wzprintf("%d.%-14s (%d) Sek:%5d Smr:%d HPReg:%d MPReg:%d VPReg:%d %04X%s\r\n",i+1,p->jmeno,p->groupnum,p->sektor,p->direction,p->vlastnosti[VLS_HPREG],
              p->vlastnosti[VLS_MPREG], p->vlastnosti[VLS_VPREG], p->vlastnosti[VLS_KOUZLA], p->lives?"":"(smrt)");
     else
        wzprintf("%d. (nepouzito)\r\n",i);
  wzputs("");
  wzprintf("Predmet(y) v mysi: ");
  v=picked_item;
  if (v==NULL) wzprintf("<zadne>");else while(*v) wzprintf("%d ",abs(*v++)-1);
  wzputs("\r\n");
  for(i=0,cn=0,astr=0;i<MAX_MOBS;i++)
     {
     if (mobs[i].vlajky & MOB_LIVE) cn++;
     if (mobs[i].vlajky & MOB_MOBILE) astr++;
     }
  wzprintf("Celkem potvor ve hre:  %5d (+%d) astral mobiles\r\n"
         "Celkem predmetu ve hre:%5d\r\n"
         " .. z toho klonu:      %5d\r\n",cn-astr,astr,item_count,item_count-it_count_orgn);

  wzputs("");
  ss=map_sectors+viewsector;
  s=map_sides+viewsector*4+viewdir;
  wzprintf("Sector: (%d) Podlaha %d Strop %d Cil akce %d Smer akce %d Akce %d\r\n",
         ss->sector_type, ss->floor,ss->ceil,ss->sector_tag,ss->side_tag,ss->action);
  wzprintf("        Vychody: Sev %d Vych %d Jih %d Z p %d\r\n",ss->step_next[0],ss->step_next[1],ss->step_next[2],ss->step_next[3]);
  wzprintf("        Vlajky: %02X %02X ",ss->flags,map_coord[viewsector].flags);show_flags(map_coord[viewsector].flags,mc_flags,12);
  wzputs("\r\n");
  wzprintf("Stena: Prim %d Sec %d Obl %d Anim_prim %d/%d Anim_sec %d/%d\r\n",
         s->prim,s->sec,s->oblouk & 0xf,s->prim_anim>>4,s->prim_anim & 0xf,s->sec_anim>>4,s->sec_anim & 0xf);
  wzprintf("       Cil akce %d Smer akce %d Akce %d\r\n",s->action,s->sector_tag,s->side_tag & 0x3);
  wzprintf("       Multiakce: %s\r\n",macros[viewsector*4+viewdir]==NULL?"<zadna>":"Existuje");
  wzprintf("       Vlajky: %04X %02X %02X ",s->flags,s->oblouk>>4,s->side_tag>>2);
  wzputs("");
  show_flags(s->flags,side_flags,32);
  show_flags(s->oblouk>>4,obl_flags,4);
  return 0;
  }

static LRESULT WizardDlgProc(HWND hDlg,UINT msg, WPARAM wParam,LPARAM lParam)
  {
  int i;
  switch (msg)
	{
	case WM_INITDIALOG:
	  hWizardDlg=hDlg;
	  hWizardText=GetDlgItem(hDlg,IDC_OUTPUT);
	  SendMessage(hWizardText,WM_SETFONT,(WPARAM)hfCourier,1);
	  display_game_status();
	  SetTimer(hDlg,10,20,NULL);
	  return 0;	
	case WM_TIMER: do_events();return 1;
	case WM_COMMAND:
	  switch (LOWORD(wParam))
		{
	 case IDCANCEL: EndDialog(hDlg,0);return 0;
     case IDC_CLEARMAP:
       {
          HWND listwnd=PrepareListWindow(hDlg);
          HWND list=GetDlgItem(listwnd,IDC_LIST);
          int res;
          ListBox_AddString(list,"Clear Monsters");
          ListBox_AddString(list,"Clear Items");
          res=PumpDialogMessages(listwnd);
          if (res==IDOK)
          {
            if (ListBox_GetSel(list,0))
            {
              for(i=0;i<MAX_MOBS;i++)
                          if (mobs[i].vlajky & MOB_LIVE)
                            {
                            vybrana_zbran=-1;
                            select_player=-1;
                            mob_hit(mobs+i,mobs[i].lives);
                            }
            }
            if (ListBox_GetSel(list,1))
            {
              for(i=0;i<mapsize*4;i++)
                              {
                              destroy_items(map_items[i]);
                              free(map_items[i]);
                              map_items[i]=NULL;
                              }
                           for(i=0;i<vyk_max;i++)
                              {
                              destroy_items(map_vyk[i].items);
                              map_vyk[i].items[0]=0;
                              }
            }
          }
          CloseListWindow(listwnd);          
       }
       break;
       
     case IDC_ADVENCE:
              {
              int i,j,c;
              if (!wzscanf("Advence to level <postava -1=vsichni><uroven>:","%d %d",&i,&j)) return 0;
              c=MessageBox(GetActiveWindow(),"Automaticky?","?",MB_YESNO|MB_ICONQUESTION);
              if (i>0) advence_player(i-1,j,c==IDYES);else
                 for(i=0;i<POCET_POSTAV;i++) if (postavy[i].used) advence_player(i,j,c==IDYES);              
              return 0;
              }
     case IDC_GOTO:
			  {
			  char prompt[50];
              sprintf(prompt,"Goto sector <1-%d>:",mapsize-1);
			  if (!wzscanf(prompt,"%d",&viewsector)) return 0;
              chod_s_postavama(1);              
              SEND_LOG("(WIZARD) Goto %d",viewsector,0);
              return 0;
			  }
     case IDC_LOADMAP:
              if (!wzscanf("Load Map <filename><sector>","%s %hd",loadlevel.name,&loadlevel.start_pos)) return 0;
              for(i=0;i<POCET_POSTAV;i++)postavy[i].sektor=loadlevel.start_pos;
              SEND_LOG("(WIZARD) Load map '%s' %d",loadlevel.name,loadlevel.start_pos);
			  EndDialog(hDlg,0);
              send_message(E_CLOSE_MAP);
              return 0;
     case IDC_OPENDOOR:if (map_sectors[viewsector].step_next[viewdir])
                delay_action(3,viewsector,viewdir,0x2000000,0,0);
              else
                delay_action(3,viewsector,viewdir,0,0,0);
              return 0;
     case IDC_TAKEMONEY:if (take_money()) return 0;break;
     case IDC_PURGE:if (purge_map()) return 0;break;
     case IDC_HEAL:if (heal_meditate()) return 0;break;
     case IDC_RAISEDEATH:if (raise_death()) return 0;break;
     case IDC_RAISEMONSTER:if (raise_killed_monster(hDlg)) return 0;break;
     case IDC_IMMORTAL:set_immortality();break;
     case IDC_NETECNOST:set_nohassle();break;
     case IDC_UNAFFECT :unaffect();break;
     case IDC_WEAPONSKILL:if (advance_weapon()) return 0;break;
     case IDC_REFRESH:display_game_status();break;
     case IDC_RELOADMOBILES:
			  i=MessageBox(hDlg,"Tato funkce precte znova parametry vsech existujicich nestvur. "
                      "Pouzivej jen v pripade, ze se tyto parametry zmenili a nesouhlasi tak "
                      "obsah ulozene pozice. Pokracovat? ","??",MB_YESNO|MB_ICONQUESTION);
              if (i==IDYES) reload_mobs();
              return 0;
     case IDC_LOADITEM:
                       {
                         HWND listdlg=PrepareListWindow(hDlg);
                         HWND list=GetDlgItem(listdlg,IDC_LIST);
                         char buff[256];
                         int i;
                         int res;

                         for (i=0;i<item_count;i++)
                         {
                           _snprintf(buff,sizeof(buff),"%d. %s",i,glob_items[i].jmeno);
                           kamenik2windows(buff,strlen(buff),buff);
                           ListBox_AddString(list,buff);
                         }
                         res=PumpDialogMessages(listdlg);
                         while (res==IDOK)
                         {
                           int cnt;
                           for (i=0,cnt=ListBox_GetCount(list);i<cnt;i++) if (ListBox_GetSel(list,i))
                           {
                             SEND_LOG("(WIZARD) Load Item %d (%s)",i,glob_items[i].jmeno);
                             macro_drop_item(viewsector,viewdir,i);                             
                             wzprintf("Dropped item: %d\r\n",i);
                           }
                           res=PumpDialogMessages(listdlg);
                         }
                         CloseListWindow(listdlg);
                       }
                       return 0;
		}
			
	 default: return 0;
     }
return 1;
}

static void OpenWizard()
  {
  hfCourier=CreateFont(15,0,0,0,0,0,0,0,0,0,0,0,0,"Courier");
  DxDialogs(1);
  DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_LADICIOKNO),GetActiveWindow(),(DLGPROC)WizardDlgProc);
  DxDialogs(0);
  DeleteObject(hfCourier);
  }

void wizard_kbd(EVENT_MSG *msg,void **usr)
  {
  char c;

  if (map_with_password && debug_enabled!=266859) return;
  usr;
  if (msg->msg==E_KEYBOARD)
     {
     c=(*(int *)msg->data)>>8;
     msg->msg=-1;
     switch (c)
        {
        case 'C':
        case 'D':
		   unwire_proc();
		   OpenWizard();
		   wire_proc();
           showview(0,0,0,0);
           break;
        case '<':show_debug=!show_debug;break;
        case '=':show_lives=!show_lives;break;
        case '>':if (mman_action!=NULL) mman_action=NULL;else mman_action=mman_scan;break;
        case '@':set_immortality();set_nohassle();break;
        case 'A':bott_draw_fletna();break;
        case 'B':wire_global_map();break;
        case '?':cur_group=10;break;/*folow_mode=!folow_mode;
                 if (folow_mode) folow_mob=mob_map[map_sectors[viewsector].step_next[viewdir]]-1;
                 else for(c=0;c<POCET_POSTAV;c++) if (postavy[c].groupnum==cur_group) viewsector=postavy[c].sektor;
                 if (folow_mob==255) folow_mode=0;
                 */
                 break;
          
        default:
              msg->msg=E_KEYBOARD;break;

        }
     }
  return;
  }

void install_wizard()
  {
  send_message(E_ADD,E_KEYBOARD,wizard_kbd);
  }



