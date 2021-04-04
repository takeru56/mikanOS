#include  <Uefi.h>
#include  <Library/UefiLib.h>
#include  <Library/UefiBootServicesTableLib.h>
#include  <Library/PrintLib.h>
#include <Library/MemoryAllocationLib.h>
#include  <Protocol/LoadedImage.h>
#include  <Protocol/SimpleFileSystem.h>
#include  <Library/BaseMemoryLib.h>
#include  <Protocol/DiskIo2.h>
#include  <Protocol/BlockIo.h>
#include  <Guid/FileInfo.h>
#include "frame_buffer_config.hpp"
#include "elf.hpp"

// ブートローダー
// 起動時にUEFIアプリケーションとして呼ばれる
// UEFIがディスク上を探してメモリ上に呼び出し実行する

// 役割
// メモリ上の状態を確認
// カーネルをディスク上から呼び出してメモリ上に展開して実行

struct MemoryMap {
    UINTN buffer_size;
    VOID* buffer;
    UINTN map_size;
    UINTN map_key;
    UINTN descriptor_size;
    UINT32 descriptor_version;
};

EFI_STATUS GetMemoryMap(struct MemoryMap* map)
{
    if (map->buffer == NULL) {
        return EFI_BUFFER_TOO_SMALL;
    }

    map->map_size  = map->buffer_size;
    // 与えた引数が入力値として渡され，出力で上書きされる
    // ブートサービス
    // map->buffer上にEFI_MEMORY_DESCRIPTORの構造体が書き込まれる
    return gBS->GetMemoryMap(
        &map->map_size,
        (EFI_MEMORY_DESCRIPTOR*)map->buffer,
        &map->map_key,
        &map->descriptor_size,
        &map->descriptor_version
    );
}

const CHAR16* GetMemoryTypeUnicode(EFI_MEMORY_TYPE type)
{
    switch (type) {
        case EfiReservedMemoryType: return L"EfiReservedMemoryType";
        case EfiLoaderCode: return L"EfiLoaderCode";
        case EfiLoaderData: return L"EfiLoaderData";
        case EfiBootServicesCode: return L"EfiBootServicesCode";
        case EfiBootServicesData: return L"EfiBootServicesData";
        case EfiRuntimeServicesCode: return L"EfiRuntimeServicesCode";
        case EfiRuntimeServicesData: return L"EfiRuntimeServicesData";
        case EfiConventionalMemory: return L"EfiConventionalMemory";
        case EfiUnusableMemory: return L"EfiUnusableMemory";
        case EfiACPIReclaimMemory: return L"EfiACPIReclaimMemory";
        case EfiACPIMemoryNVS: return L"EfiACPIMemoryNVS";
        case EfiMemoryMappedIO: return L"EfiMemoryMappedIO";
        case EfiMemoryMappedIOPortSpace: return L"EfiMemoryMappedIOPortSpace";
        case EfiPalCode: return L"EfiPalCode";
        case EfiPersistentMemory: return L"EfiPersistentMemory";
        case EfiMaxMemoryType: return L"EfiMaxMemoryType";
        default: return L"InvalidMemoryType";
    }
}

EFI_STATUS SaveMemoryMap(struct MemoryMap* map, EFI_FILE_PROTOCOL* file)
{
    CHAR8 buf[256];
    UINTN len;

    CHAR8* header = "Index, Type, Type(name), PhysicalStart, NumberOfPages, Attribute\n";
    len = AsciiStrLen(header);
    file->Write(file, &len, header);

    Print(L"map->buffer = %08lx, map->map_size = %08lx\n",map->buffer, map->map_size);

    EFI_PHYSICAL_ADDRESS iter;
    int i;
    for (iter = (EFI_PHYSICAL_ADDRESS)map->buffer, i=0; iter < (EFI_PHYSICAL_ADDRESS)map->buffer + map->map_size; iter+=map->descriptor_size, i++){
        // 整数型であるiterをポインタ型であるEFI_DESCRIPTORにキャスト
        // その整数をアドレスとするポインタに変わる
        EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*) iter;
        // bufに整形した文字列を書き込んでくれる
        // lenに文字列のバイト数が返る
        len = AsciiSPrint(
            buf, sizeof(buf),
            "%u, %x, %-ls, %08lx, %lx, %lx\n",
            i, desc->Type, GetMemoryTypeUnicode(desc->Type),
            desc->PhysicalStart, desc->NumberOfPages,
            desc->Attribute & 0xffffflu
        );
        // fileにbufを書き込む
        // lenには入力として文字列のバイト数，返り値として実際に出力されたバイト数
        file->Write(file, &len, buf);
    }

    return EFI_SUCCESS;
}

EFI_STATUS OpenRootDir(EFI_HANDLE image_handle, EFI_FILE_PROTOCOL ** root)
{
    EFI_LOADED_IMAGE_PROTOCOL* loaded_image;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs;

    gBS->OpenProtocol(
        image_handle,
        &gEfiLoadedImageProtocolGuid,
        (VOID**)&loaded_image,
        image_handle,
        NULL,
        EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL
    );

    gBS->OpenProtocol(
        loaded_image->DeviceHandle,
        &gEfiSimpleFileSystemProtocolGuid,
        (VOID**)&fs,
        image_handle,
        NULL,
        EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL
    );

    fs->OpenVolume(fs, root);
    return EFI_SUCCESS;
}

EFI_STATUS OpenGOP(EFI_HANDLE image_handle, EFI_GRAPHICS_OUTPUT_PROTOCOL** gop)
{
    UINTN num_gop_handles = 0;
    EFI_HANDLE* gop_handles = NULL;
    // GOP機能を使用してピクセルを描画するのに必要な情報を取得する

    gBS->LocateHandleBuffer(
        ByProtocol,
        &gEfiGraphicsOutputProtocolGuid,
        NULL,
        &num_gop_handles,
        &gop_handles);

    gBS->OpenProtocol(
        gop_handles[0],
        &gEfiGraphicsOutputProtocolGuid,
        (VOID**)gop,
        image_handle,
        NULL,
        EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

    FreePool(gop_handles);

    return EFI_SUCCESS;
}

const CHAR16* GetPixelFormatUnicode(EFI_GRAPHICS_PIXEL_FORMAT fmt)
{
    switch (fmt) {
        case PixelRedGreenBlueReserved8BitPerColor:
            return L"PixelRedGreenBlueReserved8BitPerColor";
        case PixelBlueGreenRedReserved8BitPerColor:
            return L"PixelBlueGreenRedReserved8BitPerColor";
        case PixelBitMask:
            return L"PixelBitMask";
        case PixelBltOnly:
            return L"PixelBltOnly";
        case PixelFormatMax:
            return L"PixelFormatMax";
        default:
            return L"InvalidPixelFormat";
    }
}

void Halt(void)
{
    while (1) __asm__("hlt");
}

void CalcLoadAddressRange(Elf64_Ehdr* ehdr, UINT64* first, UINT64* last) {
    // プログラムヘッダのファイルオフセットを取得して，プログラムヘッダの先頭のアドレスを取得する
    Elf64_Phdr* phdr = (Elf64_Phdr*)((UINT64)ehdr + ehdr->e_phoff);
    *first = MAX_UINT64;
    *last = 0;
    // プログラムヘッダの配列を順に回り，LOADセグメントのfirstとlastを取得する
    for (Elf64_Half i = 0; i < ehdr->e_phnum; ++i) {
        if (phdr[i].p_type != PT_LOAD) continue;
        *first = MIN(*first, phdr[i].p_vaddr); // 仮想addr
        *last  = MAX(*last, phdr[i].p_vaddr + phdr[i].p_memsz); // 仮想addr+メモリサイズ
    }
}

void CopyLoadSegments(Elf64_Ehdr* ehdr) {
    Elf64_Phdr* phdr = (Elf64_Phdr*) ((UINT64)ehdr + ehdr->e_phoff);
    for (Elf64_Half i = 0; i < ehdr->e_phnum; ++i) {
        if (phdr[i].p_type !=PT_LOAD) continue;

        UINT64 segm_in_file = (UINT64)ehdr + phdr[i].p_offset;
        // segm_in_fileからp_vaddrにp_filesz分をcopyする
        CopyMem((VOID*)phdr[i].p_vaddr, (VOID*)segm_in_file, phdr[i].p_filesz);

        UINTN remain_bytes = phdr[i].p_memsz - phdr[i].p_filesz;
        SetMem((VOID*)(phdr[i].p_vaddr + phdr[i].p_filesz), remain_bytes, 0);
    }
}

EFI_STATUS EFIAPI UefiMain(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table)
{
    EFI_STATUS status;
    // *** メモリマップを取得する ***
    Print(L"Hello, Mikan World!!\n");
    CHAR8 memmap_buf[4096*4];
    struct MemoryMap memmap = {sizeof(memmap_buf), memmap_buf, 0, 0, 0, 0};
    GetMemoryMap(&memmap);

    EFI_FILE_PROTOCOL* root_dir;
    OpenRootDir(image_handle, &root_dir);

    EFI_FILE_PROTOCOL* memmap_file;
    root_dir->Open(root_dir, &memmap_file, L"\\memmap", EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);

    // memmapをcsv形式でファイルに書き出す
    SaveMemoryMap(&memmap, memmap_file);
    memmap_file->Close(memmap_file);

    // *** 画面を塗りつぶす ***
    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop;
    // gopに値をセットする
    OpenGOP(image_handle, &gop);
    Print(L"Resolution: %ux%u, Pixel Format: %s, %u pixels/line\n",
        gop->Mode->Info->HorizontalResolution,
        gop->Mode->Info->VerticalResolution,
        GetPixelFormatUnicode(gop->Mode->Info->PixelFormat),
        gop->Mode->Info->PixelsPerScanLine);
    Print(L"Frame Buffer: 0x%0lx - 0x%0lx, Size: %lu bytes\n",
        gop->Mode->FrameBufferBase,
        gop->Mode->FrameBufferBase + gop->Mode->FrameBufferSize,
        gop->Mode->FrameBufferSize);

    UINT8* frame_buffer = (UINT8*)gop->Mode->FrameBufferBase;
    for (UINTN i=0; i<gop->Mode->FrameBufferSize; ++i) {
        frame_buffer[i] = 255;
    }

    // *** カーネルファイルをよびだす ***
    EFI_FILE_PROTOCOL* kernel_file;
    root_dir->Open(
        root_dir,
        &kernel_file,
        L"\\kernel.elf",
        EFI_FILE_MODE_READ,
        0
    );

    // ファイル全体を読み込むためのメモリを確保するのに必要な処理．
    // まずはkernelファイルの大きさを取得する
    // https://github.com/tianocore/edk2/blob/0ecdcb6142037dd1cdd08660a2349960bcf0270a/MdePkg/Include/Guid/FileInfo.h
    UINTN file_info_size = sizeof(EFI_FILE_INFO) + sizeof(CHAR16) * 12;
    UINT8 file_info_buffer[file_info_size];
    kernel_file->GetInfo(
        kernel_file, &gEfiFileInfoGuid, &file_info_size, file_info_buffer);

    EFI_FILE_INFO* file_info = (EFI_FILE_INFO*) file_info_buffer;
    UINTN kernel_file_size = file_info->FileSize;

    // 一時的にファイルを配置するメモリを確保し配置する
    VOID* kernel_buffer;
    status = gBS->AllocatePool(EfiLoaderData, kernel_file_size, &kernel_buffer);
    if (EFI_ERROR(status)) {
        Print(L"failed to allocate pool: %r\n", status);
        Halt();
    }
    status = kernel_file->Read(kernel_file, &kernel_file_size, kernel_buffer);
    if (EFI_ERROR(status)) {
        Print(L"error: %r\n", status);
        Halt();
    }

    Elf64_Ehdr* kernel_ehdr = (Elf64_Ehdr*)kernel_buffer;
    UINT64 kernel_first_addr, kernel_last_addr;

    // 最終目的地に書き込む範囲を取得する
    CalcLoadAddressRange(kernel_ehdr, &kernel_first_addr, &kernel_last_addr);

    UINTN num_pages = (kernel_last_addr - kernel_first_addr + 0xfff) / 0x1000;
    // 最終目的地の容量を確保
    status = gBS->AllocatePages(AllocateAddress, EfiLoaderData, num_pages, &kernel_first_addr);

    if (EFI_ERROR(status)) {
        Print(L"failed to allocate pages: %r\n", status);
        Halt();
    }

    // 一時領域から最終目的領域へのコピー
    CopyLoadSegments(kernel_ehdr);
    Print(L"Kernel: 0x%olx - 0x%0lx\n", kernel_first_addr, kernel_last_addr);

    // 一時領域を開放
    status = gBS->FreePool(kernel_buffer);
    if (EFI_ERROR(status)) {
        Print(L"failed to free pool: %r\n", status);
        Halt();
    }

    // カーネル起動前にブートサービスを停止する
    // 停止
    // memmap.map_keyは刻々と変わるので1回目の処理は失敗しがち
    status = gBS->ExitBootServices(image_handle, memmap.map_key);
    if (EFI_ERROR(status)) {
        // memmapを取得してやり直す
        // 直近のmemmapなら失敗しない
        status = GetMemoryMap(&memmap);
        if (EFI_ERROR(status)) {
            // それでも失敗したら致命的なエラー．永久ループにより停止
            Print(L"failed to get memory map: %r\n", status);
            while(1);
        }
        status = gBS->ExitBootServices(image_handle, memmap.map_key);
        if (EFI_ERROR(status)) {
            Print(L"Could not exit boot service: %r\n", status);
            while(1);
        }
    }

    // *** カーネルの起動 ***
    UINT64 entry_addr = *(UINT64*) (kernel_first_addr + 24);

    struct FrameBufferConfig config = {
        (UINT8*) gop->Mode->FrameBufferBase,
        gop->Mode->Info->PixelsPerScanLine,
        gop->Mode->Info->HorizontalResolution,
        gop->Mode->Info->VerticalResolution,
        0
    };

    switch (gop->Mode->Info->PixelFormat) {
        case PixelRedGreenBlueReserved8BitPerColor:
            config.pixel_format = kPixelRGBResv8BitPerColor;
            break;
        case PixelBlueGreenRedReserved8BitPerColor:
            config.pixel_format = kPixelBGRResv8BitPerColor;
            break;
        default:
            Print(L"Unimplemented pixel format: %d\n", gop->Mode->Info->PixelFormat);
            Halt();
    }

    // 関数の先頭アドレスと引数の型，返り値の型を組み合わせて，任意の場所から命令を実行できる
    typedef void EntryPointType(const struct FrameBufferConfig*);
    EntryPointType* entry_point = (EntryPointType*)entry_addr;
    // 実行
    entry_point(&config);
    while(1);
    return EFI_SUCCESS;
}
