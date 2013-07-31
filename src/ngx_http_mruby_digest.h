/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *  Copyright (c) ngx_mruby developers 2012-
 *
 *  Use and distribution licensed under the MIT license.
 *  See LICENSE for full text.
 *
 */

#ifndef NGX_HTTP_MRUBY_DIGEST_H
#define NGX_HTTP_MRUBY_DIGEST_H

#include <mruby.h>

void ngx_mrb_digest_class_init(mrb_state *mrb, struct RClass *calss);

#endif // NGX_HTTP_MRUBY_DIGEST_H
