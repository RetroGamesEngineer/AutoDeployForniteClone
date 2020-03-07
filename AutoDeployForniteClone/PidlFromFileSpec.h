// DetachFromCommandPrompt.h		utility functions for CmdUtils
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


#ifndef _PIDL_FROM_FILESPEC_H_
#define _PIDL_FROM_FILESPEC_H_


#include <vector>
#include <shlobj.h>

typedef std::vector<LPITEMIDLIST> PidlVector;

BOOL GetPidlAndShellFolder(char* pszPath,IShellFolder** ppsfOut,LPITEMIDLIST* ppidlOut);
IShellFolder* ShellFolderPidlsFromFileSpec (char* pszRelSpec, PidlVector& pidls);
void FreePidls (PidlVector& pidls);


#endif // _PIDL_FROM_FILESPEC_H_
