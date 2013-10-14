/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *  Copyright (c) ngx_mruby developers 2012-
 *
 *  Use and distribution licensed under the MIT license.
 *  See LICENSE for full text.
 *
 */

#include "ngx_http_mruby_class.h"

#include <ngx_http.h>

#include <mruby.h>
#include <mruby/class.h>
#include <mruby/numeric.h>
#include <mruby/string.h>

static mrb_value ngx_http_mruby_update(mrb_state *mrb, mrb_value self);
static mrb_value ngx_http_mruby_time(mrb_state *mrb, mrb_value self);
static mrb_value ngx_http_mruby_http_time(mrb_state *mrb, mrb_value self);
static mrb_value ngx_http_mruby_cookie_time(mrb_state *mrb, mrb_value self);
static mrb_value ngx_http_mruby_utc_time(mrb_state *mrb, mrb_value self);
static mrb_value ngx_http_mruby_local_time(mrb_state *mrb, mrb_value self);
static mrb_value ngx_http_mruby_parse_http_time(mrb_state *mrb, mrb_value self);

static mrb_value ngx_http_mruby_update(mrb_state *mrb, mrb_value self)
{
    ngx_time_update();
    return self;
}

static mrb_value ngx_http_mruby_time(mrb_state *mrb, mrb_value self)
{
    return mrb_fixnum_value(ngx_time());
}

static mrb_value ngx_http_mruby_http_time(mrb_state *mrb, mrb_value self)
{
    mrb_value  mrb_time;
    time_t     time;
    u_char    *p;
    u_char     buf[sizeof("Mon, 04 Aug 2013 01:00:00 GMT") - 1];

    mrb_get_args(mrb, "o", &mrb_time);

    if (mrb_type(mrb_time) != MRB_TT_FIXNUM) {
        mrb_time = mrb_funcall(mrb, mrb_time, "to_i", 0, NULL);
    }

    time = mrb_fixnum(mrb_time);
    p    = buf;
    p    = ngx_http_time(p, time);

    return mrb_str_new(mrb, (char *)buf, p - buf);
}

static mrb_value ngx_http_mruby_cookie_time(mrb_state *mrb, mrb_value self)
{
    mrb_value  mrb_time;
    time_t     time;
    u_char    *p;
    u_char     buf[sizeof("Mon, 04 Aug 2013 01:00:00 GMT") - 1];

    mrb_get_args(mrb, "o", &mrb_time);

    if (mrb_type(mrb_time) != MRB_TT_FIXNUM) {
        mrb_time = mrb_funcall(mrb, mrb_time, "to_i", 0, NULL);
    }

    time = mrb_fixnum(mrb_time);
    p    = buf;
    p    = ngx_http_cookie_time(p, time);

    return mrb_str_new(mrb, (char *)buf, p - buf);
}

static mrb_value ngx_http_mruby_utc_time(mrb_state *mrb, mrb_value self)
{
    ngx_tm_t  tm;
    u_char    buf[sizeof("2013-08-04 01:00:00") - 1];

    ngx_gmtime(ngx_time(), &tm);

    ngx_sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d", 
                    tm.ngx_tm_year, tm.ngx_tm_mon, tm.ngx_tm_mday, 
                    tm.ngx_tm_hour, tm.ngx_tm_min, tm.ngx_tm_sec);

    return mrb_str_new(mrb, (char *)buf, sizeof(buf));
}

static mrb_value ngx_http_mruby_local_time(mrb_state *mrb, mrb_value self)
{
    ngx_tm_t tm;
    u_char   buf[sizeof("2013-08-04 01:00:00") - 1];

    ngx_gmtime(ngx_time() + ngx_cached_time->gmtoff * 60, &tm);

    ngx_sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d", 
                tm.ngx_tm_year, tm.ngx_tm_mon, tm.ngx_tm_mday, 
                tm.ngx_tm_hour, tm.ngx_tm_min, tm.ngx_tm_sec);

    return mrb_str_new(mrb, (char *)buf, sizeof(buf));
}

static mrb_value ngx_http_mruby_parse_http_time(mrb_state *mrb, mrb_value self)
{
    mrb_value mrb_http_time;
    ngx_str_t http_time;

    mrb_get_args(mrb, "o", &mrb_http_time);

    if (mrb_type(mrb_http_time) != MRB_TT_STRING) {
        mrb_http_time = mrb_funcall(mrb, mrb_http_time, "to_s", 0, NULL);
    }

    http_time.data = (u_char *)RSTRING_PTR(mrb_http_time);
    http_time.len  = RSTRING_LEN(mrb_http_time);

    return mrb_fixnum_value(ngx_http_parse_time(http_time.data, http_time.len));
}

void ngx_http_mruby_time_class_init(mrb_state *mrb, struct RClass *class)
{
    struct RClass *class_time;

    class_time = mrb_define_class_under(mrb, class, "Time", mrb->object_class);

    mrb_define_class_method(mrb, class_time, "update",          ngx_http_mruby_update,          ARGS_ANY());
    mrb_define_class_method(mrb, class_time, "time",            ngx_http_mruby_time,            ARGS_ANY());
    mrb_define_class_method(mrb, class_time, "http_time",       ngx_http_mruby_http_time,       ARGS_ANY());
    mrb_define_class_method(mrb, class_time, "cookie_time",     ngx_http_mruby_cookie_time,     ARGS_ANY());
    mrb_define_class_method(mrb, class_time, "utc_time",        ngx_http_mruby_utc_time,        ARGS_ANY());
    mrb_define_class_method(mrb, class_time, "local_time",      ngx_http_mruby_local_time,      ARGS_ANY());
    mrb_define_class_method(mrb, class_time, "parse_http_time", ngx_http_mruby_parse_http_time, ARGS_ANY());
}
