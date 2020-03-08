// Auto Deploy Fortnite Clone Application
// Automatically downloads, unzips, copies files into place, creates msvc++ project files, builds msvc++ win64 development solution,
// and loads up the project in unreal! :) All using only built in WinAPI no external libraries!
// By RetroGamesEngineer (C) 2020

#include <iostream>
#include <urlmon.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <strsafe.h>
#include <memory>
#include <vector>
#include <array>
#include "PidlFromFileSpec.h"
#pragma comment(lib,"urlmon.lib")
#pragma comment(lib,"shlwapi.lib")

typedef std::vector<std::wstring> FilesInZip;
typedef std::vector<std::string> vecstr;
class DownloadProgress : public IBindStatusCallback
{
private:
    uint32_t lastProgressBytes;
    double lastProgressPercentage;
    std::wstring autoConvertTo_KB_MB_GB(uint32_t bytes)
    {
        wchar_t converted[260];
        if(bytes >= (1024 * 1024 * 1024))
        {
            float GB=((float)bytes / 1024.0f / 1024.0f / 1024.0f);
            swprintf(converted,260,L"%.02f GB",GB);
        }
        else if(bytes >= (1024 * 1024))
        {
            float MB=((float)bytes / 1024.0f / 1024.0f);
            swprintf(converted,260,L"%.02f MB",MB);
        }
        else if(bytes >= 1024)
        {
            float KB=((float)bytes / 1024.0f);
            swprintf(converted,260,L"%.02f KB",KB);
        }
        else if(bytes < 1024)
            swprintf(converted,260,L"%d bytes",bytes);
        return converted;
    }
public:
    DownloadProgress()
    {
        lastProgressPercentage=0.0;
        lastProgressBytes=0;
    }
    HRESULT __stdcall QueryInterface(const IID&,void**)
    {
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef(void)
    {
        return 1;
    }
    ULONG STDMETHODCALLTYPE Release(void)
    {
        return 1;
    }
    HRESULT STDMETHODCALLTYPE OnStartBinding(DWORD dwReserved,IBinding* pib)
    {
        return E_NOTIMPL;
    }
    virtual HRESULT STDMETHODCALLTYPE GetPriority(LONG* pnPriority)
    {
        return E_NOTIMPL;
    }
    virtual HRESULT STDMETHODCALLTYPE OnLowResource(DWORD reserved)
    {
        return S_OK;
    }
    virtual HRESULT STDMETHODCALLTYPE OnStopBinding(HRESULT hresult,LPCWSTR szError)
    {
        return E_NOTIMPL;
    }
    virtual HRESULT STDMETHODCALLTYPE GetBindInfo(DWORD* grfBINDF,BINDINFO* pbindinfo)
    {
        return E_NOTIMPL;
    }
    virtual HRESULT STDMETHODCALLTYPE OnDataAvailable(DWORD grfBSCF,DWORD dwSize,FORMATETC* pformatetc,STGMEDIUM* pstgmed)
    {
        return E_NOTIMPL;
    }
    virtual HRESULT STDMETHODCALLTYPE OnObjectAvailable(REFIID riid,IUnknown* punk)
    {
        return E_NOTIMPL;
    }
    virtual HRESULT __stdcall OnProgress(ULONG ulProgress,ULONG ulProgressMax,ULONG ulStatusCode,LPCWSTR szStatusText)
    {
        if(ulStatusCode==BINDSTATUS_BEGINDOWNLOADDATA)
        {
            lastProgressPercentage=0.0;
            lastProgressBytes=0;
        }
        if(ulProgressMax==ulProgress && ulStatusCode==BINDSTATUS_ENDDOWNLOADDATA) //Download completed!
            std::wcout << autoConvertTo_KB_MB_GB(ulProgress) << " downloaded!\n";
        else if(ulProgressMax==ulProgress && (ulStatusCode==BINDSTATUS_BEGINDOWNLOADDATA || ulStatusCode==BINDSTATUS_DOWNLOADINGDATA))
        {
            //Total file size is not known, or not yet known so display downloaded bytes progress after 10MB or greater downloaded
            if((ulProgress - lastProgressBytes) >= (1024 * 1024 * 10))
            {
                std::wcout << autoConvertTo_KB_MB_GB(ulProgress) << "... ";
                lastProgressBytes=ulProgress;
            }
        }
        else if(ulProgressMax!=ulProgress && (ulStatusCode==BINDSTATUS_BEGINDOWNLOADDATA || ulStatusCode==BINDSTATUS_DOWNLOADINGDATA)) //Display percentage progress...
        {
            double dProgress;
            if(ulProgress==0)
                lastProgressPercentage=dProgress=0.0;
            else
                dProgress=(((double)ulProgress / (double)ulProgressMax) * 100.0);

            if((dProgress - lastProgressPercentage) >= 10.0) //Only after 10% or greater is downloaded...
            {
                std::wcout << dProgress << "%... ";
                lastProgressPercentage=dProgress;
            }
        }
        return S_OK;
    }
};
BOOL WINAPI ShellUnzip(LPCWSTR zipFilePath,LPCWSTR destinationDirectory);
BOOL WINAPI GetVectorOfFilesInZip(LPCWSTR zipFilePath,FilesInZip& filesInZip);
BOOL WINAPI ShellCopyFilesFromZip(FilesInZip& filesInZip,LPCWSTR destinationDirectory);
BOOL WINAPI FindMSBuildExe();
std::string QuoteStringA(std::string stringToQuote);
std::wstring QuoteStringW(std::wstring stringToQuote);
BOOL WINAPI WaitForVisualStudioSolution();
BOOL WINAPI BuildVisualStudioProject();
std::string FindFile(std::string strFile,const std::string strFilePath,const bool bRecursive,const bool bStopWhenFound);
BOOL WINAPI ClickContextMenuItem(int clickIndex,char *file);
BOOL ObeyMenuCmd(IShellFolder* psf,LPCITEMIDLIST* aPidls,int cPidls,int clickIndex);
int ChooseFromTextMenu(HMENU hMenu,int clickIndex);
char* RemoveAmpersands(char* string);
std::string exec(const char* cmd);
std::wstring s2ws(const std::string& s);
std::string ws2s(const std::wstring& s);

//If MSBuild is not at this location it will do it's best to locate it...
std::string MSBuildExePath="C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\MSBuild\\Current\\Bin\\amd64\\MSBuild.exe";
//Build development configuration and Win64 as it's only compatible with x64
std::string MSBuildParameters="/p:Configuration=Development /p:Platform=Win64 /m:4";
std::string vcsolutionPath;
std::wstring FortniteClone;
std::wstring FortniteClone_uproject;
std::wstring gameliftsdkZipUrl=L"https://github.com/YetiTech-Studios/UE4GameLiftClientSDK/archive/master.zip";
std::wstring fortnitecloneZipUrl=L"https://github.com/pivotman319-new/fortnite-clone-ue4/archive/patch-4.zip";

int main(int argc, char **argv)
{
    IShellFolder* psfDeskTop=0;
    LPITEMIDLIST pidlDesktop=0;
    STRRET strDispName;
    wchar_t pszwDesktopPath[1024];
    std::wstring desktopPath;
    DownloadProgress progress;
    FilesInZip copyThirdPartyDir;
    int step=1;

    HRESULT hr=CoInitializeEx(0,COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if(SUCCEEDED(hr))
    {
        HRESULT hr=SHGetDesktopFolder(&psfDeskTop);
        psfDeskTop->GetDisplayNameOf(pidlDesktop,SHGDN_FORPARSING,&strDispName);
        StrRetToBufW(&strDispName,pidlDesktop,pszwDesktopPath,1024);
        desktopPath=pszwDesktopPath;

        std::cout << "Hello World Fortnite Clone!\n"; //Downloading zips if neccessary...
        std::wstring gameliftclientsdkZipPath=desktopPath + L"\\gameliftclientsdk.zip";
        if(!PathFileExistsW(gameliftclientsdkZipPath.c_str()))
        {
            std::cout << step++ << ". Downloading gameliftclientsdk from github repo to desktop...\n";
            URLDownloadToFileW(0,gameliftsdkZipUrl.c_str(),gameliftclientsdkZipPath.c_str(),0,static_cast<IBindStatusCallback*>(&progress));
        }
        std::wstring fortnitecloneZipPath=desktopPath + L"\\fortnite-clone-ue4.zip";
        if(!PathFileExistsW(fortnitecloneZipPath.c_str()))
        {
            std::cout << step++ << ". Downloading fortnite-clone-ue4 from github repo to desktop...\n";
            URLDownloadToFileW(0,fortnitecloneZipUrl.c_str(),fortnitecloneZipPath.c_str(),0,static_cast<IBindStatusCallback*>(&progress));
        }
        //Unzipping fortnite-clone if neccessary...
        FortniteClone=desktopPath + L"\\fortnite-clone-ue4-patch-4";
        if(!PathFileExistsW(FortniteClone.c_str())) 
        {
            std::cout << step++ << ". Unzipping fortnite-clone-ue4 to desktop...\n";
            ShellUnzip(fortnitecloneZipPath.c_str(),desktopPath.c_str());
        }
        //Copying ThirdParty directory directly from zip if neccessary...
        std::wstring ThirdPartyDllsDest=FortniteClone + L"\\Plugins"; 
        if(!PathFileExistsW((ThirdPartyDllsDest + L"\\GameLiftClientSDK\\ThirdParty").c_str())) 
        {
            std::cout << step++ << ". Copying gameliftsdk dlls and libs to fortnite-clone-ue4-patch-4/Plugins/GameLiftClientSDK/ThirdParty/GameLiftClientSDK/Win64\n";
            std::wstring ThirdPartyDllsSource=gameliftclientsdkZipPath + L"\\UE4GameLiftClientSDK-master\\GameLiftClientSDK";
            std::wcout << "Copying: " << ThirdPartyDllsSource << "\nTo: " << ThirdPartyDllsDest << "\n";
            copyThirdPartyDir.push_back(ThirdPartyDllsSource);
            ShellCopyFilesFromZip(copyThirdPartyDir,ThirdPartyDllsDest.c_str());
        }
        //Generating visual studio project files if neccessary...
        vcsolutionPath=ws2s(FortniteClone) + "\\FortniteClone.sln";
        FortniteClone_uproject=FortniteClone + L"\\FortniteClone.uproject";
        if(!PathFileExistsA(vcsolutionPath.c_str()))
        {
            std::cout << step++ << ". Generating visual studio project files for fortnite-clone-ue4...\n";
            ClickContextMenuItem(2,(char*)ws2s(FortniteClone_uproject).c_str()); //Click generate visual studio project files
            WaitForVisualStudioSolution();
        }
        //Always building the project and launching it in unreal :)
        std::cout << step++ << ". Building fortnite-clone-ue4 visual studio project...\n";
        if(BuildVisualStudioProject())
        {
            std::cout << "SUCCEEDED! Now Launching fortnite-clone-ue4 in unreal engine!\n";
            ClickContextMenuItem(0,(char*)ws2s(FortniteClone_uproject).c_str()); //Click open on the unreal project
        }
        else
        {
            std::cout << "Build failed... Try building it manually in visual studio to see what's wrong, loading up solution now...\n";
            ClickContextMenuItem(0,(char*)vcsolutionPath.c_str()); //Click open on the visual studio project
        }
        CoUninitialize();
    }
    else
    {
        std::cout << "CoInitializeEx failed somehow... Can't proceed..\n";
    }
    return 0;
}

//Credit to Microsoft (learned from them and Matt Ginzton / MaDdoG Software to make this possible) @ https://docs.microsoft.com/en-us/windows/win32/shell/manage
BOOL WINAPI ShellUnzip(LPCWSTR zipFilePath,LPCWSTR destinationDirectory)
{
    FilesInZip filesInZip;
    return GetVectorOfFilesInZip(zipFilePath,filesInZip) && ShellCopyFilesFromZip(filesInZip,destinationDirectory);
}

BOOL WINAPI GetVectorOfFilesInZip(LPCWSTR zipFilePath,FilesInZip& filesInZip)
{
    IShellFolder* psfDeskTop=0,* psfZipFile=0;
    LPITEMIDLIST pidlZipFile=0,pidlItems=0;
    IEnumIDList* ppenum=0;
    STRRET strDispName;
    SFGAOF attrs;
    ULONG celtFetched;
    wchar_t pszwDisplayName[1024];

    HRESULT hr=SHGetDesktopFolder(&psfDeskTop);
    hr=SHParseDisplayName(zipFilePath,nullptr,&pidlZipFile,0,&attrs);
    if(FAILED(hr))
    {
        std::cout << "SHParseDisplayName failed for " << zipFilePath << std::endl;
        psfDeskTop->Release();
        return FALSE;
    }
    hr=psfDeskTop->BindToObject(pidlZipFile,0,IID_IShellFolder,(LPVOID*)&psfZipFile);
    psfDeskTop->Release();
    hr=psfZipFile->EnumObjects(0,SHCONTF_FOLDERS | SHCONTF_NONFOLDERS,&ppenum);
    while(hr=ppenum->Next(1,&pidlItems,&celtFetched) == S_OK && (celtFetched) == 1)
    {
        psfZipFile->GetDisplayNameOf(pidlItems,SHGDN_FORPARSING,&strDispName);
        StrRetToBufW(&strDispName,pidlItems,pszwDisplayName,1024);

        filesInZip.push_back(pszwDisplayName);

        CoTaskMemFree(pidlItems);
    }
    ppenum->Release();

    CoTaskMemFree(pidlZipFile);
    psfZipFile->Release();
    return TRUE;
}

BOOL WINAPI ShellCopyFilesFromZip(FilesInZip& filesInZip,LPCWSTR destinationDirectory)
{
    BOOL success=TRUE;
    HRESULT hr;
    IFileOperation* pfo=0;
    if(SUCCEEDED(hr=CoCreateInstance(CLSID_FileOperation,0,CLSCTX_ALL,IID_PPV_ARGS(&pfo))))
    {
        pfo->SetOperationFlags(FOF_NO_UI);

        IShellItem* psiTo=0;
        hr=SHCreateItemFromParsingName(destinationDirectory,0,IID_PPV_ARGS(&psiTo));
        if(FAILED(hr))
        {
            std::cout << "SHCreateItemFromParsingName failed for " << destinationDirectory << "\n";
            pfo->Release();
            return FALSE;
        }

        for(auto &f : filesInZip)
        {
            LPITEMIDLIST fromFile=0;
            SFGAOF attrs;
            HRESULT hr=SHParseDisplayName(f.c_str(),0,&fromFile,0,&attrs);
            if(FAILED(hr))
            {
                std::wcout << "SHParseDisplayName failed for " << f.c_str() << "\n";
                psiTo->Release();
                pfo->Release();
                return FALSE;
            }
            IShellItemArray* pShellItemArr=0;
            hr=SHCreateShellItemArrayFromIDLists(1,(LPCITEMIDLIST*)&fromFile,&pShellItemArr);
            if(FAILED(hr))
            {
                std::cout << "SHCreateShellItemArrayFromIDLists failed\n";
                psiTo->Release();
                pfo->Release();
                return FALSE;
            }

            pfo->CopyItems(pShellItemArr,psiTo);

            hr=pfo->PerformOperations();
            if(SUCCEEDED(hr))
            {
                std::wcout << "Successfully created: " << f.c_str() << " ...\n";
                success=TRUE;
            }
            else
            {
                std::cout << "Failed creating: " << f.c_str() << " ...\n";
                success=FALSE;
            }
            CoTaskMemFree(fromFile);
            pShellItemArr->Release();
        }
        psiTo->Release();
    }
    pfo->Release();
    return success;
}

BOOL WINAPI FindMSBuildExe()
{
    if(PathFileExistsA(MSBuildExePath.c_str()))
        return TRUE;

    PWSTR ppszPath;
    SHGetKnownFolderPath(FOLDERID_ProgramFilesX86,0,0,&ppszPath);
    std::string programFilesPath=ws2s(ppszPath);
    std::string msvcPath=programFilesPath + "\\Microsoft Visual Studio";

    MSBuildExePath=FindFile("MSBuild.exe",msvcPath + "\\2019",true,true);
    if(!MSBuildExePath.empty())
        return TRUE;

    MSBuildExePath=FindFile("MSBuild.exe",msvcPath,true,true);
    if(!MSBuildExePath.empty())
        return TRUE;

    return FALSE;
}

std::string QuoteStringA(std::string stringToQuote)
{
    return "\"" + stringToQuote + "\"";
}

std::wstring QuoteStringW(std::wstring stringToQuote)
{
    return L"\"" + stringToQuote + L"\"";
}

BOOL WINAPI WaitForVisualStudioSolution()
{
    std::cout << "Select your chosen unreal version from the popup and click ok for it to generate the visual studio solution.\n"
        << "Waiting for the solution to be created... at: " << vcsolutionPath << "\n";
    while(!PathFileExistsA(vcsolutionPath.c_str()))
        Sleep(100);
    return TRUE;
}

BOOL WINAPI BuildVisualStudioProject()
{
    FindMSBuildExe();
    Sleep(5000);
    std::string MSBuildCommand="cd " + ws2s(FortniteClone) + " && " + QuoteStringA(MSBuildExePath) + " /t:Build FortniteClone.sln " + MSBuildParameters;
    std::cout << "Running MSBuild Command:\n" << MSBuildCommand << "\n";
    std::string result=exec(MSBuildCommand.c_str());
    if(StrStrIA(result.c_str(),"Build succeeded")!=0)
        return TRUE;
    return FALSE;
}

//Credit to Remy Lebeau (slightly modified) @ https://stackoverflow.com/questions/15068475/recursive-hard-disk-search-with-findfirstfile-findnextfile-c
std::string FindFile(std::string strFile,const std::string strFilePath,const bool bRecursive,const bool bStopWhenFound)
{
    std::string strFoundFilePath;
    WIN32_FIND_DATAA file;

    std::string strPathToSearch=strFilePath;
    if(!strPathToSearch.empty())
    {
        if(strPathToSearch.at(strPathToSearch.size()-1) != '\\')
            strPathToSearch+="\\";
    }

    HANDLE hFile=FindFirstFileA((strPathToSearch + "*").c_str(),&file);
    if(hFile != INVALID_HANDLE_VALUE)
    {
        auto subDirs=std::make_unique<vecstr>();
        do
        {
            std::string strTheNameOfTheFile=file.cFileName;

            // It could be a directory we are looking at
            // if so look into that dir
            if(file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                if((strTheNameOfTheFile != ".") && (strTheNameOfTheFile != "..") && (bRecursive))
                {
                    if(subDirs.get() == nullptr)
                        subDirs.reset(new vecstr());

                    subDirs->push_back(strPathToSearch + strTheNameOfTheFile);
                }
            }
            else
            {
                if(strTheNameOfTheFile == strFile)
                {
                    strFoundFilePath=strPathToSearch + strFile;

                    std::cout << "Found MSBuild! " << strFoundFilePath << "\n";

                    if(bStopWhenFound)
                        break;
                }
            }
        } while(FindNextFileA(hFile,&file));

        FindClose(hFile);

        if(!strFoundFilePath.empty() && bStopWhenFound)
            return strFoundFilePath;

        if(subDirs.get() != NULL)
        {
            for(int i=0; i < subDirs->size(); ++i)
            {
                strFoundFilePath=FindFile(strFile,subDirs->at(i),bRecursive,bStopWhenFound);

                if(!strFoundFilePath.empty() && bStopWhenFound)
                    break;
            }
        }
    }

    return strFoundFilePath;
}

BOOL WINAPI ClickContextMenuItem(int clickIndex,char *file)
{
    PidlVector pidls;
    IShellFolder* psfParent=ShellFolderPidlsFromFileSpec(file,pidls);
    if(!psfParent || !ObeyMenuCmd(psfParent,(LPCITEMIDLIST*)&pidls[0],pidls.size(),clickIndex))
    {
        printf("\rContextMenu: Unable to create menu for %s\n",file);
        return FALSE;
    }

    if(psfParent)
        psfParent->Release();

    FreePidls(pidls);

    return TRUE;
}

//Credit to (slightly modified): Matt Ginzton / MaDdoG Software (in project licensed as GPL)-> PidlFromSpec.h,PidlFromFileSpec.cpp
BOOL ObeyMenuCmd(IShellFolder* psf,LPCITEMIDLIST* aPidls,int cPidls,int clickIndex)
{
    BOOL success=FALSE;

    //get its context menu
    IContextMenu* pContextMenu;
    if(SUCCEEDED(psf->GetUIObjectOf(NULL,cPidls,aPidls,IID_IContextMenu,NULL,(void**)&pContextMenu)))
    {
        //try to get IContextMenu2 for SendTo
        IContextMenu2* pCM2;
        if(SUCCEEDED(pContextMenu->QueryInterface(IID_IContextMenu2,(LPVOID*)&pCM2)))
        {
            pContextMenu->Release();
            pContextMenu=(IContextMenu*)pCM2;
        }

        //prepare to invoke properties command
        CMINVOKECOMMANDINFO invoke;
        memset(&invoke,0,sizeof(invoke));
        invoke.cbSize=sizeof(invoke);
        invoke.fMask=0;
        invoke.hwnd=NULL;
        invoke.nShow=SW_SHOWNORMAL;

        HMENU hMenu=CreatePopupMenu();
        pContextMenu->QueryContextMenu(hMenu,0,0,0x7fff,CMF_EXPLORE);

        int idCmd=ChooseFromTextMenu(hMenu,clickIndex);

        invoke.lpVerb=MAKEINTRESOURCEA(idCmd);

        //invoke command
        if(SUCCEEDED(pContextMenu->InvokeCommand(&invoke)))
        {
            success=TRUE;
        }
        else
        {
            printf("ContextMenu: command could not be carried out.\n");
        }

        pContextMenu->Release();
    }

    return success;
}


int ChooseFromTextMenu(HMENU hMenu, int clickIndex)
{
    MENUITEMINFOA mii;
    char szName[MAX_PATH];
    //char* printableName;

    int nItems=::GetMenuItemCount(hMenu);
    // display menu
    /*for(int iItem=0; iItem < nItems; iItem++)
    {
        memset(&mii,0,sizeof(mii));
        mii.cbSize=sizeof(mii);
        mii.fMask=MIIM_SUBMENU | MIIM_TYPE;
        mii.dwTypeData=szName;
        mii.cch=sizeof(szName);
        ::GetMenuItemInfoA(hMenu,iItem,TRUE,&mii);

        if(mii.fType & MF_BITMAP)
            printableName=(char*)"(bitmap)";
        else if(mii.fType & MF_SEPARATOR)
            printableName=(char*)"------------------------";
        else
            printableName=RemoveAmpersands(mii.dwTypeData);

        printf("%2d: %-20s %s\n",iItem+1,printableName,mii.hSubMenu ? "-->" : "");
    }*/

    int choice=clickIndex;
    if(choice != -1)
    {
        memset(&mii,0,sizeof(mii));
        mii.cbSize=sizeof(mii);
        mii.fMask=MIIM_SUBMENU | MIIM_ID | MIIM_TYPE;
        mii.dwTypeData=szName;
        mii.cch=0;
        ::GetMenuItemInfoA(hMenu,choice,TRUE,&mii);
        choice=mii.wID;
        return choice;
    }
    else
    {
        while(choice == -1)
        {
            printf("Choice? ");
            scanf_s("%d",&choice);
            if(choice > 0 && choice <= nItems)
            {
                choice--;	// convert to 0-based

                memset(&mii,0,sizeof(mii));
                mii.cbSize=sizeof(mii);
                mii.fMask=MIIM_SUBMENU | MIIM_ID | MIIM_TYPE;
                mii.dwTypeData=szName;
                mii.cch=0;
                ::GetMenuItemInfoA(hMenu,choice,TRUE,&mii);

                if(mii.hSubMenu)
                {
                    choice=ChooseFromTextMenu(mii.hSubMenu,clickIndex);
                    break;
                }

                if(!(mii.fType & MF_SEPARATOR))
                {
                    choice=mii.wID;
                    break;
                }
            }

            printf("Invalid choice; please try again.\n");
            choice=-1;
        }
    }

    return choice;
}


char* RemoveAmpersands(char* string)
{
    for(char* psz=string; *psz != 0; psz++)
    {
        if(*psz == '&')
            memmove(psz,psz + 1,strlen(psz));
    }

    return string;
}

//Credit to waqas (slightly modified) @ https://stackoverflow.com/questions/478898/how-do-i-execute-a-command-and-get-the-output-of-the-command-within-c-using-po
std::string exec(const char* cmd)
{
    std::array<char,8192> buffer;
    std::string result;
    std::unique_ptr<FILE,decltype(&_pclose)> pipe(_popen(cmd,"r"),_pclose);
    if(!pipe)
        return "popen() failed!";
    while(fgets(buffer.data(),buffer.size(),pipe.get()) != nullptr)
    {
        std::cout << buffer.data();
        result+=buffer.data();
    }
    std::cout << "\n";
    return result;
}

//Credit to Jere Jones (slightly modified) @ https://codereview.stackexchange.com/questions/419/converting-between-stdwstring-and-stdstring
std::wstring s2ws(const std::string& s)
{
    int slength=(int)s.length() + 1;
    int len=MultiByteToWideChar(CP_ACP,0,s.c_str(),slength,0,0);
    std::wstring r(len-1,'\0');
    MultiByteToWideChar(CP_ACP,0,s.c_str(),slength,&r[0],len);
    return r;
}

std::string ws2s(const std::wstring& s)
{
    int slength=(int)s.length() + 1;
    int len=WideCharToMultiByte(CP_ACP,0,s.c_str(),slength,0,0,0,0);
    std::string r(len-1,'\0');
    WideCharToMultiByte(CP_ACP,0,s.c_str(),slength,&r[0],len,0,0);
    return r;
}