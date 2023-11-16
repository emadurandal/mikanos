#include  <Uefi.h>
#include  <Library/UefiLib.h>
#include  <Library/UefiBootServicesTableLib.h>
#include  <Library/PrintLib.h>
#include  <Library/MemoryAllocationLib.h>
#include  <Protocol/LoadedImage.h>
#include  <Protocol/SimpleFileSystem.h>
#include  <Protocol/DiskIo2.h>
#include  <Protocol/BlockIo.h>
#include  <Guid/FileInfo.h>

// #@@range_begin(struct_memory_map)
// 独自のメモリマップ構造体
struct MemoryMap {
  UINTN buffer_size;
  VOID* buffer;
  UINTN map_size;
  UINTN map_key;
  UINTN descriptor_size;
  UINT32 descriptor_version;
};

// #@@range_begin(get_memory_map)
// メモリマップを取得する関数
EFI_STATUS GetMemoryMap(struct MemoryMap* map) {
  // メモリマップ構造体側にバッファが準備されていない場合は、エラーを返す
  if (map->buffer == NULL) {
    return EFI_BUFFER_TOO_SMALL;
  }

  // GetMemoryMap()を呼び出す前に、map->map_sizeにバッファのサイズを設定しておく
  map->map_size = map->buffer_size;

  // gBSのGetMemoryMap()を呼び出す
  return gBS->GetMemoryMap(
      &map->map_size,
      (EFI_MEMORY_DESCRIPTOR*)map->buffer,
      &map->map_key,
      &map->descriptor_size,
      &map->descriptor_version);
}

// #@@range_begin(get_memory_type)
// メモリタイプEnum値を文字列に変換する関数
const CHAR16* GetMemoryTypeUnicode(EFI_MEMORY_TYPE type) {
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

EFI_STATUS SaveMemoryMap(struct MemoryMap* map, EFI_FILE_PROTOCOL* file) {
  CHAR8 buf[256];
  UINTN len;

  // CSVのヘッダー行をファイルに書き込む
  CHAR8* header =
    "Index, Type, Type(name), PhysicalStart, NumberOfPages, Attribute\n";
  len = AsciiStrLen(header);
  file->Write(file, &len, header);

  // 画面にデバッグ出力
  Print(L"map->buffer = %08lx, map->map_size = %08lx\n",
      map->buffer, map->map_size);

  // map->bufferはEFI_MEMORY_DESCRIPTORの配列なので、descriptor_sizeバイトごとにiterを増やすループ
  EFI_PHYSICAL_ADDRESS iter;
  int i;
  for (iter = (EFI_PHYSICAL_ADDRESS)map->buffer, i = 0;
       iter < (EFI_PHYSICAL_ADDRESS)map->buffer + map->map_size;
       iter += map->descriptor_size, i++) {
    EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)iter;

    // カンマ区切り（CSV）形式でメモリマップの各情報をバッファbufに書き込む
    len = AsciiSPrint(
        buf, sizeof(buf),
        "%u, %x, %-ls, %08lx, %lx, %lx\n",
        i, desc->Type, GetMemoryTypeUnicode(desc->Type),
        desc->PhysicalStart, desc->NumberOfPages,
        desc->Attribute & 0xffffflu);

    // バッファbufの内容をファイルに書き込む
    file->Write(file, &len, buf);
  }

  return EFI_SUCCESS;
}

EFI_STATUS OpenRootDir(EFI_HANDLE image_handle, EFI_FILE_PROTOCOL** root) {
  EFI_LOADED_IMAGE_PROTOCOL* loaded_image;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs;

  gBS->OpenProtocol(
      image_handle,
      &gEfiLoadedImageProtocolGuid,
      (VOID**)&loaded_image,
      image_handle,
      NULL,
      EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

  gBS->OpenProtocol(
      loaded_image->DeviceHandle,
      &gEfiSimpleFileSystemProtocolGuid,
      (VOID**)&fs,
      image_handle,
      NULL,
      EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

  fs->OpenVolume(fs, root);

  return EFI_SUCCESS;
}

EFI_STATUS OpenGOP(EFI_HANDLE image_handle,
                   EFI_GRAPHICS_OUTPUT_PROTOCOL** gop) {
  UINTN num_gop_handles = 0;
  EFI_HANDLE* gop_handles = NULL;
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

const CHAR16* GetPixelFormatUnicode(EFI_GRAPHICS_PIXEL_FORMAT fmt) {
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


EFI_STATUS EFIAPI UefiMain(
    EFI_HANDLE image_handle,
    EFI_SYSTEM_TABLE* system_table) {
  Print(L"Hello, Mikan World!\n");

  // #@@range_begin(main)
  // メモリマップ取得
  CHAR8 memmap_buf[4096 * 4];
  struct MemoryMap memmap = {sizeof(memmap_buf), memmap_buf, 0, 0, 0, 0};
  GetMemoryMap(&memmap);

  // まずはルートディレクトリを開く
  EFI_FILE_PROTOCOL* root_dir;
  OpenRootDir(image_handle, &root_dir);

  // memmapというファイルを作って開く
  EFI_FILE_PROTOCOL* memmap_file;
  root_dir->Open(
      root_dir, &memmap_file, L"\\memmap",
      EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);

  // メモリマップをmemmapファイルに書き込む
  SaveMemoryMap(&memmap, memmap_file);

  // memmapファイルを閉じる
  memmap_file->Close(memmap_file);

  EFI_GRAPHICS_OUTPUT_PROTOCOL* gop;
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
  for (UINTN i = 0; i < gop->Mode->FrameBufferSize; ++i) {
    frame_buffer[i] = 255;
  }

  // #@@range_begin(read_kernel)
  // kernel.elfを開く
  EFI_FILE_PROTOCOL* kernel_file;
  root_dir->Open(
      root_dir, &kernel_file, L"\\kernel.elf",
      EFI_FILE_MODE_READ, 0);

  // 開いたkernel.elfファイル全体を読み込むには、そのファイルサイズを知る必要がある。
  // そのためファイルサイズを取得するためkernel_file->GetInfo()を呼び出す。
  // その情報の受け皿となるfile_info_bufferを用意する。
  // sizeof(CHAR16) * 12はファイル名の文字数分のバッファを確保するためのもので、
  // 詳しくは書籍p75を参照。
  UINTN file_info_size = sizeof(EFI_FILE_INFO) + sizeof(CHAR16) * 12;
  UINT8 file_info_buffer[file_info_size];

  // kernel_file->GetInfo()を呼び出してファイルサイズを取得する。
  kernel_file->GetInfo(
      kernel_file, &gEfiFileInfoGuid,
      &file_info_size, file_info_buffer);

  // file_info_bufferはUINT8型なので、EFI_FILE_INFO*型にキャストする。
  EFI_FILE_INFO* file_info = (EFI_FILE_INFO*)file_info_buffer;
  // ファイルサイズはfile_info->FileSizeに格納されている。
  UINTN kernel_file_size = file_info->FileSize;


  // kernel.elfを読み込むためのメモリを確保する。
  // ld.lldのオプション--image-baseで指定したアドレスをkernel_base_addrに設定する。
  // カーネルファイルはこの番地に配置して動作させる前提で作ってあるということ。
  EFI_PHYSICAL_ADDRESS kernel_base_addr = 0x100000;
  gBS->AllocatePages(
      AllocateAddress, EfiLoaderData, // AllocateAddressは指定したアドレスにメモリ確保せよという意味、EfiLoaderDataは確保するメモリ領域の種別でブートローダーが使うための領域なら通常はこれを指定すれば良い
      (kernel_file_size + 0xfff) / 0x1000, &kernel_base_addr); // (kernel_file_size + 0xfff) / 0x1000はカーネルファイルサイズをバイト単位からページ単位に変換する計算式。詳しくは書籍p77を参照。

  // kernel.elfをkernel_base_addrに読み込む。  
  kernel_file->Read(kernel_file, &kernel_file_size, (VOID*)kernel_base_addr);
  // 画面にデバッグ出力
  Print(L"Kernel: 0x%0lx (%lu bytes)\n", kernel_base_addr, kernel_file_size);


  // #@@range_end(read_kernel)

  // #@@range_begin(exit_bs)
  // OSの邪魔にならないように、今まで動いていたUEFI BIOSのブートサービスを停止する
  // この後はブートサービスの機能（Print()やファイルやメモリ関連の機能）は使えなくなる
  // この関数はその時点での最新のメモリマップのマップキーを要求する（第２引数）
  // gBS->ExitBootServices()は指定されたマップキーが最近のメモリマップに紐づくマップキーでない場合、実行に失敗する
  // 色々ブートろサービスの機能を使っているので、おそらく一回目は失敗する
  EFI_STATUS status;
  status = gBS->ExitBootServices(image_handle, memmap.map_key);
  if (EFI_ERROR(status)) {
    // 失敗した場合は、再度メモリマップを所得する
    status = GetMemoryMap(&memmap);
    if (EFI_ERROR(status)) {
      Print(L"failed to get memory map: %r\n", status);
      while (1);
    }
    // 更新したマップキーで再度ブートサービスの停止を試みる。これは成功するはず。
    status = gBS->ExitBootServices(image_handle, memmap.map_key);
    if (EFI_ERROR(status)) {
      Print(L"Could not exit boot service: %r\n", status);
      while (1);
    }
  }
  // #@@range_end(exit_bs)

  // カーネルのエントリポイントアドレスを取得する
  UINT64 entry_addr = *(UINT64*)(kernel_base_addr + 24);

  // #@@range_begin(call_kernel)
  // エントリポイントのC言語としての関数プロトタイプ情報を定義する
  typedef void EntryPointType(UINT64, UINT64);
  // エントリポイントのアドレスを関数ポインタとしてキャスト
  EntryPointType* entry_point = (EntryPointType*)entry_addr;
  // カーネルのエントリポイントを呼び出す
  entry_point(gop->Mode->FrameBufferBase, gop->Mode->FrameBufferSize);
  // #@@range_end(call_kernel)

  Print(L"All done\n");

  while (1);
  return EFI_SUCCESS;
}
