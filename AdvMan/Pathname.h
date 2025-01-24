// Pathname.h: interface for the Pathname class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_Pathname_H__40F41C23_3AA2_486C_B9E5_33AEE67FB313__INCLUDED_)
#define AFX_Pathname_H__40F41C23_3AA2_486C_B9E5_33AEE67FB313__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <ctype.h>
#include <string.h>
#include <assert.h>

#ifndef ASSERT
#ifdef _DEBUG
#define ASSERT(x) assert(x)
#else 
#define ASSERT(x)
#endif
#endif

enum PathNameNullEnum {PathNull};

#define PathNameCompare(op) bool operator op (const Pathname &other) const \
{if (IsNull() || other.IsNull()) return false;else return stricmp(_fullpath,other._fullpath) op 0;}\
  bool operator op (const char *other) const \
{ASSERT(other[0]!=0);\
  if (IsNull() || other==NULL) return false;else return stricmp(_fullpath,other) op 0;}

#ifndef _UNICODE

/** class Pathname simplifying manipulation with pathnames, filenames, general paths, and
also supports convert from absolute path to relative respectively */



class Pathname  
{
  ///object value and data
  /**The implementation of Pathname creates only one buffer for all variables. It can
  increase speed by effective memory use. Strings are stored one after another separated
  by zero byte. Evry time any string changed, implementation recalculate buffer usage, and
  decide, whether it should resize buffer or not. 

  _fullpath also points to string contain full path with filename, 
  it is dominant value of the class
  */
  char *_fullpath; 
  char *_path;      ///<current path with drive
  char *_filetitle;  ///<file title without extension
  char *_extension;  ///<file extension
  char *_end;        ///<end of field. Class must know, where buffer ends

public:

  ///Construct Pathname class. 
  /** If name is not provided, current path is used, and as filename, Pathname suppl wildcards
  *.* . That is useful, for searching in directory.<br>
  If name is provided, Pathname will expand it into full name with drive and folder name.
  @param name optional argument to inicialize object
  */
  Pathname(const char *name=NULL);

  ///Construct Pathname class
  /** Using this constructor Pathname(PathNull) will create Pathname class with null content is set. 
  If null content is set, all string-query function returns NULL. IsNull function returns true. This state
  is sets until new path is assigned. There is a set of functions invalid called in null state.
  */

  Pathname(PathNameNullEnum null);

  ///Construct Pathname class
  /** 
  @param relpath Relative path or uncomplette path or single filename with extension. 
  Pathname will expand this pathname into full using absolute path provided by the second 
  argument.
  @param abspath Absolute path used as reference to folder - the origin of relative path
  provided in the first argument.
  */
  Pathname(const char *relpath, const Pathname &abspath);

  ///Construct Pathname as copy of another pathname
  Pathname(const Pathname &other);

  ///Destruct Pathname
  virtual ~Pathname();

  ///Function returns the current drive letter.
  /** Before usage, ensure, that current pathname contain drive.     
  In network path drive letter is missing.
  In this case, result is undefined. To ensure, use HasDrive function
  @return the drive letter of current path.
  */
  char GetDrive() const
  {
    if (IsNull()) return 0;
    return _path[0];
  }

  ///Static function determines, if argument contain a drive information. 
  /** 
  @param dir directory to inspect
  @return true, if directory contain drive.<p>
  This function is independed, it don't need any Pathname variable declared.
  */
  static bool HasDrive(const char *dir) 
  {return (isalpha(dir[0]) && dir[1]==':');}

  ///Function determines, if current pathname contain a drive information
  /**
  @return true, if current pathname contain a drive information
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
  const char *GetDirectory() const
  {
    if (HasDrive()) return _path+3;
    else return _path;
  }

  const char *GetDirectoryWithDrive() const
  {
    return _path;
  }

  ///Function returns current filename with extension
  /**
  @return current filename with extension. Pointer is valid until any first change in object. 
  Do not invoke release function at pointer!
  */
  const char *GetFilename() const
  {
    if (IsNull()) return NULL;      
    const char *blk=strrchr(_fullpath,'\\');
    if (blk) blk=blk+1;else blk=_fullpath;
    return blk;
  }

  ///Function returns current extension (with starting dot)
  /**
  @return current extension. Pointer is valid until any first change in object. 
  Do not invoke release function at pointer!
  */
  const char *GetExtension() const
  {return _extension;}

  ///Function returns current filename without extension (without dot)
  /**
  @return current filename without extension (without dot). Pointer is valid until any first change in object. 
  Do not invoke release function at pointer!
  */
  const char *GetTitle() const
  {return _filetitle;}

  ///Function changes current drive.
  /**If object contain pathname with drive, then current drive is changed and function returns.
  If object contain network path, then computer name is changed to the drive name.
  If object contain relative path, then whole path is replaced by path on root on drive.
  @param dr new drive letter. This parameter can be set to zero. It means, that current
  driver is deleted, and path is converted to relative path from root. Note: Zero c
  cannot be used with network paths and relative paths, and finnaly has no effect to the object
  */

  void SetDrive(const char dr);

  ///Sets new directory for object
  /** if object contain a drive letter and argument dir doesn't, then current drive is remain
  and only directory part is replaced. If current path is network path or relative path,
  then whole path is replaced by new one.
  If argument dir contain drive letter, then whole path is replaced too.
  @param dir contain new pathname. Backslash should be the last character in string
  */
  void SetDirectory(const char *dir);

  ///Sets new filename for object. 
  /** 
  If filename contain dot, function assumes, that filename is provided with extension.
  Otherwise, current extension remains untouched.
  @param filename new filename for object
  */

  void SetFilename(const char *filename);

  ///Sets new extension for object.
  /**
  If ext doesn't starting with dot, function adds it.
  @param ext new extension for object
  */
  void SetExtension(const char *ext);

  ///Sets new file title
  /** Function changes file title, extension remains untouched. 
  if title contains extension (dot inside its name), this extension doesn't change
  current extension. For example, if current extension is ".cpp" and filetitle contain
  "source.h", then result is "source.h.cpp"
  @param title a new title for object.
  */

  void SetFiletitle(const char *title);

  ///Function returns full pathname.
  /**
  @return current pathname. Pointer is valid until any first change in object. 
  Do not invoke release function at pointer!
  */

  const char *GetFullPath() const 
  {return _fullpath;} 

  ///Sets pathname
  /** Function has same effect as constructor. But it can be used 
  anytime during object lifetime. It simply replaces current pathname with newer. Pathname
  in argument is expanded to full pathname, current directory is used as reference.
  @param pathname new pathname
  */
  void SetPathName(const char *pathname);

  Pathname& operator=(const char *other) 
  {SetPathName(other);return *this;}
  Pathname& operator=(const Pathname& other);

  ///converts object to string
  operator const char *() const
  {return GetFullPath();}

  ///Static function to help getting filename from pathname
  /** Function finds last backslash / and return pointer to first character after it.
  Pointer stays valid until original path is destroyed or until original path is changed
  @param path pathname to inspect as string
  @return pointer to filename
  */
  static const char *GetNameFromPath(const char *path);

  ///Static function to help getting extension from pathname
  /** Function finds last dot '.' in filename return pointer to it (extension with dot).
  Pointer stays valid until original path is destroyed or until original path is changed
  @param path pathname to inspect as string
  @return pointer to extension
  */
  static const char *GetExtensionFromPath(const char *path);

  ///Function sets server name for network path
  /** If current path is network path, then changes server name to newer. Otherwise
  it remove drive letter, and insert server name before remain path
  @param server server name without slashes
  */
  void SetServerName(const char *server);

  ///Function inspects current path and returns, whether contain server name
  /**@return zero, if current path is not valid network path. Nonzero if path contain
  server name. Then value returned count characters containing server name with precedent 
  slashes.
  */
  int IsNetworkPath() const;

  ///Function converts current relative path into absolute path
  /** 
  If current path is not relative, function do nothing.
  @param ref reference to path, against which path is relative. 
  @return true if path has been converted, or false, if conversion is impossible
  */
  bool RelativeToFull(const Pathname &ref);

  ///Function converts current absolute path into relative path
  /** 
  If current path is not relative, function do nothing. Both paths must be on the same
  drive or network computer.

  @param ref reference to path, against which path should be relative. 
  @return true if path has been converted, or false, if conversion is impossible
  */
  bool FullToRelative(const Pathname &relativeto);

  Pathname& operator+=(const char *relativePath) 
  {*this=Pathname(relativePath,*this);return *this;}

  Pathname operator+(const char *relativePath)
  {Pathname out(relativePath,*this);return out;}

  bool IsNull() const {return _fullpath==NULL;}

  void SetNull();

  PathNameCompare(<)
    PathNameCompare(>)
    PathNameCompare(==)
    PathNameCompare(>=)
    PathNameCompare(<=)
    PathNameCompare(!=)


    ///Function gets part of pathname
    /** 
    @param path subject of examine
    @param partnum zero-base index of part of pathname. Index 0 mostly contain drive or server, in case of
    relative path, there is the name of the first folder or dots.
    @param buff buffer for store result
    @param bufsize count characters in buffer;
    @param mode mode=0, gets only name of part. 
    mode=1, get current part and remain parts of path.
    mode=-1, gets all parts till current
    @return Function returns true, if it was succesful, and it was not last part. Function returns
    false, if it was succesful, and it was last part. Function returns false and sets buffer empty,
    if an error occured. Function returns true and sets buffer empty, if buffer is too small to hold data
    */
    static bool GetPartFromPath(const char *path, int partnum, char *buff, int bufsize, int mode=0);

  ///Function gets part of object
  /** 
  @param partnum zero-base index of part of pathname. Index 0 mostly contain drive or server, in case of
  relative path, there is the name of the first folder or dots.
  @param buff buffer for store result
  @param bufsize count characters in buffer;
  @param mode mode=0, gets only name of part. 
  mode=1, get current part and remain parts of path.
  mode=-1, gets all parts till current
  @return Function returns true, if it was succesful, and it was not last part. Function returns
  false, if it was succesful, and it was last part. Function returns false and sets buffer empty,
  if an error occured. Function returns true and sets buffer empty, if buffer is too small to hold data
  */
  bool GetPart(int partnum, char *buff, int bufsize,int mode=0) const
  {
    return GetPartFromPath(this->_fullpath,partnum,buff,bufsize,mode);
  }

  /// Get Directory With Drive Without Last Back Slash
  /** Retrieves into buffer directory with drive and removes last backslash
  @param buff buffer that retrieves path
  @param size size of buffer
  @return true, if success, failed if buffer is too small*/


  bool GetDirectoryWithDriveWLBS(char *buff, size_t size) const;


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
  bool SetTempFile(const char *prefix="tmp", unsigned int unique=NULL);

  ///Returns path of current executable. 
  /**It useful, when accessing folder, from when current module has been executed */
  static Pathname GetExePath();

  ///Solves most used conversion from fullpath to path relative to project root
  /**
  @param full full pathname with drive
  @param projectRoot project root path
  @return function returns pointer to full path, where starts relative part. If
    fullpath doesn't contain project root path, it returns pointer to full.
  @example FullToProjectRoot("x:\\project\\data\\example.txt","x:\\project"); //result is "data\\example.txt"
  */

  static const char *FullToRelativeProjectRoot(const char *full, const char *projectRoot);

  ///Creates folder from path
  static bool CreateFolder(const char *path, void *security_descriptor=0);
  
  ///Creates all folders from path stored in object
  /**
  Function creates path stored in object. Function creates whole path, when it doesn't
  exists. 
  @param security_descriptor pointer for additional information about security on new folder
    if this parameter is NULL, default descriptor is used. This parameter is platform
    depended.
  @return if path has been created, returns true. In case of error, returns false
    and no changes are made on disk (Function rollbacks any changes)
    */
  bool CreateFolder(void *security_descriptor=0);
  

  enum DeleteFolderFlags {
    DFSimple=0, //only deletes latest folder
    DFPath=1, //deletes whole path, if there are no files
    DFFile=2, //deletes file specified in object. You can use wildcards
    DFRecursive=4, //also deletes all folders inside the path
/*    DFRecycleBin=8, //move all deleted files or folders into recycle bin
    DFShowProgress=16, //enables progress bar during deleting*/
  };

  ///Deletes folder stored in object
  /**
  @param dfFlags combination of flags.
  @return function returns true, when no error occured. Function return false,
    when error occured. 
    */
  bool DeleteFolder(int dfFlags);
  

protected:
  ///Function rebuild buffer with new values
  /** This protected function recalculates space for buffer, allocated it, and rebuild its
  content with data supplied by arguments. Function doesn't assumes, that all strings are 
  terminated by zero by default. "pathlen, titlelen and  extlen" must contain correct values. 
  Terminating zero is not included, but function excepting it.
  Valid using is: RebuildData(a,b,c,strlen(a),strlen(b),strlen(c));
  All pointers returned by Get functions can be used and stays valid, until this function returns.
  */
  void RebuildData(const char *path, const char *filetitle, const char *extension, int pathlen, int titlelen, int extlen);
  ///Function only rebuild _fullpath string.
  /** It doesn't check space for string! This function is used, when length of path is excepted
  the same or smaller, then current.
  */
  void RebuildPath();


};


#else

#error unicode version of Pathname is not currently supported

#endif

#undef PathNameCompare

#endif // !defined(AFX_Pathname_H__40F41C23_3AA2_486C_B9E5_33AEE67FB313__INCLUDED_)
