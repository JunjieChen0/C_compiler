#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../src/pe.h"

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) do { tests_run++; printf("  %-40s ", name); } while(0)
#define PASS() do { tests_passed++; printf("PASS\n"); } while(0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); } while(0)

static void test_init(void) {
    TEST("pe_init");
    PEFile pe;
    pe_init(&pe);
    if (pe.dos.e_magic == DOS_MAGIC &&
        pe.coff.Machine == COFF_MACHINE_AMD64 &&
        pe.opt.Magic == PE_OPT_MAGIC_PE32PLUS &&
        pe.coff.NumberOfSections == 1) {
        PASS();
    } else {
        FAIL("bad initial values");
    }
    pe_free(&pe);
}

static void test_text_write(void) {
    TEST("pe_text_write");
    PEFile pe;
    pe_init(&pe);
    uint8_t data[] = {0x90, 0xCC, 0xC3};
    pe_text_write(&pe, data, 3);
    if (pe.text_len == 3 &&
        pe.text_data[0] == 0x90 &&
        pe.text_data[1] == 0xCC &&
        pe.text_data[2] == 0xC3) {
        PASS();
    } else {
        FAIL("text data mismatch");
    }
    pe_free(&pe);
}

static void test_build_dos_header(void) {
    TEST("PE has DOS header");
    PEFile pe;
    pe_init(&pe);
    pe_text_write_byte(&pe, 0xC3);
    pe_build(&pe);

    if (pe.buf.len > 2) {
        uint16_t magic = *(uint16_t *)pe.buf.data;
        if (magic == DOS_MAGIC) {
            PASS();
        } else {
            FAIL("bad DOS magic");
        }
    } else {
        FAIL("buffer too small");
    }
    pe_free(&pe);
}

static void test_build_pe_signature(void) {
    TEST("PE has PE signature");
    PEFile pe;
    pe_init(&pe);
    pe_text_write_byte(&pe, 0xC3);
    pe_build(&pe);

    uint32_t sig = *(uint32_t *)(pe.buf.data + pe.dos.e_lfanew);
    if (sig == PE_SIGNATURE) {
        PASS();
    } else {
        FAIL("bad PE signature");
    }
    pe_free(&pe);
}

static void test_build_coff_header(void) {
    TEST("PE has COFF header (AMD64)");
    PEFile pe;
    pe_init(&pe);
    pe_text_write_byte(&pe, 0xC3);
    pe_build(&pe);

    int offset = pe.dos.e_lfanew + 4;
    PECoffHeader *coff = (PECoffHeader *)(pe.buf.data + offset);
    if (coff->Machine == COFF_MACHINE_AMD64 &&
        coff->NumberOfSections == 1) {
        PASS();
    } else {
        FAIL("bad COFF header");
    }
    pe_free(&pe);
}

static void test_build_optional_header(void) {
    TEST("PE has optional header (PE32+)");
    PEFile pe;
    pe_init(&pe);
    pe_text_write_byte(&pe, 0xC3);
    pe_build(&pe);

    int offset = pe.dos.e_lfanew + 4 + (int)sizeof(PECoffHeader);
    PEOptionalHeader *opt = (PEOptionalHeader *)(pe.buf.data + offset);
    if (opt->Magic == PE_OPT_MAGIC_PE32PLUS &&
        opt->ImageBase == PE_IMAGE_BASE &&
        opt->SectionAlignment == PE_SECTION_ALIGNMENT &&
        opt->FileAlignment == PE_FILE_ALIGNMENT) {
        PASS();
    } else {
        FAIL("bad optional header");
    }
    pe_free(&pe);
}

static void test_build_text_section(void) {
    TEST("PE has .text section");
    PEFile pe;
    pe_init(&pe);
    pe_text_write_byte(&pe, 0xC3);
    pe_build(&pe);

    int offset = pe.dos.e_lfanew + 4 + (int)sizeof(PECoffHeader) +
                 (int)sizeof(PEOptionalHeader);
    PESectionHeader *sec = (PESectionHeader *)(pe.buf.data + offset);
    if (memcmp(sec->Name, ".text", 5) == 0 &&
        (sec->Characteristics & IMAGE_SCN_CNT_CODE) != 0 &&
        (sec->Characteristics & IMAGE_SCN_MEM_EXECUTE) != 0) {
        PASS();
    } else {
        FAIL("bad .text section");
    }
    pe_free(&pe);
}

static void test_emit_prologue_epilogue(void) {
    TEST("pe_emit_prologue/epilogue");
    PEFile pe;
    pe_init(&pe);
    pe_emit_prologue(&pe, 32);
    pe_emit_epilogue(&pe);

    if (pe.text_len > 0 && pe.text_data[0] == 0x55) {
        PASS();
    } else {
        FAIL("prologue mismatch");
    }
    pe_free(&pe);
}

static void test_emit_ret(void) {
    TEST("pe_emit_ret");
    PEFile pe;
    pe_init(&pe);
    pe_emit_ret(&pe);

    if (pe.text_len == 1 && pe.text_data[0] == 0xC3) {
        PASS();
    } else {
        FAIL("ret mismatch");
    }
    pe_free(&pe);
}

static void test_emit_mov_rax_imm64(void) {
    TEST("pe_emit_mov_rax_imm64");
    PEFile pe;
    pe_init(&pe);
    pe_emit_mov_rax_imm64(&pe, 0x1234567890ABCDEFULL);

    if (pe.text_len == 10 &&
        pe.text_data[0] == 0x48 &&
        pe.text_data[1] == 0xB8) {
        PASS();
    } else {
        FAIL("mov rax, imm64 encoding");
    }
    pe_free(&pe);
}

static void test_emit_nop(void) {
    TEST("pe_emit_nop");
    PEFile pe;
    pe_init(&pe);
    pe_emit_nop(&pe);

    if (pe.text_len == 1 && pe.text_data[0] == 0x90) {
        PASS();
    } else {
        FAIL("nop mismatch");
    }
    pe_free(&pe);
}

static void test_write_file(void) {
    TEST("pe_write_to_file");
    PEFile pe;
    pe_init(&pe);
    pe_text_write_byte(&pe, 0xC3);
    pe_build(&pe);

    const char *path = "test_output.exe";
    int rc = pe_write_to_file(&pe, path);
    if (rc == 0) {
        FILE *f = fopen(path, "rb");
        if (f) {
            fseek(f, 0, SEEK_END);
            long sz = ftell(f);
            fclose(f);
            if (sz == pe.buf.len) {
                PASS();
            } else {
                FAIL("file size mismatch");
            }
            remove(path);
        } else {
            FAIL("cannot reopen file");
        }
    } else {
        FAIL("write failed");
    }
    pe_free(&pe);
}

static void test_entry_point(void) {
    TEST("entry point set correctly");
    PEFile pe;
    pe_init(&pe);
    pe_text_write_byte(&pe, 0xC3);
    pe_build(&pe);

    if (pe.opt.AddressOfEntryPoint == PE_SECTION_ALIGNMENT) {
        PASS();
    } else {
        FAIL("entry point wrong");
    }
    pe_free(&pe);
}

int main(void) {
    printf("Running PE tests:\n");
    test_init();
    test_text_write();
    test_build_dos_header();
    test_build_pe_signature();
    test_build_coff_header();
    test_build_optional_header();
    test_build_text_section();
    test_emit_prologue_epilogue();
    test_emit_ret();
    test_emit_mov_rax_imm64();
    test_emit_nop();
    test_write_file();
    test_entry_point();
    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    return tests_passed == tests_run ? 0 : 1;
}
