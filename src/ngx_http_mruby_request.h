/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *  Copyright (c) ngx_mruby developers 2012-
 *
 *  Use and distribution licensed under the MIT license.
 *  See LICENSE for full text.
 *
 */

#ifndef NGX_HTTP_MRUBY_REQUEST_H
#define NGX_HTTP_MRUBY_REQUEST_H

#include <ngx_http.h>

#include <mruby.h>

void ngx_mrb_push_request(ngx_http_request_t *r);
ngx_http_request_t *ngx_mrb_get_request(void);
void ngx_mrb_request_class_init(mrb_state *mrb, struct RClass *calss);
mrb_value ngx_mrb_get_request_var(mrb_state *mrb, mrb_value self);

#endif // NGX_HTTP_MRUBY_REQUEST_H
