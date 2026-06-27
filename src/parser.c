#include "parser.h"

static void parse_expr(Parser *p);
static void parse_assign(Parser *p);
static void parse_unary(Parser *p);
static void parse_postfix(Parser *p);
static void parse_primary(Parser *p);
static void parse_statement(Parser *p);
static void parse_compound_stmt(Parser *p);
static Type *parse_type_spec(Parser *p);
static int is_type_start(Parser *p);

static Token next(Parser *p) { return lexer_next(&p->lexer); }
static Token peek(Parser *p) { return lexer_peek(&p->lexer); }

static Token expect(Parser *p, TokenKind kind);

static int parse_constexpr_atom(Parser *p) {
    Token t = peek(p);
    if (t.kind == TK_INT) { next(p); return (int)t.ival; }
    if (t.kind == TK_LPAREN) {
        next(p);
        int val = parse_constexpr_atom(p);
        while (peek(p).kind == TK_STAR || peek(p).kind == TK_SLASH || peek(p).kind == TK_PERCENT ||
               peek(p).kind == TK_PLUS || peek(p).kind == TK_MINUS) {
            Token op = next(p);
            int rhs = parse_constexpr_atom(p);
            if (op.kind == TK_STAR) val *= rhs;
            else if (op.kind == TK_SLASH) val /= rhs;
            else if (op.kind == TK_PERCENT) val %= rhs;
            else if (op.kind == TK_PLUS) val += rhs;
            else if (op.kind == TK_MINUS) val -= rhs;
        }
        expect(p, TK_RPAREN);
        return val;
    }
    if (t.kind == TK_SIZEOF) {
        next(p);
        if (peek(p).kind == TK_LPAREN) {
            Lexer saved = p->lexer;
            next(p);
            if (is_type_start(p)) {
                Type *ty = parse_type_spec(p);
                while (peek(p).kind == TK_STAR) { next(p); ty = pointer_to(ty); }
                expect(p, TK_RPAREN);
                return ty->size;
            }
            p->lexer = saved;
        }
        return 8;
    }
    next(p);
    return 0;
}

static int parse_constexpr(Parser *p) {
    int val = parse_constexpr_atom(p);
    while (peek(p).kind == TK_STAR || peek(p).kind == TK_SLASH || peek(p).kind == TK_PERCENT ||
           peek(p).kind == TK_PLUS || peek(p).kind == TK_MINUS) {
        Token op = next(p);
        int rhs = parse_constexpr_atom(p);
        if (op.kind == TK_STAR) val *= rhs;
        else if (op.kind == TK_SLASH) val /= rhs;
        else if (op.kind == TK_PERCENT) val %= rhs;
        else if (op.kind == TK_PLUS) val += rhs;
        else if (op.kind == TK_MINUS) val -= rhs;
    }
    return val;
}

static Token peek2(Parser *p) {
    Lexer saved = p->lexer;
    next(p); // skip first token
    Token t = peek(p); // peek at second token
    p->lexer = saved; // restore
    return t;
}

static Token expect(Parser *p, TokenKind kind) {
    Token t = next(p);
    if (t.kind != kind)
        error_at(t.start, "expected %s, got %s",
                 token_kind_str(kind), token_kind_str(t.kind));
    return t;
}

static char *tok_str(Token *t) {
    char *s = xmalloc(t->len + 1);
    memcpy(s, t->start, t->len);
    s[t->len] = '\0';
    return s;
}

static int next_label(Parser *p) { return p->label_count++; }

static void register_global(Parser *p, const char *name, int size) {
    for (int i = 0; i < p->global_count; i++) {
        if (strcmp(p->globals[i].name, name) == 0) return;
    }
    if (p->global_count < MAX_GLOBALS) {
        p->globals[p->global_count].name = xstrdup(name);
        p->globals[p->global_count].size = size;
        p->global_count++;
    }
}

static void emit_label_id(Parser *p, int id) {
    char buf[32];
    snprintf(buf, sizeof(buf), ".L%d", id);
    emit_label(p->gen, buf);
}

static void emit_jmp_id(Parser *p, int id) {
    char buf[32];
    snprintf(buf, sizeof(buf), ".L%d", id);
    emit_jmp(p->gen, buf);
}

static void emit_je_id(Parser *p, int id) {
    char buf[32];
    snprintf(buf, sizeof(buf), ".L%d", id);
    emit_je(p->gen, buf);
}

static void emit_jne_id(Parser *p, int id) {
    char buf[32];
    snprintf(buf, sizeof(buf), ".L%d", id);
    emit_jne(p->gen, buf);
}

static OpSize type_opsize(Type *ty) {
    if (!ty) return SZ_QWORD;
    switch (ty->size) {
        case 1: return SZ_BYTE;
        case 2: return SZ_WORD;
        case 4: return SZ_DWORD;
        case 8: return SZ_QWORD;
        default: return SZ_QWORD;
    }
}

static void load_var(Parser *p, Symbol *s) {
    if (s->kind == SYM_ENUM_CONST) {
        emit_mov(p->gen, op_reg(REG_RAX, SZ_QWORD), op_imm(s->enum_val));
        return;
    }
    // For function symbols, don't load - function calls use emit_call directly
    if (s->kind == SYM_FUNC) {
        p->has_lvalue = 0;
        p->expr_type = s->type;
        return;
    }
    // For struct/union types, load the address instead of the value
    if (s->type && (s->type->kind == TY_STRUCT || s->type->kind == TY_UNION || s->type->kind == TY_ARRAY)) {
        if (s->offset > 0) {
            emit_lea(p->gen, op_reg(REG_RAX, SZ_QWORD), op_mem(REG_RBP, -(s->offset), SZ_QWORD));
        } else {
            char buf[256];
            snprintf(buf, sizeof(buf), "lea rax, [%s]", s->name);
            emit_raw(p->gen, buf);
        }
        p->has_lvalue = 1;
        p->lvalue_is_stack = (s->offset > 0);
        p->lvalue_base = REG_RBP;
        p->lvalue_offset = s->offset;
        p->lvalue_type = s->type;
        /* Array decays to pointer to element type */
        if (s->type->kind == TY_ARRAY && s->type->base) {
            p->expr_type = pointer_to(s->type->base);
        } else {
            p->expr_type = s->type;
        }
        return;
    }
    OpSize sz = type_opsize(s->type);
    if (s->offset > 0) {
        Operand src = op_mem(REG_RBP, -(s->offset), sz);
        if (sz == SZ_BYTE || sz == SZ_WORD) {
            if (s->type->is_unsigned)
                emit_movzx(p->gen, op_reg(REG_RAX, SZ_QWORD), src);
            else
                emit_movsx(p->gen, op_reg(REG_RAX, SZ_QWORD), src);
        } else if (sz == SZ_DWORD) {
            emit_mov(p->gen, op_reg(REG_RAX, SZ_DWORD), src);
        } else {
            emit_mov(p->gen, op_reg(REG_RAX, SZ_QWORD), src);
        }
    } else {
        char buf[256];
        snprintf(buf, sizeof(buf), "mov rax, qword [%s]", s->name);
        emit_raw(p->gen, buf);
    }
    p->has_lvalue = (s->kind == SYM_VAR);
    p->lvalue_is_stack = (s->offset > 0);
    p->lvalue_is_global = (s->offset <= 0 && s->kind == SYM_VAR);
    p->lvalue_base = REG_RBP;
    p->lvalue_offset = s->offset;
    p->lvalue_type = s->type;
    p->lvalue_addr_slot = 0;
    if (p->lvalue_is_global) {
        strncpy(p->lvalue_name, s->name, sizeof(p->lvalue_name) - 1);
        p->lvalue_name[sizeof(p->lvalue_name) - 1] = '\0';
    }
}

static void store_to_lvalue(Parser *p) {
    if (!p->has_lvalue) return;
    OpSize sz = type_opsize(p->lvalue_type);
    Operand src;
    if (sz == SZ_BYTE) src = op_reg(REG_RAX, SZ_BYTE);
    else if (sz == SZ_WORD) src = op_reg(REG_RAX, SZ_WORD);
    else if (sz == SZ_DWORD) src = op_reg(REG_RAX, SZ_DWORD);
    else src = op_reg(REG_RAX, SZ_QWORD);
    if (p->lvalue_is_stack) {
        emit_mov(p->gen, op_mem(REG_RBP, -(p->lvalue_offset), sz), src);
    } else if (p->lvalue_is_global) {
        char buf[256];
        snprintf(buf, sizeof(buf), "mov qword [%s], rax", p->lvalue_name);
        emit_raw(p->gen, buf);
    } else if (p->lvalue_addr_slot > 0) {
        /* Pointer-based lvalue: load address from saved stack slot */
        emit_mov(p->gen, op_reg(REG_R10, SZ_QWORD), op_mem(REG_RBP, -(p->lvalue_addr_slot), SZ_QWORD));
        emit_mov(p->gen, op_mem(REG_R10, 0, sz), src);
    } else {
        emit_mov(p->gen, op_mem(p->lvalue_base, p->lvalue_offset, sz), src);
    }
}

static void emit_cqo(Parser *p) {
    Instr i;
    memset(&i, 0, sizeof(i));
    i.kind = I_CQO;
    gen_emit(p->gen, i);
}

static int is_type_start(Parser *p) {
    Token t = peek(p);
    switch (t.kind) {
        case TK_VOID: case TK_CHAR_KW: case TK_SHORT: case TK_INT_KW:
        case TK_LONG: case TK_LONG_KW: case TK_FLOAT_KW: case TK_DOUBLE: case TK_BOOL:
        case TK_SIGNED: case TK_UNSIGNED:
        case TK_STRUCT: case TK_UNION: case TK_ENUM:
        case TK_EXTERN: case TK_STATIC: case TK_CONST: case TK_VOLATILE:
        case TK_REGISTER: case TK_INLINE:
            return 1;
        case TK_IDENT: {
            char *name = tok_str(&t);
            Symbol *s = sym_find(name);
            free(name);
            return s && s->kind == SYM_TYPEDEF;
        }
        default:
            return 0;
    }
}

static void skip_type_qualifiers(Parser *p) {
    for (;;) {
        Token t = peek(p);
        if (t.kind == TK_CONST || t.kind == TK_VOLATILE ||
            t.kind == TK_EXTERN || t.kind == TK_STATIC ||
            t.kind == TK_REGISTER || t.kind == TK_INLINE)
            next(p);
        else
            break;
    }
}

static Type *parse_type_spec(Parser *p) {
    skip_type_qualifiers(p);
    int is_unsigned = 0;
    if (peek(p).kind == TK_UNSIGNED) { is_unsigned = 1; next(p); }
    else if (peek(p).kind == TK_SIGNED) { next(p); }
    Token t = peek(p);
    if (t.kind == TK_VOID) { next(p); return ty_void(); }
    if (t.kind == TK_CHAR_KW) { next(p); return is_unsigned ? ty_uchar() : ty_char(); }
    if (t.kind == TK_SHORT) {
        next(p);
        if (peek(p).kind == TK_INT_KW) next(p);
        return is_unsigned ? ty_ushort() : ty_short();
    }
    if (t.kind == TK_LONG_KW) {
        next(p);
        if (peek(p).kind == TK_LONG_KW) {
            next(p);
            if (peek(p).kind == TK_INT_KW) next(p);
            return is_unsigned ? ty_ullong() : ty_llong();
        }
        if (peek(p).kind == TK_INT_KW) next(p);
        return is_unsigned ? ty_ulong() : ty_long();
    }
    if (t.kind == TK_INT_KW) { next(p); return is_unsigned ? ty_uint() : ty_int(); }
    if (t.kind == TK_DOUBLE) { next(p); return ty_double(); }
    if (t.kind == TK_FLOAT_KW) { next(p); return ty_float(); }
    if (t.kind == TK_BOOL) { next(p); return ty_bool(); }
    if (t.kind == TK_STRUCT || t.kind == TK_UNION) {
        next(p);
        char *s = NULL;
        Type *ty = NULL;
        
        // Check if there's a struct name
        if (peek(p).kind == TK_IDENT) {
            Token name = next(p);
            s = tok_str(&name);
            // Look up existing struct tag
            Symbol *tag = sym_find_kind(s, SYM_TAG);
            if (tag) {
                ty = tag->type;
            } else {
                ty = (t.kind == TK_STRUCT) ? ty_struct(s) : ty_union(s);
                sym_declare(xstrdup(s), SYM_TAG, ty);
            }
        } else {
            // Anonymous struct
            ty = (t.kind == TK_STRUCT) ? ty_struct(NULL) : ty_union(NULL);
        }
        
        if (peek(p).kind == TK_LBRACE) {
            next(p);
            int offset = 0;
            int max_align = 1;
            while (peek(p).kind != TK_RBRACE) {
                Type *field_type = NULL;
                
                // Check for struct/union pointer fields like "struct Macro *next"
                if (peek(p).kind == TK_STRUCT || peek(p).kind == TK_UNION) {
                    Lexer saved = p->lexer;
                    Token struct_tok = next(p);
                    if (peek(p).kind == TK_IDENT && peek2(p).kind == TK_STAR) {
                        // This is "struct Name *field" pattern
                        Token name = next(p);
                        char *s = tok_str(&name);
                        // Look up or create the struct type
                        Type *struct_ty = NULL;
                        Symbol *tag = sym_find(s);
                        if (tag && tag->kind == SYM_TAG) {
                            struct_ty = tag->type;
                        } else {
                            struct_ty = (struct_tok.kind == TK_STRUCT) ? ty_struct(s) : ty_union(s);
                            sym_declare(xstrdup(s), SYM_TAG, struct_ty);
                        }
                        free(s);
                        // Skip the * and create pointer type
                        next(p);
                        field_type = pointer_to(struct_ty);
                        Token field_name = expect(p, TK_IDENT);
                        expect(p, TK_SEMICOLON);
                        if (field_type->align > max_align) max_align = field_type->align;
                        offset = (offset + field_type->align - 1) & ~(field_type->align - 1);
                        add_field(ty, tok_str(&field_name), field_type, offset);
                        offset += field_type->size;
                        continue;
                    }
                    // Not the pattern we're looking for, restore and parse normally
                    p->lexer = saved;
                }
                
                field_type = parse_type_spec(p);
                // Handle pointer types before field name
                while (peek(p).kind == TK_STAR) { next(p); field_type = pointer_to(field_type); }
                Token field_name = expect(p, TK_IDENT);
                // Handle pointer types after field name (for declarations like int *p)
                while (peek(p).kind == TK_STAR) { next(p); field_type = pointer_to(field_type); }
                if (peek(p).kind == TK_LBRACKET) {
                    next(p);
                    Token size = expect(p, TK_INT);
                    expect(p, TK_RBRACKET);
                    field_type = ty_array(field_type, (int)size.ival);
                }
                expect(p, TK_SEMICOLON);
                if (field_type->align > max_align) max_align = field_type->align;
                offset = (offset + field_type->align - 1) & ~(field_type->align - 1);
                add_field(ty, tok_str(&field_name), field_type, offset);
                offset += field_type->size;
            }
            expect(p, TK_RBRACE);
            ty->size = (offset + max_align - 1) & ~(max_align - 1);
            ty->align = max_align;
        }
        return ty;
    }
    if (t.kind == TK_ENUM) {
        next(p);
        if (peek(p).kind == TK_IDENT) next(p);
        if (peek(p).kind == TK_LBRACE) {
            next(p);
            int val = 0;
            while (peek(p).kind != TK_RBRACE) {
                Token name = expect(p, TK_IDENT);
                if (peek(p).kind == TK_ASSIGN) {
                    next(p);
                    // Handle negative values
                    int neg = 0;
                    if (peek(p).kind == TK_MINUS) {
                        neg = 1;
                        next(p);
                    } else if (peek(p).kind == TK_PLUS) {
                        next(p);
                    }
                    Token t2 = expect(p, TK_INT);
                    val = (int)t2.ival;
                    if (neg) val = -val;
                }
                sym_declare_enum(tok_str(&name), val);
                val++;
                if (peek(p).kind == TK_COMMA) next(p);
            }
            expect(p, TK_RBRACE);
        }
        return ty_int();
    }
    if (is_unsigned) return ty_uint();
    if (t.kind == TK_IDENT) {
        char *name = tok_str(&t);
        Symbol *s = sym_find(name);
        free(name);
        if (s && s->kind == SYM_TYPEDEF) {
            next(p);
            /* If typedef points to a struct/union, look up the tag for the current definition */
            Type *ty = s->type;
            if (ty && (ty->kind == TY_STRUCT || ty->kind == TY_UNION) && ty->name) {
                Symbol *tag = sym_find_kind(ty->name, SYM_TAG);
                if (tag) {
                    ty = tag->type;
                }
            }
            return ty;
        }
        if (s && s->kind == SYM_TAG) {
            next(p);
            return s->type;
        }
    }
    error_at(t.start, "expected type specifier");
    return ty_int();
}

static void parse_primary(Parser *p) {
    Token t = next(p);
    switch (t.kind) {
    case TK_INT: case TK_LONG: case TK_UINT:
        emit_mov(p->gen, op_reg(REG_RAX, SZ_QWORD), op_imm(t.ival));
        p->expr_type = ty_int();
        p->has_lvalue = 0;
        break;
    case TK_CHAR:
        emit_mov(p->gen, op_reg(REG_RAX, SZ_QWORD), op_imm(t.ival));
        p->expr_type = ty_int();
        p->has_lvalue = 0;
        break;
    case TK_STRING: {
        // Handle string literal concatenation
        char *str_data = t.sval;
        int str_len = t.slen;
        while (peek(p).kind == TK_STRING) {
            Token next_str = next(p);
            // Concatenate strings
            char *new_data = xmalloc(str_len + next_str.slen);
            memcpy(new_data, str_data, str_len);
            memcpy(new_data + str_len, next_str.sval, next_str.slen);
            if (str_data != t.sval) free(str_data);
            str_data = new_data;
            str_len += next_str.slen;
        }
        int id = p->string_count++;
        p->strings[id].label = xmalloc(32);
        snprintf(p->strings[id].label, 32, "LC%d", id);
        p->strings[id].data = str_data;
        p->strings[id].len = str_len;
        emit_mov(p->gen, op_reg(REG_RAX, SZ_QWORD), op_label(p->strings[id].label));
        p->expr_type = pointer_to(ty_char());
        p->has_lvalue = 0;
        break;
    }
    case TK_IDENT: {
        char *name = tok_str(&t);
        snprintf(p->last_ident, sizeof(p->last_ident), "%s", name);
        Symbol *s = sym_find(name);
        if (!s) error_at(t.start, "undefined variable '%s'", name);
        free(name);
        load_var(p, s);
        p->expr_type = s->type;
        break;
    }
    case TK_LPAREN:
        if (is_type_start(p)) {
            Type *ty = parse_type_spec(p);
            while (peek(p).kind == TK_STAR) { next(p); ty = pointer_to(ty); }
            expect(p, TK_RPAREN);
            // Check for compound literal: (Type){...}
            if (peek(p).kind == TK_LBRACE) {
                // Parse compound literal
                next(p); // consume {
                // Allocate space on stack for the compound literal
                int align = ty->align;
                if (align < 1) align = 1;
                p->local_offset = ((p->local_offset + align - 1) / align) * align + ty->size;
                int offset = p->local_offset;
                // Parse initializer list
                int field_idx = 0;
                int elem_offset = 0;
                while (peek(p).kind != TK_RBRACE) {
                    if (peek(p).kind == TK_DOT) {
                        // Named field initializer: .field = value
                        next(p);
                        Token field_name = expect(p, TK_IDENT);
                        expect(p, TK_ASSIGN);
                        parse_assign(p);
                        char *fname = tok_str(&field_name);
                        Member *f = find_field(ty, fname);
                        free(fname);
                        if (f) {
                            OpSize sz = type_opsize(f->type);
                            Operand src;
                            if (sz == SZ_BYTE) src = op_reg(REG_RAX, SZ_BYTE);
                            else if (sz == SZ_WORD) src = op_reg(REG_RAX, SZ_WORD);
                            else if (sz == SZ_DWORD) src = op_reg(REG_RAX, SZ_DWORD);
                            else src = op_reg(REG_RAX, SZ_QWORD);
                            emit_mov(p->gen, op_mem(REG_RBP, -(offset - f->offset), sz), src);
                        }
                    } else {
                        // Positional initializer
                        parse_assign(p);
                        if (ty->kind == TY_STRUCT || ty->kind == TY_UNION) {
                            if (field_idx < ty->member_count) {
                                Member *f = &ty->members[field_idx];
                                OpSize sz = type_opsize(f->type);
                                Operand src;
                                if (sz == SZ_BYTE) src = op_reg(REG_RAX, SZ_BYTE);
                                else if (sz == SZ_WORD) src = op_reg(REG_RAX, SZ_WORD);
                                else if (sz == SZ_DWORD) src = op_reg(REG_RAX, SZ_DWORD);
                                else src = op_reg(REG_RAX, SZ_QWORD);
                                emit_mov(p->gen, op_mem(REG_RBP, -(offset - f->offset), sz), src);
                            }
                        } else if (ty->kind == TY_ARRAY) {
                            OpSize sz = type_opsize(ty->base);
                            Operand src;
                            if (sz == SZ_BYTE) src = op_reg(REG_RAX, SZ_BYTE);
                            else if (sz == SZ_WORD) src = op_reg(REG_RAX, SZ_WORD);
                            else if (sz == SZ_DWORD) src = op_reg(REG_RAX, SZ_DWORD);
                            else src = op_reg(REG_RAX, SZ_QWORD);
                            emit_mov(p->gen, op_mem(REG_RBP, -(offset - elem_offset), sz), src);
                            elem_offset += ty->base->size;
                        } else {
                            OpSize sz = type_opsize(ty);
                            Operand src;
                            if (sz == SZ_BYTE) src = op_reg(REG_RAX, SZ_BYTE);
                            else if (sz == SZ_WORD) src = op_reg(REG_RAX, SZ_WORD);
                            else if (sz == SZ_DWORD) src = op_reg(REG_RAX, SZ_DWORD);
                            else src = op_reg(REG_RAX, SZ_QWORD);
                            emit_mov(p->gen, op_mem(REG_RBP, -offset, sz), src);
                        }
                        field_idx++;
                    }
                    if (peek(p).kind == TK_COMMA) next(p);
                }
                expect(p, TK_RBRACE);
                // Load address of the compound literal
                emit_lea(p->gen, op_reg(REG_RAX, SZ_QWORD), op_mem(REG_RBP, -offset, SZ_QWORD));
                p->expr_type = ty;
                p->has_lvalue = 1;
                p->lvalue_is_stack = 1;
                p->lvalue_offset = offset;
                p->lvalue_type = ty;
            } else {
                // Regular cast expression
                parse_unary(p);
                p->expr_type = ty;
                p->has_lvalue = 0;
            }
        } else {
            parse_expr(p);
            expect(p, TK_RPAREN);
        }
        break;
    default:
        error_at(t.start, "expected expression");
    }
}

static void parse_postfix(Parser *p) {
    parse_primary(p);
    for (;;) {
        Token t = peek(p);
        if (t.kind == TK_LPAREN) {
            // Save function name before parsing arguments
            char func_name[256];
            snprintf(func_name, sizeof(func_name), "%s", p->last_ident);
            next(p);
            int nargs = 0;
            if (peek(p).kind != TK_RPAREN) {
                do {
                    parse_assign(p);
                    emit_push(p->gen, op_reg(REG_RAX, SZ_QWORD));
                    nargs++;
                } while (peek(p).kind == TK_COMMA && (next(p), 1));
            }
            expect(p, TK_RPAREN);
            static const Register arg_regs2[] = {REG_RCX, REG_RDX, REG_R8, REG_R9};
            for (int i = (nargs < 4 ? nargs : 4) - 1; i >= 0; i--)
                emit_pop(p->gen, op_reg(arg_regs2[i], SZ_QWORD));
            /* Allocate 32-byte shadow space AFTER popping args into registers */
            emit_sub(p->gen, op_reg(REG_RSP, SZ_QWORD), op_imm(32));
            emit_call(p->gen, func_name);
            /* Clean up: shadow space (32) + remaining stack args */
            int cleanup = 32 + (nargs > 4 ? (nargs - 4) * 8 : 0);
            emit_add(p->gen, op_reg(REG_RSP, SZ_QWORD), op_imm(cleanup));
            p->has_lvalue = 0;
            p->expr_type = ty_int();
        } else if (t.kind == TK_LBRACKET) {
            next(p);
            Type *arr_type = p->expr_type;  /* save array/pointer type before parsing index */
            emit_push(p->gen, op_reg(REG_RAX, SZ_QWORD));
            parse_assign(p);
            expect(p, TK_RBRACKET);
            emit_pop(p->gen, op_reg(REG_R10, SZ_QWORD));
            Type *base = arr_type->base ? arr_type->base : ty_int();
            int scale = base->size > 0 ? base->size : 1;  /* void* pointer arithmetic */
            emit_imul(p->gen, op_reg(REG_RAX, SZ_QWORD), op_imm(scale));
            emit_add(p->gen, op_reg(REG_RAX, SZ_QWORD), op_reg(REG_R10, SZ_QWORD));
            /* Save address to stack slot before loading value */
            p->local_offset = ((p->local_offset + 7) / 8) * 8 + 8;
            int addr_slot = p->local_offset;
            emit_mov(p->gen, op_mem(REG_RBP, -addr_slot, SZ_QWORD), op_reg(REG_RAX, SZ_QWORD));
            // Load value from computed address
            OpSize sz = type_opsize(base);
            if (sz == SZ_BYTE || sz == SZ_WORD) {
                if (base->is_unsigned)
                    emit_movzx(p->gen, op_reg(REG_RAX, SZ_QWORD), op_mem(REG_RAX, 0, sz));
                else
                    emit_movsx(p->gen, op_reg(REG_RAX, SZ_QWORD), op_mem(REG_RAX, 0, sz));
            } else {
                emit_mov(p->gen, op_reg(REG_RAX, sz), op_mem(REG_RAX, 0, sz));
            }
            p->has_lvalue = 1;
            p->lvalue_is_stack = 0;
            p->lvalue_base = REG_RAX;
            p->lvalue_offset = 0;
            p->lvalue_type = base;
            p->lvalue_addr_slot = addr_slot;
            p->expr_type = base;
        } else if (t.kind == TK_DOT) {
            next(p);
            Token field = expect(p, TK_IDENT);
            char *field_name = tok_str(&field);
            if (p->expr_type && (p->expr_type->kind == TY_STRUCT || p->expr_type->kind == TY_UNION)) {
                Member *f = find_field(p->expr_type, field_name);
                if (f) {
                    // If the struct is on the stack, compute the field address from RBP
                    if (p->has_lvalue && p->lvalue_is_stack) {
                        int field_offset = p->lvalue_offset - f->offset;
                        OpSize sz = type_opsize(f->type);
                        if (sz == SZ_BYTE || sz == SZ_WORD) {
                            if (f->type->is_unsigned)
                                emit_movzx(p->gen, op_reg(REG_RAX, SZ_QWORD), op_mem(REG_RBP, -field_offset, sz));
                            else
                                emit_movsx(p->gen, op_reg(REG_RAX, SZ_QWORD), op_mem(REG_RBP, -field_offset, sz));
                        } else {
                            emit_mov(p->gen, op_reg(REG_RAX, sz), op_mem(REG_RBP, -field_offset, sz));
                        }
                        p->expr_type = f->type;
                        p->has_lvalue = 1;
                        p->lvalue_is_stack = 1;
                        p->lvalue_offset = field_offset;
                        p->lvalue_type = f->type;
                    } else {
                        // For non-stack structs, compute address and load value
                        emit_add(p->gen, op_reg(REG_RAX, SZ_QWORD), op_imm(f->offset));
                        /* Save field address to stack slot before loading value */
                        p->local_offset = ((p->local_offset + 7) / 8) * 8 + 8;
                        int addr_slot = p->local_offset;
                        emit_mov(p->gen, op_mem(REG_RBP, -addr_slot, SZ_QWORD), op_reg(REG_RAX, SZ_QWORD));
                        OpSize sz = type_opsize(f->type);
                        if (sz == SZ_BYTE || sz == SZ_WORD) {
                            if (f->type->is_unsigned)
                                emit_movzx(p->gen, op_reg(REG_RAX, SZ_QWORD), op_mem(REG_RAX, 0, sz));
                            else
                                emit_movsx(p->gen, op_reg(REG_RAX, SZ_QWORD), op_mem(REG_RAX, 0, sz));
                        } else {
                            emit_mov(p->gen, op_reg(REG_RAX, sz), op_mem(REG_RAX, 0, sz));
                        }
                        p->expr_type = f->type;
                        p->has_lvalue = 1;
                        p->lvalue_is_stack = 0;
                        p->lvalue_base = REG_RAX;
                        p->lvalue_offset = 0;
                        p->lvalue_type = f->type;
                        p->lvalue_addr_slot = addr_slot;
                    }
                }
            }
            free(field_name);
        } else if (t.kind == TK_ARROW) {
            next(p);
            Token field = expect(p, TK_IDENT);
            char *field_name = tok_str(&field);
            if (p->expr_type && p->expr_type->base && 
                (p->expr_type->base->kind == TY_STRUCT || p->expr_type->base->kind == TY_UNION)) {
                Member *f = find_field(p->expr_type->base, field_name);
                if (f) {
                    emit_add(p->gen, op_reg(REG_RAX, SZ_QWORD), op_imm(f->offset));
                    /* Save field address to stack slot before loading value */
                    p->local_offset = ((p->local_offset + 7) / 8) * 8 + 8;
                    int addr_slot = p->local_offset;
                    emit_mov(p->gen, op_mem(REG_RBP, -addr_slot, SZ_QWORD), op_reg(REG_RAX, SZ_QWORD));
                    OpSize sz = type_opsize(f->type);
                    if (sz == SZ_BYTE || sz == SZ_WORD) {
                        if (f->type->is_unsigned)
                            emit_movzx(p->gen, op_reg(REG_RAX, SZ_QWORD), op_mem(REG_RAX, 0, sz));
                        else
                            emit_movsx(p->gen, op_reg(REG_RAX, SZ_QWORD), op_mem(REG_RAX, 0, sz));
                    } else {
                        emit_mov(p->gen, op_reg(REG_RAX, sz), op_mem(REG_RAX, 0, sz));
                    }
                    p->expr_type = f->type;
                    p->has_lvalue = 1;
                    p->lvalue_is_stack = 0;
                    p->lvalue_base = REG_RAX;
                    p->lvalue_offset = 0;
                    p->lvalue_type = f->type;
                    p->lvalue_addr_slot = addr_slot;
                }
            }
            free(field_name);
        } else if (t.kind == TK_PLUSPLUS) {
            next(p);
            int old_val_slot = 0;
            if (!p->lvalue_is_stack && p->lvalue_addr_slot > 0) {
                /* Pointer-based lvalue: save old value before modifying */
                p->local_offset = ((p->local_offset + 7) / 8) * 8 + 8;
                old_val_slot = p->local_offset;
                emit_mov(p->gen, op_mem(REG_RBP, -old_val_slot, SZ_QWORD), op_reg(REG_RAX, SZ_QWORD));
            } else {
                emit_mov(p->gen, op_reg(REG_R10, SZ_QWORD), op_reg(REG_RAX, SZ_QWORD));
            }
            emit_add(p->gen, op_reg(REG_RAX, SZ_QWORD), op_imm(1));
            store_to_lvalue(p);
            if (old_val_slot > 0)
                emit_mov(p->gen, op_reg(REG_RAX, SZ_QWORD), op_mem(REG_RBP, -old_val_slot, SZ_QWORD));
            else
                emit_mov(p->gen, op_reg(REG_RAX, SZ_QWORD), op_reg(REG_R10, SZ_QWORD));
            p->has_lvalue = 0;
        } else if (t.kind == TK_MINUSMINUS) {
            next(p);
            int old_val_slot = 0;
            if (!p->lvalue_is_stack && p->lvalue_addr_slot > 0) {
                p->local_offset = ((p->local_offset + 7) / 8) * 8 + 8;
                old_val_slot = p->local_offset;
                emit_mov(p->gen, op_mem(REG_RBP, -old_val_slot, SZ_QWORD), op_reg(REG_RAX, SZ_QWORD));
            } else {
                emit_mov(p->gen, op_reg(REG_R10, SZ_QWORD), op_reg(REG_RAX, SZ_QWORD));
            }
            emit_sub(p->gen, op_reg(REG_RAX, SZ_QWORD), op_imm(1));
            store_to_lvalue(p);
            if (old_val_slot > 0)
                emit_mov(p->gen, op_reg(REG_RAX, SZ_QWORD), op_mem(REG_RBP, -old_val_slot, SZ_QWORD));
            else
                emit_mov(p->gen, op_reg(REG_RAX, SZ_QWORD), op_reg(REG_R10, SZ_QWORD));
            p->has_lvalue = 0;
        } else {
            break;
        }
    }
}

static void parse_unary(Parser *p);
static void parse_cast(Parser *p) {
    if (peek(p).kind == TK_LPAREN) {
        Lexer saved = p->lexer;
        next(p);
        if (is_type_start(p)) {
            Type *ty = parse_type_spec(p);
            while (peek(p).kind == TK_STAR) { next(p); ty = pointer_to(ty); }
            expect(p, TK_RPAREN);
            // Check for compound literal: (Type){...}
            if (peek(p).kind == TK_LBRACE) {
                next(p); // consume {
                // Allocate space on stack for the compound literal
                int align = ty->align;
                if (align < 1) align = 1;
                p->local_offset = ((p->local_offset + align - 1) / align) * align + ty->size;
                int offset = p->local_offset;
                // Parse initializer list
                int field_idx = 0;
                int elem_offset = 0;
                while (peek(p).kind != TK_RBRACE) {
                    if (peek(p).kind == TK_DOT) {
                        next(p);
                        Token field_name = expect(p, TK_IDENT);
                        expect(p, TK_ASSIGN);
                        parse_assign(p);
                        char *fname = tok_str(&field_name);
                        Member *f = find_field(ty, fname);
                        free(fname);
                        if (f) {
                            OpSize sz = type_opsize(f->type);
                            Operand src;
                            if (sz == SZ_BYTE) src = op_reg(REG_RAX, SZ_BYTE);
                            else if (sz == SZ_WORD) src = op_reg(REG_RAX, SZ_WORD);
                            else if (sz == SZ_DWORD) src = op_reg(REG_RAX, SZ_DWORD);
                            else src = op_reg(REG_RAX, SZ_QWORD);
                            emit_mov(p->gen, op_mem(REG_RBP, -(offset - f->offset), sz), src);
                        }
                    } else {
                        parse_assign(p);
                        if (ty->kind == TY_STRUCT || ty->kind == TY_UNION) {
                            if (field_idx < ty->member_count) {
                                Member *f = &ty->members[field_idx];
                                OpSize sz = type_opsize(f->type);
                                Operand src;
                                if (sz == SZ_BYTE) src = op_reg(REG_RAX, SZ_BYTE);
                                else if (sz == SZ_WORD) src = op_reg(REG_RAX, SZ_WORD);
                                else if (sz == SZ_DWORD) src = op_reg(REG_RAX, SZ_DWORD);
                                else src = op_reg(REG_RAX, SZ_QWORD);
                                emit_mov(p->gen, op_mem(REG_RBP, -(offset - f->offset), sz), src);
                            }
                        } else if (ty->kind == TY_ARRAY) {
                            OpSize sz = type_opsize(ty->base);
                            Operand src;
                            if (sz == SZ_BYTE) src = op_reg(REG_RAX, SZ_BYTE);
                            else if (sz == SZ_WORD) src = op_reg(REG_RAX, SZ_WORD);
                            else if (sz == SZ_DWORD) src = op_reg(REG_RAX, SZ_DWORD);
                            else src = op_reg(REG_RAX, SZ_QWORD);
                            emit_mov(p->gen, op_mem(REG_RBP, -(offset - elem_offset), sz), src);
                            elem_offset += ty->base->size;
                        } else {
                            OpSize sz = type_opsize(ty);
                            Operand src;
                            if (sz == SZ_BYTE) src = op_reg(REG_RAX, SZ_BYTE);
                            else if (sz == SZ_WORD) src = op_reg(REG_RAX, SZ_WORD);
                            else if (sz == SZ_DWORD) src = op_reg(REG_RAX, SZ_DWORD);
                            else src = op_reg(REG_RAX, SZ_QWORD);
                            emit_mov(p->gen, op_mem(REG_RBP, -offset, sz), src);
                        }
                        field_idx++;
                    }
                    if (peek(p).kind == TK_COMMA) next(p);
                }
                expect(p, TK_RBRACE);
                // For initialization, the values are already on the stack
                // Return the type info but mark as stack-based lvalue
                p->expr_type = ty;
                p->has_lvalue = 1;
                p->lvalue_is_stack = 1;
                p->lvalue_offset = offset;
                p->lvalue_type = ty;
            } else {
                // Regular cast expression
                parse_unary(p);
                p->expr_type = ty;
                p->has_lvalue = 0;
            }
            return;
        }
        p->lexer = saved;
    }
    parse_unary(p);
}

static void parse_unary(Parser *p) {
    Token t = peek(p);
    switch (t.kind) {
    case TK_PLUS:
        next(p); parse_cast(p); break;
    case TK_MINUS:
        next(p); parse_cast(p);
        emit_neg(p->gen, op_reg(REG_RAX, SZ_QWORD));
        p->has_lvalue = 0;
        break;
    case TK_AMP:
        next(p); parse_cast(p);
        if (p->has_lvalue && p->lvalue_is_stack)
            emit_lea(p->gen, op_reg(REG_RAX, SZ_QWORD), op_mem(REG_RBP, -(p->lvalue_offset), SZ_QWORD));
        p->expr_type = pointer_to(p->expr_type);
        p->has_lvalue = 0;
        break;
    case TK_STAR:
        next(p); parse_cast(p);
        if (p->expr_type->base) {
            p->expr_type = p->expr_type->base;
            /* Save address before loading value */
            p->local_offset = ((p->local_offset + 7) / 8) * 8 + 8;
            int addr_slot = p->local_offset;
            emit_mov(p->gen, op_mem(REG_RBP, -addr_slot, SZ_QWORD), op_reg(REG_RAX, SZ_QWORD));
            OpSize sz = type_opsize(p->expr_type);
            if (sz == SZ_BYTE || sz == SZ_WORD) {
                if (p->expr_type->is_unsigned)
                    emit_movzx(p->gen, op_reg(REG_RAX, SZ_QWORD), op_mem(REG_RAX, 0, sz));
                else
                    emit_movsx(p->gen, op_reg(REG_RAX, SZ_QWORD), op_mem(REG_RAX, 0, sz));
            } else {
                emit_mov(p->gen, op_reg(REG_RAX, SZ_QWORD), op_mem(REG_RAX, 0, sz));
            }
            p->lvalue_addr_slot = addr_slot;
        }
        p->has_lvalue = 1;
        p->lvalue_is_stack = 0;
        p->lvalue_base = REG_RAX;
        p->lvalue_offset = 0;
        p->lvalue_type = p->expr_type;
        break;
    case TK_BANG:
        next(p); parse_cast(p);
        emit_cmp(p->gen, op_reg(REG_RAX, SZ_QWORD), op_imm(0));
        emit_setz(p->gen, op_reg(REG_RAX, SZ_BYTE));
        emit_movzx(p->gen, op_reg(REG_RAX, SZ_QWORD), op_reg(REG_RAX, SZ_BYTE));
        p->expr_type = ty_int();
        p->has_lvalue = 0;
        break;
    case TK_TILDE:
        next(p); parse_cast(p);
        emit_not(p->gen, op_reg(REG_RAX, SZ_QWORD));
        p->has_lvalue = 0;
        break;
    case TK_PLUSPLUS:
        next(p); parse_unary(p);
        emit_add(p->gen, op_reg(REG_RAX, SZ_QWORD), op_imm(1));
        store_to_lvalue(p);
        p->has_lvalue = 0;
        break;
    case TK_MINUSMINUS:
        next(p); parse_unary(p);
        emit_sub(p->gen, op_reg(REG_RAX, SZ_QWORD), op_imm(1));
        store_to_lvalue(p);
        p->has_lvalue = 0;
        break;
    case TK_SIZEOF:
        next(p);
        if (peek(p).kind == TK_LPAREN) {
            Lexer saved = p->lexer;
            next(p);
            if (is_type_start(p)) {
                Type *ty = parse_type_spec(p);
                while (peek(p).kind == TK_STAR) { next(p); ty = pointer_to(ty); }
                expect(p, TK_RPAREN);
                emit_mov(p->gen, op_reg(REG_RAX, SZ_QWORD), op_imm(ty->size));
            } else {
                p->lexer = saved;
                parse_cast(p);
                emit_mov(p->gen, op_reg(REG_RAX, SZ_QWORD), op_imm(p->expr_type->size));
            }
        } else {
            parse_cast(p);
            emit_mov(p->gen, op_reg(REG_RAX, SZ_QWORD), op_imm(p->expr_type->size));
        }
        p->expr_type = ty_int();
        p->has_lvalue = 0;
        break;
    default:
        parse_postfix(p);
    }
}

static void parse_mul(Parser *p) {
    parse_unary(p);
    for (;;) {
        Token t = peek(p);
        if (t.kind == TK_STAR) {
            next(p);
            emit_push(p->gen, op_reg(REG_RAX, SZ_QWORD));
            parse_unary(p);
            emit_pop(p->gen, op_reg(REG_R10, SZ_QWORD));
            emit_imul(p->gen, op_reg(REG_RAX, SZ_QWORD), op_reg(REG_R10, SZ_QWORD));
        } else if (t.kind == TK_SLASH) {
            next(p);
            emit_push(p->gen, op_reg(REG_RAX, SZ_QWORD));
            parse_unary(p);
            emit_mov(p->gen, op_reg(REG_R11, SZ_QWORD), op_reg(REG_RAX, SZ_QWORD));
            emit_pop(p->gen, op_reg(REG_RAX, SZ_QWORD));
            emit_cqo(p);
            emit_idiv(p->gen, op_reg(REG_R11, SZ_QWORD));
        } else if (t.kind == TK_PERCENT) {
            next(p);
            emit_push(p->gen, op_reg(REG_RAX, SZ_QWORD));
            parse_unary(p);
            emit_mov(p->gen, op_reg(REG_R11, SZ_QWORD), op_reg(REG_RAX, SZ_QWORD));
            emit_pop(p->gen, op_reg(REG_RAX, SZ_QWORD));
            emit_cqo(p);
            emit_idiv(p->gen, op_reg(REG_R11, SZ_QWORD));
            emit_mov(p->gen, op_reg(REG_RAX, SZ_QWORD), op_reg(REG_RDX, SZ_QWORD));
        } else {
            break;
        }
        p->has_lvalue = 0;
    }
}

static void parse_add(Parser *p) {
    parse_mul(p);
    for (;;) {
        Token t = peek(p);
        if (t.kind == TK_PLUS) {
            next(p);
            emit_push(p->gen, op_reg(REG_RAX, SZ_QWORD));
            parse_mul(p);
            emit_pop(p->gen, op_reg(REG_R10, SZ_QWORD));
            if (p->expr_type && p->expr_type->kind == TY_PTR && p->expr_type->base) {
                emit_imul(p->gen, op_reg(REG_RAX, SZ_QWORD), op_imm(p->expr_type->base->size));
            }
            emit_add(p->gen, op_reg(REG_RAX, SZ_QWORD), op_reg(REG_R10, SZ_QWORD));
        } else if (t.kind == TK_MINUS) {
            next(p);
            emit_push(p->gen, op_reg(REG_RAX, SZ_QWORD));
            parse_mul(p);
            emit_pop(p->gen, op_reg(REG_R10, SZ_QWORD));
            emit_sub(p->gen, op_reg(REG_R10, SZ_QWORD), op_reg(REG_RAX, SZ_QWORD));
            emit_mov(p->gen, op_reg(REG_RAX, SZ_QWORD), op_reg(REG_R10, SZ_QWORD));
        } else {
            break;
        }
        p->has_lvalue = 0;
    }
}

static void parse_shift(Parser *p) {
    parse_add(p);
    for (;;) {
        Token t = peek(p);
        if (t.kind == TK_SHL) {
            next(p);
            emit_push(p->gen, op_reg(REG_RAX, SZ_QWORD));
            parse_add(p);
            emit_mov(p->gen, op_reg(REG_RCX, SZ_BYTE), op_reg(REG_RAX, SZ_BYTE));
            emit_pop(p->gen, op_reg(REG_RAX, SZ_QWORD));
            emit_shl(p->gen, op_reg(REG_RAX, SZ_QWORD), op_reg(REG_RCX, SZ_BYTE));
        } else if (t.kind == TK_SHR) {
            next(p);
            emit_push(p->gen, op_reg(REG_RAX, SZ_QWORD));
            parse_add(p);
            emit_mov(p->gen, op_reg(REG_RCX, SZ_BYTE), op_reg(REG_RAX, SZ_BYTE));
            emit_pop(p->gen, op_reg(REG_RAX, SZ_QWORD));
            emit_sar(p->gen, op_reg(REG_RAX, SZ_QWORD), op_reg(REG_RCX, SZ_BYTE));
        } else {
            break;
        }
        p->has_lvalue = 0;
    }
}

static void parse_relational(Parser *p) {
    parse_shift(p);
    for (;;) {
        Token t = peek(p);
        InstrKind cc;
        if (t.kind == TK_LT) cc = I_SETL;
        else if (t.kind == TK_GT) cc = I_SETG;
        else if (t.kind == TK_LE) cc = I_SETLE;
        else if (t.kind == TK_GE) cc = I_SETGE;
        else break;
        next(p);
        emit_push(p->gen, op_reg(REG_RAX, SZ_QWORD));
        parse_shift(p);
        emit_pop(p->gen, op_reg(REG_R10, SZ_QWORD));
        emit_cmp(p->gen, op_reg(REG_R10, SZ_QWORD), op_reg(REG_RAX, SZ_QWORD));
        Instr si; memset(&si, 0, sizeof(si));
        si.kind = cc; si.dst = op_reg(REG_RAX, SZ_BYTE);
        gen_emit(p->gen, si);
        emit_movzx(p->gen, op_reg(REG_RAX, SZ_QWORD), op_reg(REG_RAX, SZ_BYTE));
        p->expr_type = ty_int();
        p->has_lvalue = 0;
    }
}

static void parse_equality(Parser *p) {
    parse_relational(p);
    for (;;) {
        Token t = peek(p);
        InstrKind cc;
        if (t.kind == TK_EQ) cc = I_SETZ;
        else if (t.kind == TK_NE) cc = I_SETNZ;
        else break;
        next(p);
        emit_push(p->gen, op_reg(REG_RAX, SZ_QWORD));
        parse_relational(p);
        emit_pop(p->gen, op_reg(REG_R10, SZ_QWORD));
        emit_cmp(p->gen, op_reg(REG_R10, SZ_QWORD), op_reg(REG_RAX, SZ_QWORD));
        Instr si; memset(&si, 0, sizeof(si));
        si.kind = cc; si.dst = op_reg(REG_RAX, SZ_BYTE);
        gen_emit(p->gen, si);
        emit_movzx(p->gen, op_reg(REG_RAX, SZ_QWORD), op_reg(REG_RAX, SZ_BYTE));
        p->expr_type = ty_int();
        p->has_lvalue = 0;
    }
}

static void parse_bitand(Parser *p) {
    parse_equality(p);
    while (peek(p).kind == TK_AMP) {
        next(p);
        emit_push(p->gen, op_reg(REG_RAX, SZ_QWORD));
        parse_equality(p);
        emit_pop(p->gen, op_reg(REG_R10, SZ_QWORD));
        emit_and(p->gen, op_reg(REG_RAX, SZ_QWORD), op_reg(REG_R10, SZ_QWORD));
        p->has_lvalue = 0;
    }
}

static void parse_bitxor(Parser *p) {
    parse_bitand(p);
    while (peek(p).kind == TK_CARET) {
        next(p);
        emit_push(p->gen, op_reg(REG_RAX, SZ_QWORD));
        parse_bitand(p);
        emit_pop(p->gen, op_reg(REG_R10, SZ_QWORD));
        emit_xor(p->gen, op_reg(REG_RAX, SZ_QWORD), op_reg(REG_R10, SZ_QWORD));
        p->has_lvalue = 0;
    }
}

static void parse_bitor(Parser *p) {
    parse_bitxor(p);
    while (peek(p).kind == TK_PIPE) {
        next(p);
        emit_push(p->gen, op_reg(REG_RAX, SZ_QWORD));
        parse_bitxor(p);
        emit_pop(p->gen, op_reg(REG_R10, SZ_QWORD));
        emit_or(p->gen, op_reg(REG_RAX, SZ_QWORD), op_reg(REG_R10, SZ_QWORD));
        p->has_lvalue = 0;
    }
}

static void parse_logand(Parser *p) {
    parse_bitor(p);
    while (peek(p).kind == TK_ANDAND) {
        next(p);
        int false_label = next_label(p);
        int end_label = next_label(p);
        emit_cmp(p->gen, op_reg(REG_RAX, SZ_QWORD), op_imm(0));
        emit_je_id(p, false_label);
        parse_bitor(p);
        emit_cmp(p->gen, op_reg(REG_RAX, SZ_QWORD), op_imm(0));
        emit_je_id(p, false_label);
        emit_mov(p->gen, op_reg(REG_RAX, SZ_QWORD), op_imm(1));
        emit_jmp_id(p, end_label);
        emit_label_id(p, false_label);
        emit_mov(p->gen, op_reg(REG_RAX, SZ_QWORD), op_imm(0));
        emit_label_id(p, end_label);
        p->expr_type = ty_int();
        p->has_lvalue = 0;
    }
}

static void parse_logor(Parser *p) {
    parse_logand(p);
    while (peek(p).kind == TK_OROR) {
        next(p);
        int true_label = next_label(p);
        int end_label = next_label(p);
        emit_cmp(p->gen, op_reg(REG_RAX, SZ_QWORD), op_imm(0));
        emit_jne_id(p, true_label);
        parse_logand(p);
        emit_cmp(p->gen, op_reg(REG_RAX, SZ_QWORD), op_imm(0));
        emit_jne_id(p, true_label);
        emit_mov(p->gen, op_reg(REG_RAX, SZ_QWORD), op_imm(0));
        emit_jmp_id(p, end_label);
        emit_label_id(p, true_label);
        emit_mov(p->gen, op_reg(REG_RAX, SZ_QWORD), op_imm(1));
        emit_label_id(p, end_label);
        p->expr_type = ty_int();
        p->has_lvalue = 0;
    }
}

static void parse_cond(Parser *p) {
    parse_logor(p);
    if (peek(p).kind == TK_QUESTION) {
        next(p);
        int else_label = next_label(p);
        int end_label = next_label(p);
        emit_cmp(p->gen, op_reg(REG_RAX, SZ_QWORD), op_imm(0));
        emit_je_id(p, else_label);
        parse_assign(p);
        expect(p, TK_COLON);
        emit_jmp_id(p, end_label);
        emit_label_id(p, else_label);
        parse_cond(p);
        emit_label_id(p, end_label);
        p->has_lvalue = 0;
    }
}

static void parse_assign(Parser *p) {
    parse_cond(p);
    Token t = peek(p);
    if (t.kind == TK_ASSIGN) {
        next(p);
        // Save lvalue info
        int saved_has = p->has_lvalue;
        Register saved_base = p->lvalue_base;
        int saved_off = p->lvalue_offset;
        Type *saved_type = p->lvalue_type;
        int saved_stack = p->lvalue_is_stack;
        int saved_addr_slot = p->lvalue_addr_slot;
        int saved_global = p->lvalue_is_global;
        char saved_name[256];
        memcpy(saved_name, p->lvalue_name, sizeof(saved_name));
        parse_assign(p);
        // Restore lvalue info
        p->has_lvalue = saved_has;
        p->lvalue_base = saved_base;
        p->lvalue_offset = saved_off;
        p->lvalue_type = saved_type;
        p->lvalue_is_stack = saved_stack;
        p->lvalue_addr_slot = saved_addr_slot;
        p->lvalue_is_global = saved_global;
        memcpy(p->lvalue_name, saved_name, sizeof(p->lvalue_name));
        // Store the value
        if (p->has_lvalue && !p->lvalue_is_stack && p->lvalue_addr_slot > 0) {
            /* Pointer-based lvalue: load address from saved stack slot */
            emit_mov(p->gen, op_reg(REG_R10, SZ_QWORD), op_mem(REG_RBP, -(p->lvalue_addr_slot), SZ_QWORD));
            OpSize sz = type_opsize(p->lvalue_type);
            Operand src;
            if (sz == SZ_BYTE) src = op_reg(REG_RAX, SZ_BYTE);
            else if (sz == SZ_WORD) src = op_reg(REG_RAX, SZ_WORD);
            else if (sz == SZ_DWORD) src = op_reg(REG_RAX, SZ_DWORD);
            else src = op_reg(REG_RAX, SZ_QWORD);
            emit_mov(p->gen, op_mem(REG_R10, 0, sz), src);
        } else {
            store_to_lvalue(p);
        }
    } else if (t.kind == TK_ADDASSIGN || t.kind == TK_SUBASSIGN ||
               t.kind == TK_MULASSIGN || t.kind == TK_DIVASSIGN ||
               t.kind == TK_MODASSIGN || t.kind == TK_ANDASSIGN ||
               t.kind == TK_ORASSIGN || t.kind == TK_XORASSIGN) {
        next(p);
        emit_push(p->gen, op_reg(REG_RAX, SZ_QWORD));
        int saved_has = p->has_lvalue;
        Register saved_base = p->lvalue_base;
        int saved_off = p->lvalue_offset;
        Type *saved_type = p->lvalue_type;
        int saved_stack = p->lvalue_is_stack;
        int saved_addr_slot = p->lvalue_addr_slot;
        parse_assign(p);
        p->lvalue_addr_slot = saved_addr_slot;
        emit_pop(p->gen, op_reg(REG_R10, SZ_QWORD));
        if (t.kind == TK_ADDASSIGN) emit_add(p->gen, op_reg(REG_R10, SZ_QWORD), op_reg(REG_RAX, SZ_QWORD));
        else if (t.kind == TK_SUBASSIGN) emit_sub(p->gen, op_reg(REG_R10, SZ_QWORD), op_reg(REG_RAX, SZ_QWORD));
        else if (t.kind == TK_MULASSIGN) emit_imul(p->gen, op_reg(REG_R10, SZ_QWORD), op_reg(REG_RAX, SZ_QWORD));
        else if (t.kind == TK_ANDASSIGN) emit_and(p->gen, op_reg(REG_R10, SZ_QWORD), op_reg(REG_RAX, SZ_QWORD));
        else if (t.kind == TK_ORASSIGN) emit_or(p->gen, op_reg(REG_R10, SZ_QWORD), op_reg(REG_RAX, SZ_QWORD));
        else if (t.kind == TK_XORASSIGN) emit_xor(p->gen, op_reg(REG_R10, SZ_QWORD), op_reg(REG_RAX, SZ_QWORD));
        emit_mov(p->gen, op_reg(REG_RAX, SZ_QWORD), op_reg(REG_R10, SZ_QWORD));
        p->has_lvalue = saved_has;
        p->lvalue_base = saved_base;
        p->lvalue_offset = saved_off;
        p->lvalue_type = saved_type;
        p->lvalue_is_stack = saved_stack;
        store_to_lvalue(p);
    }
}

static void parse_expr(Parser *p) {
    parse_assign(p);
    while (peek(p).kind == TK_COMMA) {
        next(p);
        parse_assign(p);
    }
}

static void parse_statement(Parser *p);
static void parse_compound_stmt(Parser *p);

static void parse_var_decl(Parser *p, Type *base_type) {
    Type *ty = base_type;
    while (peek(p).kind == TK_STAR) { next(p); ty = pointer_to(ty); }
    Token name = expect(p, TK_IDENT);
    char *name_str = tok_str(&name);
    while (peek(p).kind == TK_LBRACKET) {
        next(p);
        if (peek(p).kind == TK_RBRACKET) {
            next(p);
            ty = pointer_to(ty);
        } else {
            int len = parse_constexpr(p);
            expect(p, TK_RBRACKET);
            ty = ty_array(ty, len);
        }
    }
    int align = ty->align;
    if (align < 1) align = 1;
    p->local_offset = ((p->local_offset + align - 1) / align) * align + ty->size;
    Symbol *s = sym_declare(xstrdup(name_str), SYM_VAR, ty);
    s->offset = p->local_offset;
    int var_offset = p->local_offset;  // Save variable's offset
    if (peek(p).kind == TK_ASSIGN) {
        next(p);
        if (peek(p).kind == TK_LBRACE) {
            // Skip compound initializer: {expr, expr, ...}
            next(p);
            int depth = 1;
            while (depth > 0 && peek(p).kind != TK_EOF) {
                if (peek(p).kind == TK_LBRACE) depth++;
                else if (peek(p).kind == TK_RBRACE) depth--;
                if (depth > 0) next(p);
            }
            if (peek(p).kind == TK_RBRACE) next(p);
        } else {
            parse_expr(p);
            // Handle struct/union assignment by copying
            if (p->has_lvalue && p->lvalue_is_stack && 
                (ty->kind == TY_STRUCT || ty->kind == TY_UNION)) {
                // Copy struct from source to destination
                int src_offset = p->lvalue_offset;
                int dst_offset = var_offset;
                int size = ty->size;
                // Copy in 8-byte chunks
                for (int i = 0; i < size; i += 8) {
                    int chunk = size - i;
                    if (chunk > 8) chunk = 8;
                    if (chunk == 8) {
                        emit_mov(p->gen, op_reg(REG_R10, SZ_QWORD), op_mem(REG_RBP, -(src_offset - i), SZ_QWORD));
                        emit_mov(p->gen, op_mem(REG_RBP, -(dst_offset - i), SZ_QWORD), op_reg(REG_R10, SZ_QWORD));
                    } else if (chunk == 4) {
                        emit_mov(p->gen, op_reg(REG_R10, SZ_DWORD), op_mem(REG_RBP, -(src_offset - i), SZ_DWORD));
                        emit_mov(p->gen, op_mem(REG_RBP, -(dst_offset - i), SZ_DWORD), op_reg(REG_R10, SZ_DWORD));
                    } else if (chunk == 2) {
                        emit_mov(p->gen, op_reg(REG_R10, SZ_WORD), op_mem(REG_RBP, -(src_offset - i), SZ_WORD));
                        emit_mov(p->gen, op_mem(REG_RBP, -(dst_offset - i), SZ_WORD), op_reg(REG_R10, SZ_WORD));
                    } else {
                        emit_mov(p->gen, op_reg(REG_R10, SZ_BYTE), op_mem(REG_RBP, -(src_offset - i), SZ_BYTE));
                        emit_mov(p->gen, op_mem(REG_RBP, -(dst_offset - i), SZ_BYTE), op_reg(REG_R10, SZ_BYTE));
                    }
                }
            } else {
                OpSize sz = type_opsize(ty);
                Operand src;
                if (sz == SZ_BYTE) src = op_reg(REG_RAX, SZ_BYTE);
                else if (sz == SZ_WORD) src = op_reg(REG_RAX, SZ_WORD);
                else if (sz == SZ_DWORD) src = op_reg(REG_RAX, SZ_DWORD);
                else src = op_reg(REG_RAX, SZ_QWORD);
                emit_mov(p->gen, op_mem(REG_RBP, -(var_offset), sz), src);
            }
        }
    }
    free(name_str);
}

static void parse_statement(Parser *p) {
    Token t = peek(p);
    if (t.kind == TK_LBRACE) {
        parse_compound_stmt(p);
    } else if (t.kind == TK_IF) {
        next(p);
        expect(p, TK_LPAREN);
        parse_expr(p);
        expect(p, TK_RPAREN);
        int else_label = next_label(p);
        int end_label = next_label(p);
        emit_cmp(p->gen, op_reg(REG_RAX, SZ_QWORD), op_imm(0));
        emit_je_id(p, else_label);
        parse_statement(p);
        if (peek(p).kind == TK_ELSE) {
            next(p);
            emit_jmp_id(p, end_label);
            emit_label_id(p, else_label);
            parse_statement(p);
            emit_label_id(p, end_label);
        } else {
            emit_label_id(p, else_label);
        }
    } else if (t.kind == TK_WHILE) {
        next(p);
        int loop_label = next_label(p);
        int end_label = next_label(p);
        int saved_break = p->break_label;
        int saved_cont = p->continue_label;
        p->break_label = end_label;
        p->continue_label = loop_label;
        emit_label_id(p, loop_label);
        expect(p, TK_LPAREN);
        parse_expr(p);
        expect(p, TK_RPAREN);
        emit_cmp(p->gen, op_reg(REG_RAX, SZ_QWORD), op_imm(0));
        emit_je_id(p, end_label);
        parse_statement(p);
        emit_jmp_id(p, loop_label);
        emit_label_id(p, end_label);
        p->break_label = saved_break;
        p->continue_label = saved_cont;
    } else if (t.kind == TK_DO) {
        next(p);
        int loop_label = next_label(p);
        int cont_label = next_label(p);
        int end_label = next_label(p);
        int saved_break = p->break_label;
        int saved_cont = p->continue_label;
        p->break_label = end_label;
        p->continue_label = cont_label;
        emit_label_id(p, loop_label);
        parse_statement(p);
        emit_label_id(p, cont_label);
        expect(p, TK_WHILE);
        expect(p, TK_LPAREN);
        parse_expr(p);
        expect(p, TK_RPAREN);
        expect(p, TK_SEMICOLON);
        emit_cmp(p->gen, op_reg(REG_RAX, SZ_QWORD), op_imm(0));
        emit_jne_id(p, loop_label);
        emit_label_id(p, end_label);
        p->break_label = saved_break;
        p->continue_label = saved_cont;
    } else if (t.kind == TK_FOR) {
        next(p);
        expect(p, TK_LPAREN);
        scope_push();
        if (peek(p).kind == TK_SEMICOLON) {
            next(p);
        } else if (is_type_start(p)) {
            Type *base = parse_type_spec(p);
            parse_var_decl(p, base);
            expect(p, TK_SEMICOLON);
        } else {
            parse_expr(p);
            expect(p, TK_SEMICOLON);
        }
        int loop_label = next_label(p);
        int cont_label = next_label(p);
        int end_label = next_label(p);
        int saved_break = p->break_label;
        int saved_cont = p->continue_label;
        p->break_label = end_label;
        p->continue_label = cont_label;
        emit_label_id(p, loop_label);
        if (peek(p).kind != TK_SEMICOLON) {
            parse_expr(p);
            emit_cmp(p->gen, op_reg(REG_RAX, SZ_QWORD), op_imm(0));
            emit_je_id(p, end_label);
        }
        expect(p, TK_SEMICOLON);
        Instr *before_incr = p->gen->funcs[p->gen->func_count - 1].tail;
        if (peek(p).kind != TK_RPAREN) parse_expr(p);
        expect(p, TK_RPAREN);
        Instr *incr_start = before_incr ? before_incr->next : p->gen->funcs[p->gen->func_count - 1].head;
        if (incr_start) {
            if (before_incr) before_incr->next = NULL;
            else p->gen->funcs[p->gen->func_count - 1].head = NULL;
            p->gen->funcs[p->gen->func_count - 1].tail = before_incr;
        }
        parse_statement(p);
        emit_label_id(p, cont_label);
        if (incr_start) {
            Function *f = &p->gen->funcs[p->gen->func_count - 1];
            Instr *end = incr_start;
            while (end->next) end = end->next;
            if (f->tail) f->tail->next = incr_start;
            else f->head = incr_start;
            f->tail = end;
        }
        emit_jmp_id(p, loop_label);
        emit_label_id(p, end_label);
        p->break_label = saved_break;
        p->continue_label = saved_cont;
        scope_pop();
    } else if (t.kind == TK_RETURN) {
        next(p);
        if (peek(p).kind != TK_SEMICOLON) {
            parse_expr(p);
        }
        expect(p, TK_SEMICOLON);
        emit_jmp_id(p, p->return_label);
    } else if (t.kind == TK_BREAK) {
        next(p);
        expect(p, TK_SEMICOLON);
        if (p->break_label < 0) error_at(t.start, "break outside loop");
        emit_jmp_id(p, p->break_label);
    } else if (t.kind == TK_CONTINUE) {
        next(p);
        expect(p, TK_SEMICOLON);
        if (p->continue_label < 0) error_at(t.start, "continue outside loop");
        emit_jmp_id(p, p->continue_label);
    } else if (t.kind == TK_SWITCH) {
        next(p);
        expect(p, TK_LPAREN);
        parse_expr(p);
        expect(p, TK_RPAREN);
        int end_label = next_label(p);
        int saved_break = p->break_label;
        p->break_label = end_label;
        // Save switch expression value
        emit_push(p->gen, op_reg(REG_RAX, SZ_QWORD));
        // Parse switch body
        expect(p, TK_LBRACE);
        scope_push();
        while (peek(p).kind != TK_RBRACE && peek(p).kind != TK_EOF) {
            if (peek(p).kind == TK_CASE) {
                next(p);
                parse_expr(p);
                expect(p, TK_COLON);
                emit_pop(p->gen, op_reg(REG_R10, SZ_QWORD));
                emit_push(p->gen, op_reg(REG_R10, SZ_QWORD));
                emit_cmp(p->gen, op_reg(REG_R10, SZ_QWORD), op_reg(REG_RAX, SZ_QWORD));
                int next_case = next_label(p);
                emit_jne_id(p, next_case);
                // Parse case body
                while (peek(p).kind != TK_CASE && peek(p).kind != TK_DEFAULT && 
                       peek(p).kind != TK_RBRACE && peek(p).kind != TK_EOF) {
                    parse_statement(p);
                }
                emit_label_id(p, next_case);
            } else if (peek(p).kind == TK_DEFAULT) {
                next(p);
                expect(p, TK_COLON);
                // Parse default body
                while (peek(p).kind != TK_RBRACE && peek(p).kind != TK_EOF) {
                    parse_statement(p);
                }
            } else {
                parse_statement(p);
            }
        }
        scope_pop();
        expect(p, TK_RBRACE);
        emit_label_id(p, end_label);
        p->break_label = saved_break;
    } else if (t.kind == TK_SEMICOLON) {
        next(p);
    } else if (is_type_start(p) || t.kind == TK_STATIC || t.kind == TK_EXTERN) {
        int is_static = 0;
        int is_extern = 0;
        if (t.kind == TK_STATIC) { is_static = 1; next(p); }
        else if (t.kind == TK_EXTERN) { is_extern = 1; next(p); }
        Type *base = parse_type_spec(p);
        parse_var_decl(p, base);
        expect(p, TK_SEMICOLON);
    } else {
        parse_expr(p);
        expect(p, TK_SEMICOLON);
    }
}

static void parse_compound_stmt(Parser *p) {
    expect(p, TK_LBRACE);
    scope_push();
    while (peek(p).kind != TK_RBRACE && peek(p).kind != TK_EOF)
        parse_statement(p);
    expect(p, TK_RBRACE);
    scope_pop();
}

static void parse_func(Parser *p, Type *ret_type, char *name, int is_static) {
    expect(p, TK_LPAREN);
    Type **param_types = NULL;
    char **param_names = NULL;
    int param_count = 0;
    int is_variadic = 0;
    p->local_offset = 0;
    if (peek(p).kind != TK_RPAREN) {
        do {
            if (peek(p).kind == TK_ELLIPSIS) { next(p); is_variadic = 1; break; }
            Type *pty = parse_type_spec(p);
            while (peek(p).kind == TK_STAR) { next(p); pty = pointer_to(pty); }
            char *pname = NULL;
            if (peek(p).kind == TK_IDENT) {
                Token pname_tok = next(p);
                pname = tok_str(&pname_tok);
            }
            // Handle array parameters: int foo(int arr[]) => int foo(int *arr)
            while (peek(p).kind == TK_LBRACKET) {
                next(p);
                if (peek(p).kind == TK_RBRACKET) {
                    next(p);
                    pty = pointer_to(pty);
                } else {
                    // Array with size - skip the size expression
                    while (peek(p).kind != TK_RBRACKET && peek(p).kind != TK_EOF) next(p);
                    if (peek(p).kind == TK_RBRACKET) next(p);
                    pty = pointer_to(pty);
                }
            }
            param_types = xrealloc(param_types, sizeof(Type*) * (param_count + 1));
            param_names = xrealloc(param_names, sizeof(char*) * (param_count + 1));
            param_types[param_count] = pty;
            param_names[param_count] = pname;
            param_count++;
        } while (peek(p).kind == TK_COMMA && (next(p), 1));
    }
    expect(p, TK_RPAREN);
    Type *func_type = ty_func(ret_type, param_types, param_count, is_variadic);
    sym_declare(xstrdup(name), SYM_FUNC, func_type);
    gen_func_begin(p->gen, name, param_count, is_static);
    p->cur_func_ret_type = ret_type;
    p->return_label = next_label(p);
    p->break_label = -1;
    p->continue_label = -1;
    scope_push();
    {
        int local_off = 0;
        for (int i = 0; i < param_count; i++) {
            local_off += 8;
            char *pname = param_names[i];
            if (!pname) {
                pname = xmalloc(32);
                snprintf(pname, 32, "__p%d", i);
            }
            Symbol *ps = sym_declare(pname, SYM_VAR, param_types[i]);
            ps->offset = local_off;
        }
        p->local_offset = local_off;
    }
    parse_compound_stmt(p);
    int frame_size = p->local_offset;
    if (frame_size % 16 != 0) frame_size += 16 - (frame_size % 16);
    emit_label_id(p, p->return_label);
    emit_mov(p->gen, op_reg(REG_RSP, SZ_QWORD), op_reg(REG_RBP, SZ_QWORD));
    emit_pop(p->gen, op_reg(REG_RBP, SZ_QWORD));
    emit_ret(p->gen);
    gen_func_end(p->gen);
    Function *f = &p->gen->funcs[p->gen->func_count - 1];
    Instr *prologue[6];
    int npro = 0;
    Instr pi;
    memset(&pi, 0, sizeof(pi));
    pi.kind = I_PUSH; pi.dst = op_reg(REG_RBP, SZ_QWORD);
    prologue[npro] = xmalloc(sizeof(Instr)); memcpy(prologue[npro], &pi, sizeof(Instr)); npro++;
    pi.kind = I_MOV; pi.dst = op_reg(REG_RBP, SZ_QWORD); pi.src = op_reg(REG_RSP, SZ_QWORD);
    prologue[npro] = xmalloc(sizeof(Instr)); memcpy(prologue[npro], &pi, sizeof(Instr)); npro++;
    if (frame_size > 4096) {
        /* Windows x64 stack probing for large frames */
        char probe_buf[64];
        Instr probe1;
        memset(&probe1, 0, sizeof(Instr));
        probe1.kind = I_RAW;
        snprintf(probe_buf, sizeof(probe_buf), "mov rax, %d", frame_size);
        strncpy(probe1.dst.label, probe_buf, sizeof(probe1.dst.label) - 1);
        prologue[npro] = xmalloc(sizeof(Instr)); memcpy(prologue[npro], &probe1, sizeof(Instr)); npro++;
        Instr probe2;
        memset(&probe2, 0, sizeof(Instr));
        probe2.kind = I_RAW;
        strncpy(probe2.dst.label, "call ___chkstk_ms", sizeof(probe2.dst.label) - 1);
        prologue[npro] = xmalloc(sizeof(Instr)); memcpy(prologue[npro], &probe2, sizeof(Instr)); npro++;
    }
    if (frame_size > 0) {
        pi.kind = I_SUB; pi.dst = op_reg(REG_RSP, SZ_QWORD); pi.src = op_imm(frame_size);
        prologue[npro] = xmalloc(sizeof(Instr)); memcpy(prologue[npro], &pi, sizeof(Instr)); npro++;
    }
    {int i; for (i = 0; i < npro - 1; i++) prologue[i]->next = prologue[i + 1];}
    {Instr *old_head;
    memcpy(&old_head, &f->head, sizeof(Instr *));
    prologue[npro - 1]->next = old_head;
    memcpy(&f->head, &prologue[0], sizeof(Instr *));
    if (!old_head) memcpy(&f->tail, &prologue[npro - 1], sizeof(Instr *));}
    // Save register parameters to stack AFTER stack probing
    {static const Register param_regs[] = {REG_RCX, REG_RDX, REG_R8, REG_R9};
    int i;
    Instr *last_pro = prologue[npro - 1];
    for (i = 0; i < param_count && i < 4; i++) {
        int offset = (i + 1) * 8;
        Instr si;
        memset(&si, 0, sizeof(Instr));
        si.kind = I_MOV;
        si.dst = op_mem(REG_RBP, -offset, SZ_QWORD);
        si.src = op_reg(param_regs[i], SZ_QWORD);
        Instr *store = xmalloc(sizeof(Instr));
        memcpy(store, &si, sizeof(Instr));
        Instr *old_next;
        memcpy(&old_next, &last_pro->next, sizeof(Instr *));
        store->next = old_next;
        memcpy(&last_pro->next, &store, sizeof(Instr *));
        last_pro = store;
    }}
    scope_pop();
    free(name);
}

void parse_translation_unit(Parser *p) {
    scope_push();
    while (peek(p).kind != TK_EOF) {
        if (peek(p).kind == TK_TYPEDEF) {
            next(p);
            Type *base_type = parse_type_spec(p);
            Type *ty = base_type;
            while (peek(p).kind == TK_STAR) { next(p); ty = pointer_to(ty); }
            Token name = expect(p, TK_IDENT);
            sym_declare_typedef(tok_str(&name), ty);
            expect(p, TK_SEMICOLON);
            continue;
        }
        // Handle storage classes
        int is_static = 0;
        int is_extern = 0;
        if (peek(p).kind == TK_STATIC) { is_static = 1; next(p); }
        else if (peek(p).kind == TK_EXTERN) { is_extern = 1; next(p); }
        Type *base_type = parse_type_spec(p);
        Type *ty = base_type;
        while (peek(p).kind == TK_STAR) { next(p); ty = pointer_to(ty); }
        // Check if this is just a struct/union/enum definition without a variable
        if (peek(p).kind == TK_SEMICOLON) {
            next(p);
            continue;
        }
        Token name = expect(p, TK_IDENT);
        char *name_str = tok_str(&name);
        // Handle array brackets for global/static variables
        while (peek(p).kind == TK_LBRACKET) {
            next(p);
            if (peek(p).kind == TK_RBRACKET) {
                next(p);
                ty = ty_array(ty, 0);
            } else {
                Token size_tok = next(p);
                int size = (int)size_tok.ival;
                expect(p, TK_RBRACKET);
                ty = ty_array(ty, size);
            }
        }
        if (peek(p).kind == TK_LPAREN) {
            // Check if this is a function prototype (no body)
            Lexer saved = p->lexer;
            next(p); // consume (
            // Parse parameter list
            int is_proto = 0;
            if (peek(p).kind == TK_RPAREN) {
                next(p); // consume )
                if (peek(p).kind == TK_SEMICOLON) {
                    is_proto = 1;
                }
            } else {
                // Skip through parameter list
                int depth = 1;
                while (depth > 0 && peek(p).kind != TK_EOF) {
                    if (peek(p).kind == TK_LPAREN) depth++;
                    else if (peek(p).kind == TK_RPAREN) {
                        depth--;
                        if (depth == 0) {
                            next(p); // consume )
                            if (peek(p).kind == TK_SEMICOLON) {
                                is_proto = 1;
                            }
                            break;
                        }
                    }
                    next(p);
                }
            }
            p->lexer = saved;
            
            if (is_proto) {
                // Function prototype - just declare the symbol
                next(p); // consume (
                Type **param_types = NULL;
                int param_count = 0;
                int is_variadic = 0;
                if (peek(p).kind != TK_RPAREN) {
                    do {
                        if (peek(p).kind == TK_ELLIPSIS) { next(p); is_variadic = 1; break; }
                        Type *pty = parse_type_spec(p);
                        while (peek(p).kind == TK_STAR) { next(p); pty = pointer_to(pty); }
                        if (peek(p).kind == TK_IDENT) next(p); // skip param name
                        // Handle array parameters in prototypes
                        while (peek(p).kind == TK_LBRACKET) {
                            next(p);
                            if (peek(p).kind == TK_RBRACKET) {
                                next(p);
                                pty = pointer_to(pty);
                            } else {
                                while (peek(p).kind != TK_RBRACKET && peek(p).kind != TK_EOF) next(p);
                                if (peek(p).kind == TK_RBRACKET) next(p);
                                pty = pointer_to(pty);
                            }
                        }
                        param_types = xrealloc(param_types, sizeof(Type*) * (param_count + 1));
                        param_types[param_count++] = pty;
                    } while (peek(p).kind == TK_COMMA && (next(p), 1));
                }
                expect(p, TK_RPAREN);
                Type *func_type = ty_func(ty, param_types, param_count, is_variadic);
                sym_declare(xstrdup(name_str), SYM_FUNC, func_type);
                expect(p, TK_SEMICOLON);
                free(name_str);
            } else {
                parse_func(p, ty, name_str, is_static);
            }
        } else {
            sym_declare(xstrdup(name_str), SYM_VAR, ty);
            register_global(p, name_str, ty->size);
            if (peek(p).kind == TK_ASSIGN) next(p);
            if (peek(p).kind != TK_SEMICOLON) while (peek(p).kind != TK_SEMICOLON && peek(p).kind != TK_EOF) next(p);
            if (peek(p).kind == TK_SEMICOLON) next(p);
            free(name_str);
        }
    }
    scope_pop();
}

void parser_init(Parser *p, char *source, CodeGen *gen) {
    lexer_init(&p->lexer, source);
    p->gen = gen;
    p->label_count = 0;
    p->local_offset = 0;
    p->cur_func_ret_type = NULL;
    p->return_label = -1;
    p->break_label = -1;
    p->continue_label = -1;
    p->expr_type = ty_int();
    p->has_lvalue = 0;
    p->lvalue_base = REG_NONE;
    p->lvalue_offset = 0;
    p->lvalue_type = NULL;
    p->lvalue_is_stack = 0;
    p->lvalue_addr_slot = 0;
    p->lvalue_is_global = 0;
    p->string_count = 0;
    p->global_count = 0;
}

char *parser_flush(Parser *p) {
    gen_flush(p->gen);
    char *code = gen_output(p->gen);
    int code_len = strlen(code);
    int extra = 0;
    for (int i = 0; i < p->string_count; i++)
        extra += 64 + p->strings[i].len * 4;
    extra += p->global_count * 64;
    if (extra == 0) return code;
    char *out = xmalloc(code_len + extra + 64);
    memcpy(out, code, code_len);
    int pos = code_len;
    /* Emit .rodata section for string literals */
    if (p->string_count > 0) {
        pos += snprintf(out + pos, extra + 64 - pos, "\nsection .rodata\n");
        for (int i = 0; i < p->string_count; i++) {
            pos += snprintf(out + pos, extra + 64 - pos, "%s db ", p->strings[i].label);
            if (p->strings[i].len == 0) {
                pos += snprintf(out + pos, extra + 64 - pos, "0\n");
            } else {
                for (int j = 0; j < p->strings[i].len; j++) {
                    if (j > 0) out[pos++] = ',';
                    pos += snprintf(out + pos, 16, "%d", (unsigned char)p->strings[i].data[j]);
                }
                pos += snprintf(out + pos, extra + 64 - pos, ",0\n");
            }
        }
    }
    /* Emit .bss section for global variables */
    if (p->global_count > 0) {
        pos += snprintf(out + pos, extra + 64 - pos, "\nsection .bss\n");
        for (int i = 0; i < p->global_count; i++) {
            int sz = p->globals[i].size > 0 ? p->globals[i].size : 8;
            pos += snprintf(out + pos, extra + 64 - pos, "common %s %d\n", p->globals[i].name, sz);
        }
    }
    return out;
}
