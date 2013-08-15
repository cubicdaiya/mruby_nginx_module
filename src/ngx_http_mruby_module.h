/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *  Copyright (c) ngx_mruby developers 2012-
 *
 *  Use and distribution licensed under the MIT license.
 *  See LICENSE for full text.
 *
 */

#ifndef NGX_HTTP_MRUBY_MODULE_H
#define NGX_HTTP_MRUBY_MODULE_H

#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>

#include "ngx_http_mruby_core.h"
#include "ngx_http_mruby_init.h"

#if defined(NDK) && NDK
typedef struct {
    size_t           size;
    ngx_str_t        script;
    ngx_mrb_state_t *state;
    ngx_mrb_code_t  *code;
} ngx_http_mruby_set_var_data_t;
#include <ndk.h>
#endif

#define MODULE_NAME        "mruby_nginx_module"
#define MODULE_VERSION     "0.0.2"

extern ngx_module_t  ngx_http_mruby_module;

typedef struct ngx_http_mruby_main_conf_t {
    ngx_mrb_state_t *state;
    ngx_mrb_code_t  *init_code;
    ngx_int_t        (*init_handler)(ngx_conf_t *conf, struct ngx_http_mruby_main_conf_t *mmcf);
    unsigned         enabled_post_read_handler:1;
    unsigned         enabled_server_rewrite_handler:1;
    unsigned         enabled_rewrite_handler:1;
    unsigned         enabled_access_handler:1;
    unsigned         enabled_content_handler:1;
    unsigned         enabled_log_handler:1;
    unsigned         enabled_header_filter:1;
    unsigned         enabled_body_filter:1;
} ngx_http_mruby_main_conf_t;

typedef struct ngx_http_mruby_loc_conf_t {
    // 
    ngx_mrb_code_t *post_read_code;
    ngx_mrb_code_t *server_rewrite_code;
    ngx_mrb_code_t *rewrite_code;
    ngx_mrb_code_t *access_code;
    ngx_mrb_code_t *content_code;
    ngx_mrb_code_t *log_code;
    ngx_mrb_code_t *post_read_inline_code;
    ngx_mrb_code_t *server_rewrite_inline_code;
    ngx_mrb_code_t *rewrite_inline_code;
    ngx_mrb_code_t *access_inline_code;
    ngx_mrb_code_t *content_inline_code;
    ngx_mrb_code_t *log_inline_code;
    ngx_mrb_code_t *header_filter_code;
    ngx_mrb_code_t *header_filter_inline_code;
    ngx_mrb_code_t *body_filter_code;
    ngx_mrb_code_t *body_filter_inline_code;
    ngx_flag_t      cached;

    ngx_int_t (*init_handler)(ngx_conf_t *conf, struct ngx_http_mruby_main_conf_t *mmcf);
    ngx_int_t (*post_read_handler)(ngx_http_request_t *r);
    ngx_int_t (*server_rewrite_handler)(ngx_http_request_t *r);
    ngx_int_t (*rewrite_handler)(ngx_http_request_t *r);
    ngx_int_t (*access_handler)(ngx_http_request_t *r);
    ngx_int_t (*content_handler)(ngx_http_request_t *r);
    ngx_int_t (*log_handler)(ngx_http_request_t *r);

    // filter handlers
    ngx_http_handler_pt            header_filter_handler;
    ngx_http_output_body_filter_pt body_filter_handler;
} ngx_http_mruby_loc_conf_t;

#endif // NGX_HTTP_MRUBY_MODULE_H
