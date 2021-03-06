#include "stdafx.h"

/*-----------------------------------------------------------------------------
Copyright 1995 - 2000 Microsoft Corporation. All Rights Reserved.

COMMENTS:
This file is copied from Microsoft SDK samples
All buffers allocated in this sample come from HeapAlloc.  If
HeapAlloc fails, the PERR macro is not used because HeapAlloc does
not call SetLastError().
-----------------------------------------------------------------------------*/
#include <windows.h>
#include <stdlib.h>


//
//  Constants and Macros
//
#define USER_ACCESS             GENERIC_ALL
#define WORLD_ACCESS            GENERIC_READ
#define FILENAME                "testfile.txt"

#define PERR(s) fprintf(stderr, "%s(%d) %s : Error %d\n%s\n", \
    __FILE__, __LINE__, (s), GetLastError(), \
    GetLastErrorText())

//
//  Prototypes
//
PSID GetUserSid(void);
PSID CreateWorldSid(void);
LPSTR GetLastErrorText(void);


//
//  FUNCTION: main
//
//  PURPOSE: Driving routine for this sample.
//
//  PARAMETERS:
//      none
//
//  RETURN VALUE:
//      none
//
//  COMMENTS:
//
int _tmain(int argc, _TCHAR* argv[])
{
    PSID psidUser, psidEveryone;
    PACL pAcl;
    SECURITY_DESCRIPTOR sd;
    SECURITY_ATTRIBUTES sa;
    BOOL bRes;
    HANDLE hFile;
    DWORD dwBytesWritten, cbMsg, cbAcl;
    char szMsg[] = "Protected data\n";


    // Get the SIDs we'll need for the DACL
    //
    psidUser = GetUserSid();
    psidEveryone = CreateWorldSid();


    // Allocate space for the ACL
    //      For information about computing the size of the ACL, see
    //      the Win32 SDK reference entry for InitializeAcl()
    //
    cbAcl = GetLengthSid(psidUser) + GetLengthSid(psidEveryone) +
        sizeof(ACL)+(2 * (sizeof(ACCESS_ALLOWED_ACE)-sizeof(DWORD)));

    pAcl = (PACL)HeapAlloc(GetProcessHeap(), 0, cbAcl);
    if (NULL == pAcl) {
        fprintf(stderr, "HeapAlloc failed.\n");
        ExitProcess(EXIT_FAILURE);
    }


    bRes = InitializeAcl(pAcl,
        cbAcl,
        ACL_REVISION);
    if (FALSE == bRes) {
        PERR("InitializeAcl");
        ExitProcess(EXIT_FAILURE);
    }

    // Add Aces for User and World
    //
    bRes = AddAccessAllowedAce(pAcl,
        ACL_REVISION,
        USER_ACCESS,
        psidUser);
    if (FALSE == bRes) {
        PERR("AddAccessAllowedAce");
        ExitProcess(EXIT_FAILURE);
    }

    /*bRes = AddAccessAllowedAce(pAcl,
        ACL_REVISION,
        WORLD_ACCESS,
        psidEveryone);
    if (FALSE == bRes) {
        PERR("AddAccessAllowedAce");
        ExitProcess(EXIT_FAILURE);
    }*/

    // Put together the security descriptor
    //
    bRes = InitializeSecurityDescriptor(&sd,
        SECURITY_DESCRIPTOR_REVISION);
    if (FALSE == bRes) {
        PERR("InitializeSecurityDescriptor");
        ExitProcess(EXIT_FAILURE);
    }

    bRes = SetSecurityDescriptorDacl(&sd,
        TRUE,
        pAcl,
        FALSE);
    if (FALSE == bRes) {
        PERR("SetSecurityDescriptorDacl");
        ExitProcess(EXIT_FAILURE);
    }

    // Add the security descriptor to the sa structure
    //
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = &sd;
    sa.bInheritHandle = FALSE;

    // Generate the file using the security attributes that
    // we've assembled
    //
    hFile = CreateFile(FILENAME,
        GENERIC_READ |
        GENERIC_WRITE,
        0,
        &sa,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (INVALID_HANDLE_VALUE == hFile) {
        PERR("CreateFile");
        ExitProcess(EXIT_FAILURE);
    }

    cbMsg = lstrlen(szMsg);

    bRes = WriteFile(hFile,
        szMsg,
        cbMsg,
        &dwBytesWritten,
        NULL);
    if (FALSE == bRes) {
        PERR("WriteFile");
        ExitProcess(EXIT_FAILURE);
    }

    if (!CloseHandle(hFile)) {
        PERR("CloseHandle");
        ExitProcess(EXIT_FAILURE);
    }

    // Clean up
    //
    HeapFree(GetProcessHeap(), 0, pAcl);
    HeapFree(GetProcessHeap(), 0, psidUser);
    HeapFree(GetProcessHeap(), 0, psidEveryone);


    printf("Created %s with special access.\n", FILENAME);

    ExitProcess(EXIT_SUCCESS);
}


//
//  FUNCTION: GetUserSid
//
//  PURPOSE: Obtains a pointer to the SID for the current user
//
//  PARAMETERS:
//      none
//
//  RETURN VALUE:
//      Pointer to the SID
//
//  COMMENTS:
//      The SID buffer returned by this function is allocated with
//      HeapAlloc and should be freed with HeapFree.
//
PSID GetUserSid()
{
    HANDLE hToken;
    BOOL bRes;
    DWORD cbBuffer, cbRequired;
    PTOKEN_USER pUserInfo;
    PSID pUserSid;

    // The User's SID can be obtained from the process token
    //
    bRes = OpenProcessToken(GetCurrentProcess(),
        TOKEN_QUERY,
        &hToken);
    if (FALSE == bRes) {
        PERR("OpenProcessToken");
        ExitProcess(EXIT_FAILURE);
    }

    // Set buffer size to 0 for first call to determine
    // the size of buffer we need.
    //
    cbBuffer = 0;
    bRes = GetTokenInformation(hToken,
        TokenUser,
        NULL,
        cbBuffer,
        &cbRequired);

    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        PERR("GetTokenInformation");
        ExitProcess(EXIT_FAILURE);
    }

    // Allocate a buffer for our token user data
    //
    cbBuffer = cbRequired;
    pUserInfo = (PTOKEN_USER)HeapAlloc(GetProcessHeap(), 0, cbBuffer);
    if (NULL == pUserInfo) {
        fprintf(stderr, "HeapAlloc failed.\n");
        ExitProcess(EXIT_FAILURE);
    }

    // Make the "real" call
    //
    bRes = GetTokenInformation(hToken,
        TokenUser,
        pUserInfo,
        cbBuffer,
        &cbRequired);
    if (FALSE == bRes) {
        PERR("GetTokenInformation");
        ExitProcess(EXIT_FAILURE);
    }

    // Make another copy of the SID for the return value
    //
    cbBuffer = GetLengthSid(pUserInfo->User.Sid);

    pUserSid = (PSID)HeapAlloc(GetProcessHeap(), 0, cbBuffer);
    if (NULL == pUserSid) {
        fprintf(stderr, "HeapAlloc failed.\n");
        ExitProcess(EXIT_FAILURE);
    }

    bRes = CopySid(cbBuffer, pUserSid, pUserInfo->User.Sid);
    if (FALSE == bRes) {
        PERR("CopySid");
        ExitProcess(EXIT_FAILURE);
    }

    bRes = HeapFree(GetProcessHeap(), 0, pUserInfo);
    if (FALSE == bRes) {
        fprintf(stderr, "HeapFree failed.\n");
        ExitProcess(EXIT_FAILURE);
    }

    return pUserSid;
}


//
//  FUNCTION: CreateWorldSid
//
//  PURPOSE: Creates a SID that represents "Everyone"
//
//  PARAMETERS:
//      none
//
//  RETURN VALUE:
//      A pointer to the SID.
//
//  COMMENTS:
//      The SID buffer returned by this function is allocated with
//      HeapAlloc and should be freed with HeapFree.  I made a copy
//      of the SID on the heap so that when the calling code is done
//      with the SID it can use HeapFree().  This is consistent with
//      other parts of this sample.
//
//      The SID Authority and RID used here are defined in winnt.h.
//
PSID CreateWorldSid()
{
    SID_IDENTIFIER_AUTHORITY authWorld = SECURITY_WORLD_SID_AUTHORITY;
    PSID pSid, psidWorld;
    BOOL bRes;
    DWORD cbSid;

    bRes = AllocateAndInitializeSid(&authWorld,
        1,
        SECURITY_WORLD_RID,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        &psidWorld);
    if (FALSE == bRes) {
        PERR("AllocateAndInitializeSid");
        ExitProcess(EXIT_FAILURE);
    }

    // Make a copy of the SID using a HeapAlloc'd block for return
    //
    cbSid = GetLengthSid(psidWorld);

    pSid = (PSID)HeapAlloc(GetProcessHeap(), 0, cbSid);
    if (NULL == pSid) {
        fprintf(stderr, "HeapAlloc failed.\n");
        ExitProcess(EXIT_FAILURE);
    }

    bRes = CopySid(cbSid, pSid, psidWorld);
    if (FALSE == bRes) {
        PERR("CopySid");
        ExitProcess(EXIT_FAILURE);
    }

    FreeSid(psidWorld);

    return pSid;
}


//
//  FUNCTION: GetLastErrorText
//
//  PURPOSE: Retrieves the text associated with the last system error.
//
//  PARAMETERS:
//      none
//
//  RETURN VALUE:
//      A pointer to the error text.
//
//  COMMENTS:
//      The contents of the returned buffer will only be valid until
//      the next call to this routine.
//
LPSTR GetLastErrorText()
{
#define MAX_MSG_SIZE 256

    static char szMsgBuf[MAX_MSG_SIZE];
    DWORD dwError, dwRes;

    dwError = GetLastError();

    dwRes = FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        dwError,
        MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
        szMsgBuf,
        MAX_MSG_SIZE,
        NULL);
    if (0 == dwRes) {
        fprintf(stderr, "FormatMessage failed with %d\n", GetLastError());
        ExitProcess(EXIT_FAILURE);
    }

    return szMsgBuf;
}
void DumpACL(PACL pACL) {
    __try {
        if (pACL == NULL) {
            _tprintf(TEXT("NULL DACL\n"));
            __leave;
        }

        ACL_SIZE_INFORMATION aclSize = { 0 };
        if (!GetAclInformation(pACL, &aclSize, sizeof(aclSize),
            AclSizeInformation))
            __leave;
        _tprintf(TEXT("ACL ACE count: %d\n"), aclSize.AceCount);

        struct {
            BYTE  lACEType;
            PTSTR pszTypeName;
        }aceTypes[6] = {
            { ACCESS_ALLOWED_ACE_TYPE, TEXT("ACCESS_ALLOWED_ACE_TYPE") },
            { ACCESS_DENIED_ACE_TYPE, TEXT("ACCESS_DENIED_ACE_TYPE") },
            { SYSTEM_AUDIT_ACE_TYPE, TEXT("SYSTEM_AUDIT_ACE_TYPE") },
            { ACCESS_ALLOWED_OBJECT_ACE_TYPE,
            TEXT("ACCESS_ALLOWED_OBJECT_ACE_TYPE") },
            { ACCESS_DENIED_OBJECT_ACE_TYPE,
            TEXT("ACCESS_DENIED_OBJECT_ACE_TYPE") },
            { SYSTEM_AUDIT_OBJECT_ACE_TYPE,
            TEXT("SYSTEM_AUDIT_OBJECT_ACE_TYPE") } };

        struct {
            ULONG lACEFlag;
            PTSTR pszFlagName;
        }aceFlags[7] = {
            { INHERITED_ACE, TEXT("INHERITED_ACE") },
            { CONTAINER_INHERIT_ACE, TEXT("CONTAINER_INHERIT_ACE") },
            { OBJECT_INHERIT_ACE, TEXT("OBJECT_INHERIT_ACE") },
            { INHERIT_ONLY_ACE, TEXT("INHERIT_ONLY_ACE") },
            { NO_PROPAGATE_INHERIT_ACE, TEXT("NO_PROPAGATE_INHERIT_ACE") },
            { FAILED_ACCESS_ACE_FLAG, TEXT("FAILED_ACCESS_ACE_FLAG") },
            { SUCCESSFUL_ACCESS_ACE_FLAG,
            TEXT("SUCCESSFUL_ACCESS_ACE_FLAG") } };

        for (ULONG lIndex = 0; lIndex < aclSize.AceCount; lIndex++) {
            ACCESS_ALLOWED_ACE* pACE;
            if (!GetAce(pACL, lIndex, (PVOID*)&pACE))
                __leave;

            _tprintf(TEXT("\nACE #%d\n"), lIndex);

            ULONG lIndex2 = 6;
            PTSTR pszString = TEXT("Unknown ACE Type");
            while (lIndex2--) {
                if (pACE->Header.AceType == aceTypes[lIndex2].lACEType) {
                    pszString = aceTypes[lIndex2].pszTypeName;
                }
            }
            _tprintf(TEXT("  ACE Type =\n  \t%s\n"), pszString);

            _tprintf(TEXT("  ACE Flags = \n"));
            lIndex2 = 7;
            while (lIndex2--) {
                if ((pACE->Header.AceFlags & aceFlags[lIndex2].lACEFlag)
                    != 0)
                    _tprintf(TEXT("  \t%s\n"),
                        aceFlags[lIndex2].pszFlagName);
            }

            _tprintf(TEXT("  ACE Mask (32->0) =\n  \t"));
            lIndex2 = (ULONG)1 << 31;
            while (lIndex2) {
                _tprintf(((pACE->Mask & lIndex2) != 0) ? TEXT("1") : TEXT("0"));
                lIndex2 >>= 1;
            }

            TCHAR szName[1024];
            TCHAR szDom[1024];
            PSID pSID = PSIDFromPACE(pACE);
            SID_NAME_USE sidUse;
            ULONG lLen1 = 1024, lLen2 = 1024;
            if (!LookupAccountSid(NULL, pSID,
                szName, &lLen1, szDom, &lLen2, &sidUse))
                lstrcpy(szName, TEXT("Unknown"));
            PTSTR pszSID;
            if (!ConvertSidToStringSid(pSID, &pszSID))
                __leave;
            _tprintf(TEXT("\n  ACE SID =\n  \t%s (%s)\n"), pszSID, szName);
            LocalFree(pszSID);
        }
    }
    __finally {}
}


