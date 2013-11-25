/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *  Copyright (c) ngx_mruby developers 2012-
 *
 *  Use and distribution licensed under the MIT license.
 *  See LICENSE for full text.
 *
 */

#include "ngx_http_mruby_state.h"
#include "ngx_http_mruby_module.h"
#include "ngx_http_mruby_class.h"

#include <mruby.h>
#include <mruby/proc.h>
#include <mruby/data.h>
#include <mruby/compile.h>
#include <mruby/string.h>

#define GC_ARENA_RESTORE mrb_gc_arena_restore(mrb, 0);

static ngx_int_t ngx_http_mruby_class_init(mrb_state *mrb);

static ngx_int_t ngx_http_mruby_class_init(mrb_state *mrb)
{
    struct RClass *class;

    class = mrb_define_module(mrb, "Nginx");

    ngx_http_mruby_core_init(mrb, class);          GC_ARENA_RESTORE;
    ngx_http_mruby_request_class_init(mrb, class); GC_ARENA_RESTORE;
    ngx_http_mruby_variable_class_init(mrb, class);GC_ARENA_RESTORE;
    ngx_http_mruby_context_class_init(mrb, class); GC_ARENA_RESTORE;
    ngx_http_mruby_digest_class_init(mrb, class);  GC_ARENA_RESTORE;
    ngx_http_mruby_time_class_init(mrb, class);    GC_ARENA_RESTORE;
    ngx_http_mruby_base64_class_init(mrb, class);  GC_ARENA_RESTORE;
    ngx_http_mruby_regex_class_init(mrb);          GC_ARENA_RESTORE;

    return NGX_OK;
}

static ngx_int_t ngx_http_mruby_gencode_state(ngx_http_mruby_state_t *state, ngx_http_mruby_code_t *code)
{
    FILE *mrb_file;
    struct mrb_parser_state *p;

    if ((mrb_file = fopen((char *)code->code.file, "r")) == NULL) {
        return NGX_ERROR;
    }

    state->ai = mrb_gc_arena_save(state->mrb);
    p         = mrb_parse_file(state->mrb, mrb_file, NULL);
    code->proc   = mrb_generate_code(state->mrb, p);

    mrb_pool_close(p->pool);
    fclose(mrb_file);

    if (code->proc == NULL) {
        return NGX_ERROR;
    }

    return NGX_OK;
}

ngx_int_t ngx_http_mruby_state_reinit_from_file(ngx_http_mruby_state_t *state, ngx_http_mruby_code_t *code)
{
    if (state == NGX_CONF_UNSET_PTR) {
        return NGX_ERROR;
    }
    if (ngx_http_mruby_gencode_state(state, code) != NGX_OK) {
        return NGX_ERROR;
    }
    return NGX_OK;
}

ngx_http_mruby_code_t *ngx_http_mruby_mrb_code_from_file(ngx_pool_t *pool, ngx_str_t *code_file_path)
{
    ngx_http_mruby_code_t *code;
    size_t          len;
    u_char         *p;

    code = ngx_pcalloc(pool, sizeof(*code));
    if (code == NULL) {
        return NGX_CONF_UNSET_PTR;
    }

    len = ngx_strlen((char *)code_file_path->data);
    if (len == 0) {
        return NGX_CONF_UNSET_PTR;
    } else if (code_file_path->data[0] == '/') {
        code->code.file = ngx_pcalloc(pool, len + 1);
        if (code->code.file == NULL) {
            return NGX_CONF_UNSET_PTR;
        }
        ngx_cpystrn((u_char *)code->code.file, (u_char *)code_file_path->data, code_file_path->len + 1);
    } else {
        code->code.file = ngx_pcalloc(pool, ngx_cycle->conf_prefix.len + len + 1);
        if (code->code.file == NULL) {
            return NGX_CONF_UNSET_PTR;
        }
        p = ngx_cpystrn((u_char *)code->code.file, (u_char *)ngx_cycle->conf_prefix.data, ngx_cycle->conf_prefix.len + 1);
        ngx_cpystrn(p, (u_char *)code_file_path->data, code_file_path->len + 1);
    }
    code->code_type = NGX_HTTP_MRUBY_CODE_TYPE_FILE;
    return code;
}

ngx_http_mruby_code_t *ngx_http_mruby_mrb_code_from_string(ngx_pool_t *pool, ngx_str_t *code_s)
{
    ngx_http_mruby_code_t *code;
    size_t len;

    code = ngx_pcalloc(pool, sizeof(*code));
    if (code == NULL) {
        return NGX_CONF_UNSET_PTR;
    }

    len = ngx_strlen(code_s->data);
    code->code.string = ngx_pcalloc(pool, len + 1);
    if (code->code.string == NULL) {
        return NGX_CONF_UNSET_PTR;
    }
    ngx_cpystrn((u_char *)code->code.string, code_s->data, len + 1);
    code->code_type = NGX_HTTP_MRUBY_CODE_TYPE_STRING;
    return code;
}

ngx_int_t ngx_http_mruby_shared_state_init(ngx_http_mruby_state_t *state)
{
    mrb_state *mrb;

    mrb = mrb_open();
    ngx_http_mruby_class_init(mrb);

    state->mrb = mrb;

    return NGX_OK;
}

ngx_int_t ngx_http_mruby_shared_state_compile(ngx_http_mruby_state_t *state, ngx_http_mruby_code_t *code)
{
    FILE *mrb_file;
    struct mrb_parser_state *p;

    if (code->code_type == NGX_HTTP_MRUBY_CODE_TYPE_FILE) {
        if ((mrb_file = fopen((char *)code->code.file, "r")) == NULL) {
            return NGX_ERROR;
        }
        p = mrb_parse_file(state->mrb, mrb_file, NULL);
        fclose(mrb_file);
    } else {
        p = mrb_parse_string(state->mrb, (char *)code->code.string, NULL);
    }

    code->proc = mrb_generate_code(state->mrb, p);
    mrb_pool_close(p->pool);

    if (code->proc == NULL) {
        return NGX_ERROR;
    }

    return NGX_OK;
}


