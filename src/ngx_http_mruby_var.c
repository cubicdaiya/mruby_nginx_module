/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *  Copyright (c) ngx_mruby developers 2012-
 *
 *  Use and distribution licensed under the MIT license.
 *  See LICENSE for full text.
 *
 */

#include "ngx_http_mruby_var.h"

#include <mruby.h>
#include <mruby/string.h>

// 
// Nginx::Var is a getter/sertter for nginx's variables.
// See nginx/src/http/ngx_http_variables.c about nginx's variables
// 

static mrb_value ngx_mrb_var_get(mrb_state *mrb, mrb_value self, const char *c_name);
static mrb_value ngx_mrb_var_method_missing(mrb_state *mrb, mrb_value self);
static mrb_value ngx_mrb_var_set(mrb_state *mrb, mrb_value self);

static mrb_value ngx_mrb_var_get(mrb_state *mrb, mrb_value self, const char *c_name)
{
    ngx_http_request_t        *r;
    ngx_http_variable_value_t *var;
    ngx_str_t                  ngx_name;
    u_char                    *low;
    size_t                     len;
    ngx_uint_t                 key;

    r = ngx_mrb_get_request();

    ngx_name.len  = ngx_strlen(c_name);
    ngx_name.data = (u_char *)c_name;
    len           = ngx_name.len;

    if (len) {
        low = ngx_pnalloc(r->pool, len);
        if (low == NULL) {
            return mrb_nil_value();
        }
    } else {
        return mrb_nil_value();
    }

    key = ngx_hash_strlow(low, ngx_name.data, len);
    var = ngx_http_get_variable(r, &ngx_name, key);

    if (var->not_found) {
        return mrb_nil_value();
    }

    return mrb_str_new(mrb, (char *)var->data, var->len);
}

static mrb_value ngx_mrb_var_method_missing(mrb_state *mrb, mrb_value self)
{
    mrb_value  name, a;
    int        alen;
    mrb_value  s_name;
    char      *c_name;

    mrb_get_args(mrb, "n*", &name, &a, &alen);

    s_name = mrb_sym2str(mrb, mrb_symbol(name));
    c_name = mrb_str_to_cstr(mrb, s_name);

    return ngx_mrb_var_get(mrb, self, c_name);
}

static mrb_value ngx_mrb_var_set(mrb_state *mrb, mrb_value self)
{
    ngx_http_request_t        *r;
    ngx_http_variable_t       *v;
    ngx_http_variable_value_t *vv;
    ngx_http_core_main_conf_t *cmcf;
    ngx_str_t                  key;
    ngx_uint_t                 hash;
    u_char                    *val, *low;
    char                      *k;
    mrb_value                  o;

    r = ngx_mrb_get_request();

    mrb_get_args(mrb, "zo", &k, &o);
    if (mrb_type(o) != MRB_TT_STRING) {
        o = mrb_funcall(mrb, o, "to_s", 0, NULL);
    }
    val      = (u_char *)RSTRING_PTR(o);
    key.data = (u_char *)k;
    key.len  = ngx_strlen(k);
    if (key.len) {
        low = ngx_pnalloc(r->pool, key.len);
        if (low == NULL) {
            return mrb_nil_value();
        }
    } else {
        return mrb_nil_value();
    }
    hash = ngx_hash_strlow(low, key.data, key.len);
    cmcf = ngx_http_get_module_main_conf(r, ngx_http_core_module);
    v    = ngx_hash_find(&cmcf->variables_hash, hash, key.data, key.len);
    if (v) {
        if (!(v->flags & NGX_HTTP_VAR_CHANGEABLE)) {
            ngx_log_error(NGX_LOG_ERR
                , r->connection->log
                , 0
                , "%s ERROR %s:%d: %s not changeable"
                , MODULE_NAME
                , __FUNCTION__
                , __LINE__
                , key.data
            );
            return mrb_nil_value();
        }
        if (v->set_handler) {
            vv = ngx_palloc(r->pool, sizeof(ngx_http_variable_value_t));
            if (vv == NULL) {
                ngx_log_error(NGX_LOG_ERR
                    , r->connection->log
                    , 0
                    , "%s ERROR :%d: memory allocate failed"
                    , MODULE_NAME
                    , __FUNCTION__
                    , __LINE__
                );
                return mrb_nil_value();
            }
            vv->valid        = 1;
            vv->not_found    = 0;
            vv->no_cacheable = 0;
            vv->data         = val;
            vv->len          = ngx_strlen(val);

            v->set_handler(r, vv, v->data);

            return mrb_str_new_cstr(mrb, (char *)val);
        }
        if (v->flags & NGX_HTTP_VAR_INDEXED) {
            vv = &r->variables[v->index];

            vv->valid        = 1;
            vv->not_found    = 0;
            vv->no_cacheable = 0;
            vv->data         = val;
            vv->len          = ngx_strlen(val);

            return mrb_str_new_cstr(mrb, (char *)val);
        }
        ngx_log_error(NGX_LOG_ERR
            , r->connection->log
            , 0
            , "%s ERROR %s:%d: %s is not assinged"
            , MODULE_NAME
            , __FUNCTION__
            , __LINE__
            , key.data
        );
        return mrb_nil_value();
    }

    ngx_log_error(NGX_LOG_ERR
        , r->connection->log
        , 0
        , "%s ERROR %s:%d: %s is not found"
        , MODULE_NAME
        , __FUNCTION__
        , __LINE__
        , key.data
    );
    return mrb_nil_value();
}

void ngx_mrb_var_class_init(mrb_state *mrb, struct RClass *class)
{
    struct RClass *class_var;

    class_var = mrb_define_class_under(mrb, class, "Var", mrb->object_class);

    mrb_define_method(mrb, class_var, "method_missing", ngx_mrb_var_method_missing, ARGS_ANY());
    mrb_define_method(mrb, class_var, "set",            ngx_mrb_var_set, ARGS_REQ(2));
}
