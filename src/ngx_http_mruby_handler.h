/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *  Copyright (c) ngx_mruby developers 2012-
 *
 *  Use and distribution licensed under the MIT license.
 *  See LICENSE for full text.
 *
 */

#ifndef NGX_HTTP_MRUBY_HANDLER_H
#define NGX_HTTP_MRUBY_HANDLER_H

#include <nginx.h>
#include <ngx_http.h>

#include "ngx_http_mruby_core.h"
#include "ngx_http_mruby_init.h"
#include "ngx_http_mruby_module.h"

ngx_int_t ngx_http_mruby_init_handler(ngx_conf_t *cf, ngx_http_mruby_main_conf_t *mmcf);
ngx_int_t ngx_http_mruby_post_read_handler(ngx_http_request_t *r);
ngx_int_t ngx_http_mruby_server_rewrite_handler(ngx_http_request_t *r);
ngx_int_t ngx_http_mruby_rewrite_handler(ngx_http_request_t *r);
ngx_int_t ngx_http_mruby_access_handler(ngx_http_request_t *r);
ngx_int_t ngx_http_mruby_content_handler(ngx_http_request_t *r);
ngx_int_t ngx_http_mruby_log_handler(ngx_http_request_t *r);
ngx_int_t ngx_http_mruby_post_read_inline_handler(ngx_http_request_t *r);
ngx_int_t ngx_http_mruby_server_rewrite_inline_handler(ngx_http_request_t *r);
ngx_int_t ngx_http_mruby_rewrite_inline_handler(ngx_http_request_t *r);
ngx_int_t ngx_http_mruby_access_inline_handler(ngx_http_request_t *r);
ngx_int_t ngx_http_mruby_content_inline_handler(ngx_http_request_t *r);
ngx_int_t ngx_http_mruby_log_inline_handler(ngx_http_request_t *r);

#if defined(NDK) && NDK
ngx_int_t ngx_http_mruby_set_handler(ngx_http_request_t *r, ngx_str_t *val,
                                     ngx_http_variable_value_t *v, void *data);
ngx_int_t ngx_http_mruby_set_inline_handler(ngx_http_request_t *r, ngx_str_t *val,
                                            ngx_http_variable_value_t *v, void *data);
#endif

#endif // NGX_HTTP_MRUBY_HANDLER_H
