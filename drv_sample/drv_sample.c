// sample_driver.c - Minimal WDM sample driver (C, WDK)
// Device name:     \Device\Sample
// Symlink name:    \DosDevices\sample   (user-mode: \\.\sample)
//
// Echo logic:
//  - WRITE stores data into internal ring-less buffer (simple linear buffer)
//  - READ returns buffered data (consumes it)
//
// IOCTLs:
//  - IOCTL_SAMPLE_GET_VERSION  -> returns DWORD version
//  - IOCTL_SAMPLE_CLEAR_BUFFER -> clears internal buffer
//
// Build: WDK (Visual Studio) -> Kernel Mode Driver, Empty (WDM)
// Add this file, set as C/C++ source, link as driver (sys)

#include <ntddk.h>

// ---------------- IOCTL definitions (must match user-mode) ----------------
#define FILE_DEVICE_SAMPLE  0x8000

#define IOCTL_SAMPLE_GET_VERSION \
    CTL_CODE(FILE_DEVICE_SAMPLE, 0x801, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_SAMPLE_CLEAR_BUFFER \
    CTL_CODE(FILE_DEVICE_SAMPLE, 0x802, METHOD_BUFFERED, FILE_WRITE_ACCESS)

// ---------------- Device extension ----------------
#define SAMPLE_BUFFER_SIZE (4096)

typedef struct _SAMPLE_DEVICE_EXTENSION {
    FAST_MUTEX  Lock;
    UCHAR       Buffer[SAMPLE_BUFFER_SIZE];
    ULONG       DataLen; // number of valid bytes in Buffer
} SAMPLE_DEVICE_EXTENSION, * PSAMPLE_DEVICE_EXTENSION;

// ---------------- Globals ----------------
static const WCHAR g_DeviceName[] = L"\\Device\\Sample";
static const WCHAR g_SymLinkName[] = L"\\DosDevices\\sample";

// ---------------- Forward decls ----------------
DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD     SampleUnload;

_Dispatch_type_(IRP_MJ_CREATE)
DRIVER_DISPATCH SampleCreateClose;

_Dispatch_type_(IRP_MJ_READ)
DRIVER_DISPATCH SampleRead;

_Dispatch_type_(IRP_MJ_WRITE)
DRIVER_DISPATCH SampleWrite;

_Dispatch_type_(IRP_MJ_DEVICE_CONTROL)
DRIVER_DISPATCH SampleDeviceControl;

NTSTATUS SampleCompleteIrp(_Inout_ PIRP Irp, _In_ NTSTATUS Status, _In_ ULONG_PTR Info)
{
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = Info;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

// ---------------- Dispatch: CREATE/CLOSE ----------------
NTSTATUS SampleCreateClose(_In_ PDEVICE_OBJECT DeviceObject, _Inout_ PIRP Irp)
{
    DbgPrint("\n>>>>>>> [*] IRP_CREATE!");
    UNREFERENCED_PARAMETER(DeviceObject);
    return SampleCompleteIrp(Irp, STATUS_SUCCESS, 0);
}

// ---------------- Dispatch: WRITE (store into internal buffer) ----------------
NTSTATUS SampleWrite(_In_ PDEVICE_OBJECT DeviceObject, _Inout_ PIRP Irp)
{
    PSAMPLE_DEVICE_EXTENSION ext = (PSAMPLE_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION      isl = IoGetCurrentIrpStackLocation(Irp);
    ULONG                   len = isl->Parameters.Write.Length;

    DbgPrint("\n>>>>>>> [*] IRP_WRITE!");
    // Using DO_BUFFERED_IO -> Irp->AssociatedIrp.SystemBuffer is valid
    PUCHAR inBuf = (PUCHAR)Irp->AssociatedIrp.SystemBuffer;
    if (!inBuf || len == 0)
        return SampleCompleteIrp(Irp, STATUS_SUCCESS, 0);

    ExAcquireFastMutex(&ext->Lock);

    // If incoming > buffer, keep only last bytes (simple behavior)
    if (len > 0 && len >= SAMPLE_BUFFER_SIZE) {
        RtlCopyMemory(ext->Buffer, inBuf + (len - SAMPLE_BUFFER_SIZE), SAMPLE_BUFFER_SIZE);
        ext->DataLen = SAMPLE_BUFFER_SIZE;
        ExReleaseFastMutex(&ext->Lock);

        ext->Buffer[ext->DataLen - 1] = 0;
        DbgPrint("\n>>>>>>> [*] %d bytes: %s <- 0x%llx [0x%llx]!", ext->Buffer, inBuf, ext->Buffer);

        return SampleCompleteIrp(Irp, STATUS_SUCCESS, len);
    }

    // If not enough free space, shift left (consume oldest) to make room
    if (ext->DataLen + len > SAMPLE_BUFFER_SIZE) {
        ULONG overflow = (ext->DataLen + len) - SAMPLE_BUFFER_SIZE;
        // shift remaining data to front
        if (overflow < ext->DataLen) {
            RtlMoveMemory(ext->Buffer, ext->Buffer + overflow, ext->DataLen - overflow);
            ext->DataLen -= overflow;
        }
        else {
            ext->DataLen = 0;
        }
    }

    // Append
    RtlCopyMemory(ext->Buffer + ext->DataLen, inBuf, len);
    DbgPrint("\n>>>>>>> [*] %d bytes: %s <- 0x%llx [0x%llx]!", ext->Buffer, inBuf, ext->Buffer);

    ext->DataLen += len;

    ExReleaseFastMutex(&ext->Lock);

    return SampleCompleteIrp(Irp, STATUS_SUCCESS, len);
}

// ---------------- Dispatch: READ (return and consume) ----------------
NTSTATUS SampleRead(_In_ PDEVICE_OBJECT DeviceObject, _Inout_ PIRP Irp)
{
    PSAMPLE_DEVICE_EXTENSION ext = (PSAMPLE_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION      isl = IoGetCurrentIrpStackLocation(Irp);
    ULONG                   outLen = isl->Parameters.Read.Length;

    DbgPrint("\n>>>>>>> [*] IRP_READ!");

    PUCHAR outBuf = (PUCHAR)Irp->AssociatedIrp.SystemBuffer;
    if (!outBuf || outLen == 0)
        return SampleCompleteIrp(Irp, STATUS_SUCCESS, 0);

    ExAcquireFastMutex(&ext->Lock);

    ULONG toCopy = (ext->DataLen < outLen) ? ext->DataLen : outLen;

    if (toCopy > 0) {
        RtlCopyMemory(outBuf, ext->Buffer, toCopy);
        DbgPrint("\n>>>>>>> [*] %d bytes done -> 0x%llx [0x%llx]!", toCopy, outBuf, ext->Buffer);
        // consume copied bytes
        if (toCopy < ext->DataLen) {
            RtlMoveMemory(ext->Buffer, ext->Buffer + toCopy, ext->DataLen - toCopy);
        }
        ext->DataLen -= toCopy;
    }

    ExReleaseFastMutex(&ext->Lock);

    return SampleCompleteIrp(Irp, STATUS_SUCCESS, toCopy);
}

// ---------------- Dispatch: DEVICE_CONTROL ----------------
NTSTATUS SampleDeviceControl(_In_ PDEVICE_OBJECT DeviceObject, _Inout_ PIRP Irp)
{
    PSAMPLE_DEVICE_EXTENSION ext = (PSAMPLE_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION isl = IoGetCurrentIrpStackLocation(Irp);
    ULONG code = isl->Parameters.DeviceIoControl.IoControlCode;

    // METHOD_BUFFERED: input/output are in Irp->AssociatedIrp.SystemBuffer
    PVOID sysBuf = Irp->AssociatedIrp.SystemBuffer;
    ULONG inLen = isl->Parameters.DeviceIoControl.InputBufferLength;
    ULONG outLen = isl->Parameters.DeviceIoControl.OutputBufferLength;

    if (KD_DEBUGGER_NOT_PRESENT == FALSE) {
        DbgBreakPoint();
    }
    DbgPrint("\n>>>>>>> [*] IRP_IOCTRL:[0x%x] 0x%llx in=%ld out=%ld!", (int)code, sysBuf, inLen, outLen);

    switch (code)
    {
    case IOCTL_SAMPLE_GET_VERSION:
        if (outLen < sizeof(ULONG) || sysBuf == NULL) {
            return SampleCompleteIrp(Irp, STATUS_BUFFER_TOO_SMALL, 0);
        }
        else {
            *(ULONG*)sysBuf = 0x00010000; // v1.0 in hex-like form
            return SampleCompleteIrp(Irp, STATUS_SUCCESS, sizeof(ULONG));
        }

    case IOCTL_SAMPLE_CLEAR_BUFFER:
        // no input required
        UNREFERENCED_PARAMETER(inLen);

        ExAcquireFastMutex(&ext->Lock);
        ext->DataLen = 0;
        ExReleaseFastMutex(&ext->Lock);

        return SampleCompleteIrp(Irp, STATUS_SUCCESS, 0);

    default:
        return SampleCompleteIrp(Irp, STATUS_INVALID_DEVICE_REQUEST, 0);
    }
}

// ---------------- Unload ----------------
VOID SampleUnload(_In_ PDRIVER_OBJECT DriverObject)
{
    UNICODE_STRING symLink;

    DbgPrint("\n>>>>>>> [*] DRIVER_UNLOAD!");

    RtlInitUnicodeString(&symLink, g_SymLinkName);
    IoDeleteSymbolicLink(&symLink);

    if (DriverObject->DeviceObject) {
        IoDeleteDevice(DriverObject->DeviceObject);
    }
}

// ---------------- DriverEntry ----------------
NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(RegistryPath);

    NTSTATUS status;
    PDEVICE_OBJECT deviceObject = NULL;
    UNICODE_STRING devName, symLink;

    DbgPrint("\n>>>>>>> [*] DRIVER_ENTRY!");
    
    RtlInitUnicodeString(&devName, g_DeviceName);

    status = IoCreateDevice(
        DriverObject,
        sizeof(SAMPLE_DEVICE_EXTENSION),
        &devName,
        FILE_DEVICE_SAMPLE,
        0,
        FALSE,
        &deviceObject);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    DbgPrint("\n>>>>>>> [*] Device created!");

    // Use buffered I/O so we can use Irp->AssociatedIrp.SystemBuffer for read/write/ioctl
    deviceObject->Flags |= DO_BUFFERED_IO;

    // Init extension
    PSAMPLE_DEVICE_EXTENSION ext = (PSAMPLE_DEVICE_EXTENSION)deviceObject->DeviceExtension;
    ExInitializeFastMutex(&ext->Lock);
    ext->DataLen = 0;

    // Create user-visible symlink
    RtlInitUnicodeString(&symLink, g_SymLinkName);
    status = IoCreateSymbolicLink(&symLink, &devName);
    if (!NT_SUCCESS(status)) {
        IoDeleteDevice(deviceObject);
        return status;
    }

    // Dispatch table
    DriverObject->MajorFunction[IRP_MJ_CREATE] = SampleCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = SampleCreateClose;
    DriverObject->MajorFunction[IRP_MJ_READ] = SampleRead;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = SampleWrite;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = SampleDeviceControl;
    
    DbgPrint("\n>>>>>>> [*] Callbacks are ok!");

    DriverObject->DriverUnload = SampleUnload;

    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    return STATUS_SUCCESS;
}
