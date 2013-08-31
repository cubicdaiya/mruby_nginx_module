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

void ngx_mrb_request_class_init(mrb_state *mrb, struct RClass *calss);
void ngx_mrb_variable_class_init(mrb_state *mrb, struct RClass *calss);
void ngx_mrb_context_class_init(mrb_state *mrb, struct RClass *calss);
void ngx_mrb_digest_class_init(mrb_state *mrb, struct RClass *calss);
void ngx_mrb_time_class_init(mrb_state *mrb, struct RClass *calss);
void ngx_mrb_base64_class_init(mrb_state *mrb, struct RClass *calss);

#endif // NGX_HTTP_MRUBY_CLASS_H
