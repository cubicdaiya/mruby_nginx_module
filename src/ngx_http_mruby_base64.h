/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *  Copyright (c) ngx_mruby developers 2012-
 *
 *  Use and distribution licensed under the MIT license.
 *  See LICENSE for full text.
 *
 */

#ifndef NGX_HTTP_MRUBY_BASE64_H
#define NGX_HTTP_MRUBY_BASE64_H

#include <mruby.h>

void ngx_mrb_base64_class_init(mrb_state *mrb, struct RClass *calss);

#endif // NGX_HTTP_MRUBY_BASE64_H
