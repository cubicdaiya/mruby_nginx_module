/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *  Copyright (c) ngx_mruby developers 2012-
 *
 *  Use and distribution licensed under the MIT license.
 *  See LICENSE for full text.
 *
 */

#ifndef NGX_HTTP_MRUBY_ERROR_H
#define NGX_HTTP_MRUBY_ERROR_H

#include <ngx_http.h>
#include <mruby.h>

#include "ngx_http_mruby_core.h"

void ngx_mrb_raise_error(ngx_mrb_state_t *state, ngx_mrb_code_t *code, ngx_http_request_t *r);
void ngx_mrb_raise_inline_error(mrb_state *mrb, mrb_value obj, ngx_http_request_t *r);
void ngx_mrb_raise_file_error(mrb_state *mrb, mrb_value obj, ngx_http_request_t *r, char *code_file);
void ngx_mrb_raise_conf_error(ngx_mrb_state_t *state, ngx_mrb_code_t *code, ngx_conf_t *cf);
void ngx_mrb_raise_inline_conf_error(mrb_state *mrb, mrb_value obj, ngx_conf_t *cf);
void ngx_mrb_raise_file_conf_error(mrb_state *mrb, mrb_value obj, ngx_conf_t *cf, char *code_file);

#endif // NGX_HTTP_MRUBY_ERROR_H
