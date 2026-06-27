#ifndef PE_H
#define PE_H

#include "utils.h"
#include "gen.h"

#define PE_PAGE_SIZE         0x1000
#define PE_FILE_ALIGNMENT    0x200
#define PE_SECTION_ALIGNMENT 0x1000
#define PE_IMAGE_BASE        0x140000000ULL

#define DOS_MAGIC    0x5A4D
#define PE_SIGNATURE 0x00004550

#define COFF_MACHINE_AMD64   0x8664
#define COFF_MACHINE_I386    0x014C

#define PE_OPT_MAGIC_PE32PLUS 0x020B

#define IMAGE_SCN_CNT_CODE               0x00000020
#define IMAGE_SCN_CNT_INITIALIZED_DATA   0x00000040
#define IMAGE_SCN_MEM_EXECUTE            0x20000000
#define IMAGE_SCN_MEM_READ               0x40000000
#define IMAGE_SCN_MEM_WRITE              0x80000000

#define PE_SUBSYSTEM_WINDOWS_CUI 3
#define PE_SUBSYSTEM_WINDOWS_GUI 2

#pragma pack(push, 1)

typedef struct {
    uint16_t e_magic;
    uint16_t e_cblp;
    uint16_t e_cp;
    uint16_t e_crlc;
    uint16_t e_cparhdr;
    uint16_t e_minalloc;
    uint16_t e_maxalloc;
    uint16_t e_ss;
    uint16_t e_sp;
    uint16_t e_csum;
    uint16_t e_ip;
    uint16_t e_cs;
    uint16_t e_lfarlc;
    uint16_t e_ovno;
    uint16_t e_res[4];
    uint16_t e_oemid;
    uint16_t e_oeminfo;
    uint16_t e_res2[10];
    int32_t  e_lfanew;
} PEDosHeader;

typedef struct {
    uint16_t Machine;
    uint16_t NumberOfSections;
    uint32_t TimeDateStamp;
    uint32_t PointerToSymbolTable;
    uint32_t NumberOfSymbols;
    uint16_t SizeOfOptionalHeader;
    uint16_t Characteristics;
} PECoffHeader;

typedef struct {
    uint16_t Magic;
    uint8_t  MajorLinkerVersion;
    uint8_t  MinorLinkerVersion;
    uint32_t SizeOfCode;
    uint32_t SizeOfInitializedData;
    uint32_t SizeOfUninitializedData;
    uint32_t AddressOfEntryPoint;
    uint32_t BaseOfCode;
    uint64_t ImageBase;
    uint32_t SectionAlignment;
    uint32_t FileAlignment;
    uint16_t MajorOperatingSystemVersion;
    uint16_t MinorOperatingSystemVersion;
    uint16_t MajorImageVersion;
    uint16_t MinorImageVersion;
    uint16_t MajorSubsystemVersion;
    uint16_t MinorSubsystemVersion;
    uint32_t Win32VersionValue;
    uint32_t SizeOfImage;
    uint32_t SizeOfHeaders;
    uint32_t CheckSum;
    uint16_t Subsystem;
    uint16_t DllCharacteristics;
    uint64_t SizeOfStackReserve;
    uint64_t SizeOfStackCommit;
    uint64_t SizeOfHeapReserve;
    uint64_t SizeOfHeapCommit;
    uint32_t LoaderFlags;
    uint32_t NumberOfRvaAndSizes;
    uint64_t DataDirectory[32];
} PEOptionalHeader;

typedef struct {
    char     Name[8];
    uint32_t VirtualSize;
    uint32_t VirtualAddress;
    uint32_t SizeOfRawData;
    uint32_t PointerToRawData;
    uint32_t PointerToRelocations;
    uint32_t PointerToLinenumbers;
    uint16_t NumberOfRelocations;
    uint16_t NumberOfLinenumbers;
    uint32_t Characteristics;
} PESectionHeader;

#pragma pack(pop)

typedef struct {
    uint8_t *data;
    int      len;
    int      cap;
} PEBuffer;

typedef struct {
    PEDosHeader    dos;
    PECoffHeader   coff;
    PEOptionalHeader opt;
    PESectionHeader  text_section;

    uint8_t *text_data;
    int      text_len;

    PEBuffer buf;
} PEFile;

void pe_init(PEFile *pe);
void pe_free(PEFile *pe);

void pe_set_entry(PEFile *pe, uint32_t rva);

void pe_text_write(PEFile *pe, const void *data, int len);
void pe_text_write_byte(PEFile *pe, uint8_t b);

void pe_text_write_u32(PEFile *pe, uint32_t val);
void pe_text_write_u64(PEFile *pe, uint64_t val);

int  pe_build(PEFile *pe);
int  pe_write_to_file(PEFile *pe, const char *path);

void pe_emit_prologue(PEFile *pe, int stack_size);
void pe_emit_epilogue(PEFile *pe);
void pe_emit_mov_rax_imm64(PEFile *pe, uint64_t val);
void pe_emit_ret(PEFile *pe);
void pe_emit_sub_rsp(PEFile *pe, int32_t val);
void pe_emit_add_rsp(PEFile *pe, int32_t val);
void pe_emit_push_rbp(PEFile *pe);
void pe_emit_mov_rbp_rsp(PEFile *pe);
void pe_emit_mov_rsp_rbp(PEFile *pe);
void pe_emit_pop_rbp(PEFile *pe);

void pe_encode_rex(PEFile *pe, int w, int r, int x, int b);
void pe_encode_modrm(PEFile *pe, int mod, int reg, int rm);
void pe_encode_sib(PEFile *pe, int scale, int index, int base);

void pe_emit_mov_reg_imm32(PEFile *pe, int reg, int32_t val);
void pe_emit_mov_reg_reg(PEFile *pe, int dst, int src);
void pe_emit_add_reg_imm32(PEFile *pe, int reg, int32_t val);
void pe_emit_sub_reg_imm32(PEFile *pe, int reg, int32_t val);
void pe_emit_xor_reg_reg(PEFile *pe, int dst, int src);

void pe_emit_call_rel32(PEFile *pe, int32_t rel);
void pe_emit_jmp_rel32(PEFile *pe, int32_t rel);
void pe_emit_nop(PEFile *pe);

#endif
