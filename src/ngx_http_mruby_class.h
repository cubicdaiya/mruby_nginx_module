/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *  Copyright (c) ngx_mruby developers 2012-
 *
 *  Use and distribution licensed under the MIT license.
 *  See LICENSE for full text.
 *
 */

#ifndef NGX_HTTP_MRUBY_CLASS_H
#define NGX_HTTP_MRUBY_CLASS_H

#include <mruby.h>

void ngx_http_mruby_request_class_init(mrb_state *mrb, struct RClass *calss);
void ngx_http_mruby_variable_class_init(mrb_state *mrb, struct RClass *calss);
void ngx_http_mruby_context_class_init(mrb_state *mrb, struct RClass *calss);
void ngx_http_mruby_digest_class_init(mrb_state *mrb, struct RClass *calss);
void ngx_http_mruby_time_class_init(mrb_state *mrb, struct RClass *calss);
void ngx_http_mruby_base64_class_init(mrb_state *mrb, struct RClass *calss);
void ngx_http_mruby_regex_class_init(mrb_state *mrb);

#endif // NGX_HTTP_MRUBY_CLASS_H
