// WPathname.h: interface for the WPathname class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_WPathname_H__158F59D5_B422_4FA6_86AC_10B5EC48C81B__INCLUDED_)
#define AFX_WPathname_H__158F59D5_B422_4FA6_86AC_10B5EC48C81B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "WString.h"

#ifndef ASSERT
#ifdef _DEBUG
#define ASSERT(x) assert(x)
#else
#define ASSERT(x)
#endif
#endif

#define WPathnameCompare(op) bool operator op (const WPathname &other) const \
{if (IsNull() || other.IsNull()) return false;else return wcsicmp(_fullpath,other._fullpath) op 0;}\
  bool operator op (const wchar_t *other) const \
{ASSERT(!other || other[0]!=0);\
  if (IsNull() || other==NULL) return false;else return wcsicmp(_fullpath,other) op 0;}

/** class WPathname simplifying manipulation with WPathnames, filenames, general paths, and
also supports convert from absolute path to relative respectively */



class WPathname  
{
  ///object value and data
  /**The implementation of WPathname creates only one buffer for all variables. It can
  increase speed by effective memory use. Strings are stored one after another separated
  by zero byte. Evry time any string changed, implementation recalculate buffer usage, and
  decide, whether it should resize buffer or not. 

  _fullpath also points to string contain full path with filename, 
  it is dominant value of the class
  */
  WString _fullpath; 
  WString _path;      ///<current path with drive
  WString _filetitle;  ///<file title without extension
  WString _extension;  ///<file extension

public:

  ///Construct WPathname class. 
  /** If name is not provided, current path is used, and as filename, WPathname suppl wildcards
  *.* . That is useful, for searching in directory.<br>
  If name is provided, WPathname will expand it into full name with drive and folder name.
  @param name optional argument to inicialize object
  */
  WPathname(const wchar_t *name=NULL);
  WPathname(const WString &name);

  ///Construct WPathname class
  /** 
  @param relpath Relative path or uncomplette path or single filename with extension. 
  WPathname will expand this WPathname into full using absolute path provided by the second 
  argument.
  @param abspath Absolute path used as reference to folder - the origin of relative path
  provided in the first argument.
  */
  WPathname(const wchar_t *relpath, const WPathname &abspath);

  ///Construct WPathname as copy of another WPathname
  WPathname(const WPathname &other);


  ///Function returns the current drive letter.
  /** Before usage, ensure, that current WPathname contain drive.     
  In network path drive letter is missing.
  In this case, result is undefined. To ensure, use HasDrive function
  @return the drive letter of current path.
  */
  wchar_t GetDrive() const
  {
    if (IsNull()) return 0;
    return _path[0];
  }

  ///Static function determines, if argument contain a drive information. 
  /** 
  @param dir directory to inspect
  @return true, if directory contain drive.<p>
  This function is independed, it don't need any WPathname variable declared.
  */
  static bool HasDrive(const wchar_t *dir) 
  {return (dir[0]>='A' && dir[0]<='Z' || dir[0]>='a' && dir[0]<='z') && dir[1]==':';}

  ///Function determines, if current WPathname contain a drive information
  /**
  @return true, if current WPathname contain a drive information
  */
  bool HasDrive() const 
  {
    if (IsNull()) return false;
    return HasDrive(_path);
  }


  ///Function returns current folder name
  /**
  if current folder name contain drive, only folder name is returned (without drive).
  In other cases (relative or network drives) returns full path.
  @return folder name or full path. Pointer is valid until any first change in object. 
  Do not invoke release function at pointer!
  */
  const wchar_t *GetDirectory() const
  {
    if (HasDrive()) return _path.GetString()+3;
    else return _path.GetString()+IsNetworkPath();
  }

  const wchar_t *GetDirectoryWithDrive() const
  {
    return _path;
  }

  ///Function returns current filename with extension
  /**
  @return current filename with extension. Pointer is valid until any first change in object. 
  Do not invoke release function at pointer!
  */
  const wchar_t *GetFilename() const
  {
    if (IsNull()) return NULL;      
    const wchar_t *blk=wcsrchr(_fullpath,'\\');
    if (blk) blk=blk+1;else blk=_fullpath;
    return blk;
  }

  ///Function returns current extension (with starting dot)
  /**
  @return current extension. Pointer is valid until any first change in object. 
  Do not invoke release function at pointer!
  */
  const wchar_t *GetExtension() const
  {return _extension;}

  ///Function returns current filename without extension (without dot)
  /**
  @return current filename without extension (without dot). Pointer is valid until any first change in object. 
  Do not invoke release function at pointer!
  */
  const wchar_t *GetTitle() const
  {return _filetitle;}

  ///Function changes current drive.
  /**If object contain WPathname with drive, then current drive is changed and function returns.
  If object contain network path, then computer name is changed to the drive name.
  If object contain relative path, then whole path is replaced by path on root on drive.
  @param dr new drive letter. This parameter can be set to zero. It means, that current
  driver is deleted, and path is converted to relative path from root. Note: Zero c
  cannot be used with network paths and relative paths, and finnaly has no effect to the object
  */

  void SetDrive(wchar_t dr);

  ///Sets new directory for object
  /** if object contain a drive letter and argument dir doesn't, then current drive is remain
  and only directory part is replaced. If current path is network path or relative path,
  then whole path is replaced by new one.
  If argument dir contain drive letter, then whole path is replaced too.
  @param dir contain new WPathname. Backslash should be the last wchar_tacter in string
  */
  void SetDirectory(const wchar_t *dir);

  ///Sets new filename for object. 
  /** 
  If filename contain dot, function assumes, that filename is provided with extension.
  Otherwise, current extension remains untouched.
  @param filename new filename for object
  */

  void SetFilename(const wchar_t *filename);

  ///Sets new extension for object.
  /**
  If ext doesn't starting with dot, function adds it.
  @param ext new extension for object
  */
  void SetExtension(const wchar_t *ext);

  ///Sets new file title
  /** Function changes file title, extension remains untouched. 
  if title contains extension (dot inside its name), this extension doesn't change
  current extension. For example, if current extension is ".cpp" and filetitle contain
  "source.h", then result is "source.h.cpp"
  @param title a new title for object.
  */

  void SetFileTitle(const wchar_t *title);

  ///Function returns full WPathname.
  /**
  @return current WPathname. Pointer is valid until any first change in object. 
  Do not invoke release function at pointer!
  */

  const wchar_t *GetFullPath() const 
  {return _fullpath;} 

  ///Sets WPathname
  /** Function has same effect as constructor. But it can be used 
  anytime during object lifetime. It simply replaces current WPathname with newer. WPathname
  in argument is expanded to full WPathname, current directory is used as reference.
  @param WPathname new WPathname
  */
  void SetPathname(const wchar_t *pathname);
  void SetPathname(const WString &pathname);

  WPathname& operator=(const wchar_t *other) 
  {SetPathname(other);return *this;}

  WPathname& operator=(const WString &other) 
  {SetPathname(other);return *this;}

  WPathname& operator=(const WPathname& other)
  {
   _fullpath=other._fullpath; 
   _path=other._path;     
  _filetitle=other._filetitle;
  _extension=other._extension;
  return *this;
  }

  ///converts object to string
  operator const wchar_t *() const
  {return GetFullPath();}

  ///Static function to help getting filename from WPathname
  /** Function finds last backslash / and return pointer to first wchar_tacter after it.
  Pointer stays valid until original path is destroyed or until original path is changed
  @param path WPathname to inspect as string
  @return pointer to filename
  */
  static const wchar_t *GetNameFromPath(const wchar_t *path);

  ///Static function to help getting extension from WPathname
  /** Function finds last dot '.' in filename return pointer to it (extension with dot).
  Pointer stays valid until original path is destroyed or until original path is changed
  @param path WPathname to inspect as string
  @return pointer to extension
  */
  static const wchar_t *GetExtensionFromPath(const wchar_t *path);

  ///Function sets server name for network path
  /** If current path is network path, then changes server name to newer. Otherwise
  it remove drive letter, and insert server name before remain path
  @param server server name without slashes
  */
  void SetServerName(const wchar_t *server);

  ///Function inspects current path and returns, whether contain server name
  /**@return zero, if current path is not valid network path. Nonzero if path contain
  server name. Then value returned count wchar_tacters containing server name with precedent 
  slashes.
  */
  int IsNetworkPath() const;

  ///Function converts current relative path into absolute path
  /** 
  If current path is not relative, function do nothing.
  @param ref reference to path, against which path is relative. 
  @return true if path has been converted, or false, if conversion is impossible
  */
  bool RelativeToFull(const WPathname &ref);

  ///Function converts current absolute path into relative path
  /** 
  If current path is not relative, function do nothing. Both paths must be on the same
  drive or network computer.

  @param ref reference to path, against which path should be relative. 
  @return true if path has been converted, or false, if conversion is impossible
  */
  bool FullToRelative(const WPathname &relativeto);

  WPathname& operator+=(const wchar_t *relativePath) 
  {*this=WPathname(relativePath,*this);return *this;}

  WPathname operator+(const wchar_t *relativePath)
  {WPathname out(relativePath,*this);return out;}

  bool IsNull() const {return _fullpath.GetLength()==0;}

  void SetNull();

    WPathnameCompare(<)
    WPathnameCompare(>)
    WPathnameCompare(==)
    WPathnameCompare(>=)
    WPathnameCompare(<=)
    WPathnameCompare(!=)


    ///Function gets part of WPathname
    /** 
    @param path subject of examine
    @param partnum zero-base index of part of WPathname. Index 0 mostly contain drive or server, in case of
    relative path, there is the name of the first folder or dots.
    @param buff buffer for store result
    @param bufsize count wchar_tacters in buffer;
    @param mode mode=0, gets only name of part. 
    mode=1, get current part and remain parts of path.
    mode=-1, gets all parts till current
    @return Function returns true, if it was succesful, and it was not last part. Function returns
    false, if it was succesful, and it was last part. Function returns false and sets buffer empty,
    if an error occured. Function returns true and sets buffer empty, if buffer is too small to hold data
    */
    static bool GetPartFromPath(const wchar_t *path, int partnum, wchar_t *buff, int bufsize, int mode=0);

  ///Function gets part of object
  /** 
  @param partnum zero-base index of part of WPathname. Index 0 mostly contain drive or server, in case of
  relative path, there is the name of the first folder or dots.
  @param buff buffer for store result
  @param bufsize count wchar_tacters in buffer;
  @param mode mode=0, gets only name of part. 
  mode=1, get current part and remain parts of path.
  mode=-1, gets all parts till current
  @return Function returns true, if it was succesful, and it was not last part. Function returns
  false, if it was succesful, and it was last part. Function returns false and sets buffer empty,
  if an error occured. Function returns true and sets buffer empty, if buffer is too small to hold data
  */
  bool GetPart(int partnum, wchar_t *buff, int bufsize,int mode=0) const
  {
    return GetPartFromPath(this->_fullpath,partnum,buff,bufsize,mode);
  }

  /// Get Directory With Drive Without Last Back Slash
  /** Retrieves into buffer directory with drive and removes last backslash
  @param buff buffer that retrieves path
  @param size size of buffer
  @return true, if success, failed if buffer is too small*/


  bool GetDirectoryWithDriveWLBS(wchar_t *buff, size_t size) const;

  WString GetDirectoryWithDriveWLBS() const;


  /// function checks, if path is valid and returns true, if does.
  bool IsPathValid() const;

  /// Sets special directory. 
  /**
  @param bSpecCode this value may be operation-system
  depend. Windows implementation using CSIDL_XXXX constants, which is described in SHGetSpecialFolderLocation function
  description
  @return true, if function were successful
  */
  bool SetDirectorySpecial(int nSpecCode);

  ///Sets temporaly directory. 
  bool SetTempDirectory();

  ///Guess temporaly file name
  /**
  @param prefix prefix string for name
  @param unique if unique is non-zero, it is used for new temporaly file. If unique is zero, function guess own unique
	  value.
  @return true if function were successful
  */
  bool SetTempFile(const wchar_t *prefix=L"tmp", unsigned int unique=NULL);

  ///Returns path of current executable. 
  /**It useful, when accessing folder, from when current module has been executed */
  static WPathname GetExePath();

  ///Solves most used conversion from fullpath to path relative to project root
  /**
  @param full full WPathname with drive
  @param projectRoot project root path
  @return function returns pointer to full path, where starts relative part. If
    fullpath doesn't contain project root path, it returns pointer to full.
  @example FullToProjectRoot("x:\\project\\data\\example.txt","x:\\project"); //result is "data\\example.txt"
  */

  static const wchar_t *FullToRelativeProjectRoot(const wchar_t *full, const wchar_t *projectRoot);
  

protected:
  ///Function only rebuild _fullpath string.
  /** It doesn't check space for string! This function is used, when length of path is excepted
  the same or smaller, then current.
  */
  void RebuildPath();


};



#endif // !defined(AFX_WPathname_H__158F59D5_B422_4FA6_86AC_10B5EC48C81B__INCLUDED_)
