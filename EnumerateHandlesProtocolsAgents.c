/** 

  Copyright (c) 2025, Federico Di Marco <fededim@gmail.com>. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the MIT License
  which accompanies this distribution.  The full text of the license may be found at
  https://opensource.org/license/mit

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  A simple UEFI application which dumps all supported handles, protocols and agents (GUIDS) by your BIOS.
  Launch it from an UEFI Shell.

**/



//
// Basic UEFI Libraries
//
#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
//#include <Library/HandleParsingLib.h>

//
// Boot and Runtime Services
//
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

//
// Shell Library
//
#include <Library/ShellLib.h>

//
// We run on any UEFI Specification
//
extern CONST UINT32 _gUefiDriverRevision = 0;

//
// Our name
//
CHAR8* gEfiCallerBaseName = "EnumerateHandlesProtocolsAgents";

EFI_STATUS
EFIAPI
UefiUnload(
	IN EFI_HANDLE ImageHandle
)
{
	// 
	// This code should be compiled out and never called 
	// 
	ASSERT(FALSE);
}



CHAR16* GetStringNameFromHandle(EFI_HANDLE TheHandle) {
	EFI_COMPONENT_NAME2_PROTOCOL* CompNameStruct;
	EFI_STATUS                    Status;
	CHAR16* RetVal;
	CHAR8* BestLang;

	BestLang = NULL;

	if (TheHandle == NULL)
		return L"";

	// try first componentName2
	Status = gBS->OpenProtocol(TheHandle, &gEfiComponentName2ProtocolGuid, (VOID**)&CompNameStruct, gImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
	if (EFI_ERROR(Status))
	{
		DEBUG((EFI_D_ERROR, "Error opening COMPONENT NAME 2 PROTOCOL on handle 0x%08X %r\n", TheHandle, Status));
		return L"";
	}

	Status = CompNameStruct->GetDriverName(CompNameStruct, "en", &RetVal);
	gBS->CloseProtocol(TheHandle, &gEfiComponentName2ProtocolGuid, (VOID**)&CompNameStruct, gImageHandle);
	if (EFI_ERROR(Status)) {
		DEBUG((EFI_D_ERROR, "Error getting driver name on handle 0x%08X %r\n", TheHandle, Status));
	}
	else {
		return RetVal;
	}

	// try then componentName1
	Status = gBS->OpenProtocol(TheHandle, &gEfiComponentNameProtocolGuid, (VOID**)&CompNameStruct, gImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
	if (EFI_ERROR(Status))
	{
		DEBUG((EFI_D_ERROR, "Error opening COMPONENT NAME 1 PROTOCOL on handle 0x%08X %r\n", TheHandle, Status));
		return L"";
	}

	Status = CompNameStruct->GetDriverName(CompNameStruct, "eng", &RetVal);
	gBS->CloseProtocol(TheHandle, &gEfiComponentNameProtocolGuid, (VOID**)&CompNameStruct, gImageHandle);
	if (EFI_ERROR(Status)) {
		DEBUG((EFI_D_ERROR, "Error getting driver name on handle 0x%08X %r\n", TheHandle, Status));
		return L"";
	}

	return RetVal;
}




EFI_STATUS
EFIAPI
EfiMain(
	IN EFI_HANDLE ImageHandle,
	IN EFI_SYSTEM_TABLE* SystemTable
)
{
	EFI_BOOT_SERVICES* pBS = SystemTable->BootServices;
	EFI_STATUS Status;
	UINTN HandleCount;
	EFI_HANDLE* pHandleBuffer;
	UINTN HandleIndex;
	UINTN ProtocolCount, AgentCount;
	EFI_GUID** pProtocolBuffer;
	UINTN ProtocolIndex, AgentIndex;
	EFI_OPEN_PROTOCOL_INFORMATION_ENTRY* pProtocolInfos;
	EFI_OPEN_PROTOCOL_INFORMATION_ENTRY pProtocolInfo;

	// 1st get list of all handles
	Status = pBS->LocateHandleBuffer(
		AllHandles,
		NULL,
		NULL,
		&HandleCount,
		&pHandleBuffer);
	if (EFI_ERROR(Status))
	{
		DEBUG((EFI_D_ERROR, "LocateHandleBuffer failed %r\n", Status));
		return SHELL_ABORTED;
	}

	Print(L"Enumerated %d HANDLES", HandleCount);

	// 2nd interate handles and get+print all protocols
	for (HandleIndex = 0; HandleIndex < HandleCount; HandleIndex++)
	{
		Status = pBS->ProtocolsPerHandle(
			pHandleBuffer[HandleIndex],
			&pProtocolBuffer,
			&ProtocolCount);
		if (EFI_ERROR(Status))
		{
			DEBUG((EFI_D_ERROR, "[%d]: HANDLE 0x%08X...ProtocolsPerHandle call failed status %r\n", HandleIndex, pHandleBuffer[HandleIndex], Status));
			pBS->FreePool(pHandleBuffer);
			continue;
		}

		Print(L"\n\n[%d]: HANDLE 0x%08X (%S)...found %d PROTOCOLS\n", HandleIndex, pHandleBuffer[HandleIndex], GetStringNameFromHandle(pHandleBuffer[HandleIndex]), ProtocolCount);

		for (ProtocolIndex = 0; ProtocolIndex < ProtocolCount; ProtocolIndex++)
		{
			Print(L"(%d): PROTOCOL GUID %g\n", ProtocolIndex, pProtocolBuffer[ProtocolIndex]);

			AgentCount = 0;
			Status = pBS->OpenProtocolInformation(pHandleBuffer[HandleIndex], pProtocolBuffer[ProtocolIndex], &pProtocolInfos, &AgentCount);
			if (EFI_ERROR(Status))
			{
				DEBUG((EFI_D_ERROR, "(%d): PROTOCOL %g...OpenProtocolInformation failed status:%r\n", ProtocolIndex, pProtocolBuffer[ProtocolIndex], Status));
				continue;
			}

			for (AgentIndex = 0; AgentIndex < AgentCount; AgentIndex++)
			{
				pProtocolInfo = pProtocolInfos[AgentIndex];
				Print(L"{%d}: AGENT 0x%08X (%S) CONTROLLER 0x%08X (%S) ATTRIBUTES %x\n", AgentIndex, pProtocolInfo.AgentHandle, GetStringNameFromHandle(pProtocolInfo.AgentHandle) ,pProtocolInfo.ControllerHandle, GetStringNameFromHandle(pProtocolInfo.ControllerHandle), pProtocolInfo.Attributes);
			}
	
			pBS->FreePool(pProtocolInfos);
		}

		pBS->FreePool(pProtocolBuffer);
	}

	pBS->FreePool(pHandleBuffer);

	return SHELL_SUCCESS;
}
