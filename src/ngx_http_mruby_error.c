/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *  Copyright (c) ngx_mruby developers 2012-
 *
 *  Use and distribution licensed under the MIT license.
 *  See LICENSE for full text.
 *
 */

#include "ngx_http_mruby_error.h"

#include <mruby/string.h>
#include <mruby/array.h>
#include <mruby/variable.h>

void ngx_mrb_raise_error(ngx_mrb_state_t *state, ngx_mrb_code_t *code, ngx_http_request_t *r)
{
    if (code->code_type == NGX_MRB_CODE_TYPE_FILE) {
        ngx_mrb_raise_file_error(state->mrb, mrb_obj_value(state->mrb->exc), r, code->code.file);
    } else {
        ngx_mrb_raise_inline_error(state->mrb, mrb_obj_value(state->mrb->exc), r);
    }
}

void ngx_mrb_raise_inline_error(mrb_state *mrb, mrb_value obj, ngx_http_request_t *r)
{  
    struct RString *str;
    char *err_out;
    
    obj = mrb_funcall(mrb, obj, "inspect", 0);
    if (mrb_type(obj) == MRB_TT_STRING) {
        str = mrb_str_ptr(obj);
        err_out = str->ptr;
        ngx_log_error(NGX_LOG_ERR
            , r->connection->log
            , 0
            , "mrb_run failed. error: %s"
            , err_out
        );
    }
}

void ngx_mrb_raise_file_error(mrb_state *mrb, mrb_value obj, ngx_http_request_t *r, char *code_file)
{  
    struct RString *str;
    char *err_out;
    
    obj = mrb_funcall(mrb, obj, "inspect", 0);
    if (mrb_type(obj) == MRB_TT_STRING) {
        str = mrb_str_ptr(obj);
        err_out = str->ptr;
        ngx_log_error(NGX_LOG_ERR
            , r->connection->log
            , 0
            , "mrb_run failed. file: %s error: %s"
            , code_file
            , err_out
        );
    }
}

void ngx_mrb_raise_conf_error(ngx_mrb_state_t *state, ngx_mrb_code_t *code, ngx_conf_t *cf)
{
    if (code->code_type == NGX_MRB_CODE_TYPE_FILE) {
        ngx_mrb_raise_file_conf_error(state->mrb, mrb_obj_value(state->mrb->exc), cf, code->code.file);
    } else {
        ngx_mrb_raise_inline_conf_error(state->mrb, mrb_obj_value(state->mrb->exc), cf);
    }
}

void ngx_mrb_raise_inline_conf_error(mrb_state *mrb, mrb_value obj, ngx_conf_t *cf)
{  
    struct RString *str;
    char *err_out;
    
    obj = mrb_funcall(mrb, obj, "inspect", 0);
    if (mrb_type(obj) == MRB_TT_STRING) {
        str = mrb_str_ptr(obj);
        err_out = str->ptr;
        ngx_conf_log_error(NGX_LOG_ERR
            , cf
            , 0
            , "mrb_run failed. error: %s"
            , err_out
        );
    }
}

void ngx_mrb_raise_file_conf_error(mrb_state *mrb, mrb_value obj, ngx_conf_t *cf, char *code_file)
{  
    struct RString *str;
    char *err_out;
    
    obj = mrb_funcall(mrb, obj, "inspect", 0);
    if (mrb_type(obj) == MRB_TT_STRING) {
        str = mrb_str_ptr(obj);
        err_out = str->ptr;
        ngx_conf_log_error(NGX_LOG_ERR
            , cf
            , 0
            , "mrb_run failed. file: %s error: %s"
            , code_file
            , err_out
        );
    }
}
