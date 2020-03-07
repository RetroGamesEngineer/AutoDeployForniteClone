// PidlFromFileSpec.cpp				utility functions for CmdUtils
// created 5/27/2000				magi@cs.stanford.edu


/*  COPYLEFT NOTICE:
 *  This source file is one piece of CmdUtils.  CmdUtils source code
 *  has been released by its author as free software under the GPL.
 *  For updates and the complete collection, visit:
 *      http://www.maddogsw.com/
 *
 *  CmdUtils: a collection of command-line tool interfaces to the Win95 shell
 *  Copyright (C) 1996-2000 Matt Ginzton / MaDdoG Software
 *  original author: Matt Ginzton, magi@cs.stanford.edu
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  If this concept intrigues you, read http://www.gnu.org/copyleft/
 */


#include <windows.h>
#include <assert.h>
#include <iostream>
#include "PidlFromFileSpec.h"

IShellFolder* ShellFolderPidlsFromFileSpec (char* pszRelSpec, PidlVector& pidls)
{
	char* pszFullSpec;
	IShellFolder* psfParent = NULL;
	LPITEMIDLIST pidlRel;
	BOOL success = FALSE;

	// build fully qualified path name
	pszFullSpec = _fullpath (NULL, pszRelSpec, 0);
	if (pszFullSpec == NULL)
		return FALSE;

	// make copy to expand wildcards in
	char szExpandedPath[MAX_PATH];
	lstrcpyA (szExpandedPath, pszFullSpec);
	char* pszWildcard = strrchr (szExpandedPath, '\\');
	if (pszWildcard)
		++pszWildcard;
	else
		pszWildcard = szExpandedPath + lstrlenA (szExpandedPath);

	// Problem:
	// we could be passed a drive, a file, or a group of files (wildcard).
	// (do we take multiple filenames on command line?  if not, should we?)
	// On drive, findfirstfile will fail, but parsedisplayname will give us a pidl
	// on normal file, both will succeed
	// on wildcard, findfirstfile will always succeed, but parsedisplayname will work on 9x and not on NT.
	// So which do we do first?  in NT case, either one failing has to be nonfatal.
	// And, to make things worse, on Win9x (but not NT), FindFirstFile ("E:\")
	// will behave as if we'd passed E:\* if E: is a mapped network drive,
	// so they can't get props for any mapped network drive.  Hence the hack strlen
	// comparison in the initialization of bDone.

	WIN32_FIND_DATAA find;
	HANDLE hFind;
	hFind = FindFirstFileA (pszFullSpec, &find);
	BOOL bDone = (hFind == INVALID_HANDLE_VALUE) || (lstrlenA (pszFullSpec) <= 3);

	// expand wildcards -- get all files matching spec
	while (!bDone)
	{
		// build psfParent from the first full filename
		if (!psfParent)
		{
			lstrcpyA (pszWildcard, find.cFileName);
			GetPidlAndShellFolder (szExpandedPath, &psfParent, &pidlRel);
		}

		// then use psfParent to pidl-ize them
		if (psfParent)
		{
			LPITEMIDLIST pidl;
			ULONG cch;
			WCHAR wszFile[MAX_PATH];
			size_t cvted;
			mbstowcs_s(&cvted,wszFile, find.cFileName, 1+lstrlenA(find.cFileName));

			//TODO -- could optionally mask hidden, system, etc. files here

			if (SUCCEEDED (psfParent->ParseDisplayName (NULL, NULL, wszFile, &cch, &pidl, NULL)))
				pidls.push_back (pidl);
		}

		bDone = !(FindNextFileA (hFind, &find));
	}

	FindClose (hFind);

	// if we didn't get anything from FindFirstFile, maybe it's a drive letter...
	if (pidls.size() == 0)
	{
		if (!psfParent)
			GetPidlAndShellFolder (pszFullSpec, &psfParent, &pidlRel);

		if (psfParent)
		{
			// we were able to find a pidl directly from the given path,
			// but not from FindFirstFile.  This is what happens when the
			// path is, i.e., a drive (C:\)
			// so just use the first pidl we got.
			pidls.push_back (pidlRel);
		}
	}

	free (pszFullSpec);
	return psfParent;
}


void FreePidls (PidlVector& pidls)
{
	IMalloc* shMalloc = NULL;
	if (SUCCEEDED (SHGetMalloc (&shMalloc)))
	{
		for (int i = 0; i < pidls.size(); i++)
			shMalloc->Free (pidls[i]);

		shMalloc->Release();
	}
}




#define PidlNext(pidl) ((LPITEMIDLIST)((((BYTE*)(pidl))+(pidl->mkid.cb))))

static LPITEMIDLIST CopyPidl (LPITEMIDLIST pidl, IMalloc* shMalloc)
{
	int cb = pidl->mkid.cb + 2;	//add two for terminator

	if (PidlNext (pidl)->mkid.cb != 0)
		return NULL;			//must be relative (1-length) pidl!

	LPITEMIDLIST pidlNew = (LPITEMIDLIST)shMalloc->Alloc (cb);
	if (pidlNew != NULL)
	{
		memcpy (pidlNew, pidl, cb);
	}

	return pidlNew;
}


BOOL GetPidlAndShellFolder (char* pszPath, IShellFolder** ppsfOut, LPITEMIDLIST* ppidlOut)
{
	IShellFolder * pd = NULL;
	IMalloc* shMalloc = NULL;
	LPITEMIDLIST pidlFull = NULL;
	ULONG cch;
	ULONG attrs;
	BOOL success = FALSE;

	if (!SUCCEEDED (SHGetDesktopFolder (&pd)))
		goto BailOut;

	if (!SUCCEEDED (SHGetMalloc (&shMalloc)))
		goto BailOut;

	WCHAR wpath[MAX_PATH];
	size_t cvted;
	mbstowcs_s (&cvted,wpath, pszPath, 1+lstrlenA(pszPath));

	//get fully-qualified pidl
	if (!SUCCEEDED (pd->ParseDisplayName (NULL, NULL, wpath, &cch, &pidlFull, &attrs)))
		goto BailOut;
	
	IShellFolder *psfCurr, *psfNext;
	if (!SUCCEEDED (pd->QueryInterface (IID_IShellFolder, (LPVOID*)&psfCurr)))
		goto BailOut;

	//for each pidl component, bind to folder
	LPITEMIDLIST pidlNext, pidlLast;
	pidlNext = PidlNext (pidlFull);
	pidlLast = pidlFull;
	
	while (pidlNext->mkid.cb != 0)
	{
		
		UINT uSave = pidlNext->mkid.cb;		//stop the chain temporarily
		pidlNext->mkid.cb = 0;				//so we can bind to the next folder 1 deeper
		if (!SUCCEEDED (psfCurr->BindToObject(pidlLast, NULL, IID_IShellFolder, (LPVOID*)&psfNext)))
			goto BailOut;
		pidlNext->mkid.cb = uSave;			//restore the chain

		psfCurr->Release();					//and set up to work with the next-level folder
		psfCurr = psfNext;
		pidlLast = pidlNext;

		pidlNext = PidlNext (pidlNext);		//advance to next pidl
	}

	success = TRUE;

	*ppidlOut = CopyPidl (pidlLast, shMalloc);
	*ppsfOut = psfCurr;

BailOut:
	//cleanup
	if (pidlFull != NULL && shMalloc != NULL)
		shMalloc->Free (pidlFull);		//other pidl's were only offsets into this, and don't need freeing
	if (pd != NULL)
		pd->Release();
	if (shMalloc != NULL)
		shMalloc->Release();

	return success;
}
