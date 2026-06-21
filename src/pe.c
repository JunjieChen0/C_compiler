#include "pe.h"
#include <string.h>

static void buf_init(PEBuffer *b) {
    b->data = NULL;
    b->len = 0;
    b->cap = 0;
}

static void buf_free(PEBuffer *b) {
    free(b->data);
    b->data = NULL;
    b->len = 0;
    b->cap = 0;
}

static void buf_grow(PEBuffer *b, int need) {
    if (b->len + need <= b->cap) return;
    int new_cap = b->cap ? b->cap * 2 : 4096;
    while (new_cap < b->len + need) new_cap *= 2;
    b->data = xrealloc(b->data, new_cap);
    b->cap = new_cap;
}

static void buf_write(PEBuffer *b, const void *data, int len) {
    buf_grow(b, len);
    memcpy(b->data + b->len, data, len);
    b->len += len;
}

static void buf_pad(PEBuffer *b, int count) {
    buf_grow(b, count);
    memset(b->data + b->len, 0, count);
    b->len += count;
}

static void buf_write_u32(PEBuffer *b, uint32_t v) {
    buf_write(b, &v, 4);
}

static int align_up(int val, int align) {
    return (val + align - 1) & ~(align - 1);
}

void pe_init(PEFile *pe) {
    memset(pe, 0, sizeof(*pe));
    buf_init(&pe->buf);

    pe->dos.e_magic = DOS_MAGIC;
    pe->dos.e_lfanew = 64;

    pe->coff.Machine = COFF_MACHINE_AMD64;
    pe->coff.NumberOfSections = 1;
    pe->coff.SizeOfOptionalHeader = sizeof(PEOptionalHeader);
    pe->coff.Characteristics = 0x0022;

    pe->opt.Magic = PE_OPT_MAGIC_PE32PLUS;
    pe->opt.MajorLinkerVersion = 14;
    pe->opt.MinorLinkerVersion = 0;
    pe->opt.ImageBase = PE_IMAGE_BASE;
    pe->opt.SectionAlignment = PE_SECTION_ALIGNMENT;
    pe->opt.FileAlignment = PE_FILE_ALIGNMENT;
    pe->opt.MajorOperatingSystemVersion = 6;
    pe->opt.MinorOperatingSystemVersion = 0;
    pe->opt.MajorSubsystemVersion = 6;
    pe->opt.MinorSubsystemVersion = 0;
    pe->opt.SizeOfHeaders = PE_FILE_ALIGNMENT;
    pe->opt.Subsystem = PE_SUBSYSTEM_WINDOWS_CUI;
    pe->opt.DllCharacteristics = 0x8160;
    pe->opt.SizeOfStackReserve = 0x100000;
    pe->opt.SizeOfStackCommit = 0x1000;
    pe->opt.SizeOfHeapReserve = 0x100000;
    pe->opt.SizeOfHeapCommit = 0x1000;
    pe->opt.NumberOfRvaAndSizes = 16;

    memcpy(pe->text_section.Name, ".text\0\0\0", 8);
    pe->text_section.Characteristics =
        IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ;
}

void pe_free(PEFile *pe) {
    buf_free(&pe->buf);
    free(pe->text_data);
    pe->text_data = NULL;
    pe->text_len = 0;
}

void pe_set_entry(PEFile *pe, uint32_t rva) {
    pe->opt.AddressOfEntryPoint = rva;
}

void pe_text_write(PEFile *pe, const void *data, int len) {
    if (!pe->text_data) {
        pe->text_data = xmalloc(len > 4096 ? len : 4096);
    }
    memcpy(pe->text_data + pe->text_len, data, len);
    pe->text_len += len;
}

void pe_text_write_byte(PEFile *pe, uint8_t b) {
    pe_text_write(pe, &b, 1);
}

void pe_text_write_u32(PEFile *pe, uint32_t val) {
    pe_text_write(pe, &val, 4);
}

void pe_text_write_u64(PEFile *pe, uint64_t val) {
    pe_text_write(pe, &val, 8);
}

void pe_encode_rex(PEFile *pe, int w, int r, int x, int b) {
    uint8_t rex = 0x40;
    if (w) rex |= 0x08;
    if (r) rex |= 0x04;
    if (x) rex |= 0x02;
    if (b) rex |= 0x01;
    pe_text_write_byte(pe, rex);
}

void pe_encode_modrm(PEFile *pe, int mod, int reg, int rm) {
    pe_text_write_byte(pe, (uint8_t)((mod << 6) | ((reg & 7) << 3) | (rm & 7)));
}

void pe_encode_sib(PEFile *pe, int scale, int index, int base) {
    pe_text_write_byte(pe, (uint8_t)((scale << 6) | ((index & 7) << 3) | (base & 7)));
}

void pe_emit_push_rbp(PEFile *pe) {
    pe_text_write_byte(pe, 0x55);
}

void pe_emit_mov_rbp_rsp(PEFile *pe) {
    pe_encode_rex(pe, 1, 0, 0, 0);
    pe_text_write_byte(pe, 0x89);
    pe_encode_modrm(pe, 3, 4, 5);
}

void pe_emit_mov_rsp_rbp(PEFile *pe) {
    pe_encode_rex(pe, 1, 0, 0, 0);
    pe_text_write_byte(pe, 0x89);
    pe_encode_modrm(pe, 3, 5, 4);
}

void pe_emit_pop_rbp(PEFile *pe) {
    pe_text_write_byte(pe, 0x5D);
}

void pe_emit_ret(PEFile *pe) {
    pe_text_write_byte(pe, 0xC3);
}

void pe_emit_sub_rsp(PEFile *pe, int32_t val) {
    pe_encode_rex(pe, 1, 0, 0, 0);
    if (val >= -128 && val <= 127) {
        pe_text_write_byte(pe, 0x83);
        pe_encode_modrm(pe, 3, 5, 4);
        pe_text_write_byte(pe, (uint8_t)(int8_t)val);
    } else {
        pe_text_write_byte(pe, 0x81);
        pe_encode_modrm(pe, 3, 5, 4);
        pe_text_write_u32(pe, (uint32_t)val);
    }
}

void pe_emit_add_rsp(PEFile *pe, int32_t val) {
    pe_encode_rex(pe, 1, 0, 0, 0);
    if (val >= -128 && val <= 127) {
        pe_text_write_byte(pe, 0x83);
        pe_encode_modrm(pe, 3, 0, 4);
        pe_text_write_byte(pe, (uint8_t)(int8_t)val);
    } else {
        pe_text_write_byte(pe, 0x81);
        pe_encode_modrm(pe, 3, 0, 4);
        pe_text_write_u32(pe, (uint32_t)val);
    }
}

void pe_emit_mov_reg_imm32(PEFile *pe, int reg, int32_t val) {
    if (reg >= 8) {
        pe_encode_rex(pe, 1, 0, 0, 1);
    } else {
        pe_encode_rex(pe, 1, 0, 0, 0);
    }
    pe_text_write_byte(pe, (uint8_t)(0xB8 + (reg & 7)));
    pe_text_write_u32(pe, (uint32_t)val);
}

void pe_emit_mov_reg_reg(PEFile *pe, int dst, int src) {
    int r = (src >> 3) & 1;
    int b = (dst >> 3) & 1;
    pe_encode_rex(pe, 1, r, 0, b);
    pe_text_write_byte(pe, 0x89);
    pe_encode_modrm(pe, 3, src & 7, dst & 7);
}

void pe_emit_mov_rax_imm64(PEFile *pe, uint64_t val) {
    pe_encode_rex(pe, 1, 0, 0, 0);
    pe_text_write_byte(pe, 0xB8);
    pe_text_write_u64(pe, val);
}

void pe_emit_add_reg_imm32(PEFile *pe, int reg, int32_t val) {
    int b = (reg >> 3) & 1;
    pe_encode_rex(pe, 1, 0, 0, b);
    if (val >= -128 && val <= 127) {
        pe_text_write_byte(pe, 0x83);
        pe_encode_modrm(pe, 3, 0, reg & 7);
        pe_text_write_byte(pe, (uint8_t)(int8_t)val);
    } else {
        pe_text_write_byte(pe, 0x81);
        pe_encode_modrm(pe, 3, 0, reg & 7);
        pe_text_write_u32(pe, (uint32_t)val);
    }
}

void pe_emit_sub_reg_imm32(PEFile *pe, int reg, int32_t val) {
    int b = (reg >> 3) & 1;
    pe_encode_rex(pe, 1, 0, 0, b);
    if (val >= -128 && val <= 127) {
        pe_text_write_byte(pe, 0x83);
        pe_encode_modrm(pe, 3, 5, reg & 7);
        pe_text_write_byte(pe, (uint8_t)(int8_t)val);
    } else {
        pe_text_write_byte(pe, 0x81);
        pe_encode_modrm(pe, 3, 5, reg & 7);
        pe_text_write_u32(pe, (uint32_t)val);
    }
}

void pe_emit_xor_reg_reg(PEFile *pe, int dst, int src) {
    int r = (src >> 3) & 1;
    int b = (dst >> 3) & 1;
    pe_encode_rex(pe, 1, r, 0, b);
    pe_text_write_byte(pe, 0x31);
    pe_encode_modrm(pe, 3, src & 7, dst & 7);
}

void pe_emit_call_rel32(PEFile *pe, int32_t rel) {
    pe_text_write_byte(pe, 0xE8);
    pe_text_write_u32(pe, (uint32_t)rel);
}

void pe_emit_jmp_rel32(PEFile *pe, int32_t rel) {
    pe_text_write_byte(pe, 0xE9);
    pe_text_write_u32(pe, (uint32_t)rel);
}

void pe_emit_nop(PEFile *pe) {
    pe_text_write_byte(pe, 0x90);
}

void pe_emit_prologue(PEFile *pe, int stack_size) {
    pe_emit_push_rbp(pe);
    pe_emit_mov_rbp_rsp(pe);
    if (stack_size > 0)
        pe_emit_sub_rsp(pe, stack_size);
}

void pe_emit_epilogue(PEFile *pe) {
    pe_emit_mov_rsp_rbp(pe);
    pe_emit_pop_rbp(pe);
    pe_emit_ret(pe);
}

int pe_build(PEFile *pe) {
    PEBuffer *b = &pe->buf;
    b->len = 0;

    int header_size = (int)sizeof(PEDosHeader) + 4 +
                      (int)sizeof(PECoffHeader) +
                      (int)sizeof(PEOptionalHeader) +
                      (int)sizeof(PESectionHeader);

    int headers_size = align_up(header_size, PE_FILE_ALIGNMENT);
    int text_raw_offset = headers_size;
    int text_raw_size = align_up(pe->text_len, PE_FILE_ALIGNMENT);
    int text_rva = PE_SECTION_ALIGNMENT;

    pe->text_section.VirtualSize = pe->text_len;
    pe->text_section.VirtualAddress = text_rva;
    pe->text_section.SizeOfRawData = text_raw_size;
    pe->text_section.PointerToRawData = text_raw_offset;

    pe->opt.SizeOfCode = text_raw_size;
    pe->opt.BaseOfCode = text_rva;
    pe->opt.AddressOfEntryPoint = text_rva;
    pe->opt.SizeOfImage = align_up(text_rva + text_raw_size, PE_SECTION_ALIGNMENT);

    buf_write(b, &pe->dos, sizeof(pe->dos));
    buf_write_u32(b, PE_SIGNATURE);
    buf_write(b, &pe->coff, sizeof(pe->coff));
    buf_write(b, &pe->opt, sizeof(pe->opt));
    buf_write(b, &pe->text_section, sizeof(pe->text_section));

    int written = header_size;
    if (written < headers_size)
        buf_pad(b, headers_size - written);

    if (pe->text_len > 0)
        buf_write(b, pe->text_data, pe->text_len);

    int tail = text_raw_size - pe->text_len;
    if (tail > 0)
        buf_pad(b, tail);

    return 0;
}

int pe_write_to_file(PEFile *pe, const char *path) {
    if (pe->buf.len == 0) {
        if (pe_build(pe) < 0) return -1;
    }

    FILE *f = fopen(path, "wb");
    if (!f) {
        fprintf(stderr, "Cannot open output file %s\n", path);
        return -1;
    }
    fwrite(pe->buf.data, 1, pe->buf.len, f);
    fclose(f);
    return 0;
}
