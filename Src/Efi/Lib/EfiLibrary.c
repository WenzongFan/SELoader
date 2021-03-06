/*
 * BSD 2-clause "Simplified" License
 *
 * Copyright (c) 2017, Lans Zhang <jia.zhang@windriver.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <Efi.h>
#include <EfiLibrary.h>
#include <BaseLibrary.h>

#include "Internal.h"

EFI_SYSTEM_TABLE *gST;
EFI_BOOT_SERVICES *gBS;
EFI_RUNTIME_SERVICES *gRT;
EFI_HANDLE gThisImage;
EFI_HANDLE gThisDevice;
CHAR16 *gRootPath;

extern CONST CHAR16 *GitCommit;

EFI_STATUS
EfiLibraryInitialize(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
	if (!ImageHandle || !SystemTable)
		return EFI_INVALID_PARAMETER;

#ifdef GNU_EFI_VERSION
	InitializeLib(ImageHandle, SystemTable);
#endif
	gST = SystemTable;
	gBS = SystemTable->BootServices;
	gRT = SystemTable->RuntimeServices;
	gThisImage = ImageHandle;

	EFI_STATUS Status;

	Status = EfiDeviceLocate(&gThisDevice);
	if (EFI_ERROR(Status))
		return Status;

	Status = EfiDevicePathRootDirectory(&gRootPath);
	if (EFI_ERROR(Status))
		return Status;

#ifdef TRACE_BUILD
	EfiConsoleTraceInfo(L"Traced on\nPress any key to continue "
			    L"when prompted with \">>|\"\n");
#endif

	EfiConsolePrintInfo(L"SELoader " SEL_VERSION "+git%s launched\n",
			    GitCommit);

	EfiConsolePrintLevel Level;

	Status = EfiConsoleGetVerbosity(&Level);
	if (!EFI_ERROR(Status)) {
		if (Level != CPL_DEBUG)
			SecurityPolicyInitialize();
		else
			EfiSecurityPolicyPrint();
	}

	return Status;
}

VOID
EfiLibraryHexDump(CONST CHAR16 *Prompt, UINT8 *Data, UINTN DataSize)
{
	if (DataSize && !Data)
		return;

	if (Prompt)
		EfiConsolePrintDebug(L"%s (%d-byte): ", Prompt, DataSize);

	for (UINTN Index = 0; Index < DataSize; ++Index)
		EfiConsolePrintDebug(L"%02x", Data[Index]);

	EfiConsolePrintDebug(L"\n");
}

EFI_STATUS
EfiLibraryVectorizedBufferEnter(VOID **Buffer, UINTN *BufferSize)
{
	if (Buffer && !BufferSize)
		return EFI_INVALID_PARAMETER;

	if (Buffer && *Buffer && BufferSize &&
	    !*BufferSize)
		return EFI_INVALID_PARAMETER;

	if (!Buffer && BufferSize && *BufferSize)
		return EFI_INVALID_PARAMETER;

	return EFI_SUCCESS;
}

EFI_STATUS
EfiLibraryVectorizedBufferLeave(VOID **Buffer, UINTN *BufferSize,
				VOID *Data, UINTN DataSize,
				BOOLEAN Duplication)
{
	if (BufferSize) {
		if (!*BufferSize)
			*BufferSize = DataSize;
		else
			*BufferSize = MIN(*BufferSize, DataSize);
	}

	if (Buffer) {
		if (!*Buffer) {
			if (Duplication == FALSE) {
				*Buffer = Data;
				return EFI_SUCCESS;
			} else {
				EFI_STATUS Status;

				Status = EfiMemoryAllocate(*BufferSize,
							   Buffer);
				if (EFI_ERROR(Status))
					return Status;
			}
		}

		MemCpy(*Buffer, Data, *BufferSize);
	}

	return EFI_SUCCESS;
}
