/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *  Copyright (c) 2013 Internet Initiative Japan Inc.
 *
 *  Use and distribution licensed under the MIT license.
 *  See LICENSE for full text.
 *
 */

#include "ngx_http_mruby_module.h"
#include "ngx_http_mruby_class.h"

#include <mruby.h>
#include <mruby/proc.h>
#include <mruby/data.h>
#include <mruby/string.h>
#include <mruby/variable.h>
#include <mruby/class.h>

#define MRUBY_REGEXP_IGNORECASE         0x01
#define MRUBY_REGEXP_EXTENDED           0x02
#define MRUBY_REGEXP_MULTILINE          0x04

static ngx_pool_t *ngx_mrb_pcre_pool = NULL;

static void *(*old_pcre_malloc)(size_t);
static void (*old_pcre_free)(void *ptr);

static void *ngx_mrb_pcre_malloc(size_t size)
{
    if (ngx_mrb_pcre_pool) {
        return ngx_palloc(ngx_mrb_pcre_pool, size);
    }

    return NULL;
}

static void ngx_mrb_pcre_free(void *ptr)
{
    if (ngx_mrb_pcre_pool) {
        ngx_pfree(ngx_mrb_pcre_pool, ptr);
        return;
    }
}

//
// following original functions are brought from 
// mruby-regex-pcre(https://github.com/iij/mruby-regexp-pcre) and customed
//

struct ngx_mrb_regexp_pcre {
    pcre *re;
};

struct ngx_mrb_matchdata {
    mrb_int length;
    int *ovector;
};

static void ngx_mrb_regexp_free(mrb_state *mrb, void *ptr)
{
    struct ngx_mrb_regexp_pcre *mrb_re = ptr;

    if (mrb_re != NULL) {
    if (mrb_re->re != NULL) {
        pcre_free(mrb_re->re);
    }
    mrb_free(mrb, mrb_re);
  }
}

static void ngx_mrb_matchdata_free(mrb_state *mrb, void *ptr)
{
    struct ngx_mrb_matchdata *mrb_md = ptr;

    if (mrb_md != NULL) {
        if (mrb_md->ovector != NULL) {
            mrb_free(mrb, mrb_md->ovector);
        }
        mrb_free(mrb, mrb_md);
    }
}

struct mrb_data_type ngx_mrb_regexp_type    = { "Regexp",    ngx_mrb_regexp_free    };
struct mrb_data_type ngx_mrb_matchdata_type = { "MatchData", ngx_mrb_matchdata_free };

static int ngx_mrb_mruby_to_pcre_options(mrb_value options)
{
    int coptions = PCRE_DOTALL;

    if (mrb_nil_p(options)) {
        coptions = 0;
    } else if (mrb_fixnum_p(options)) {
        int nopt;
        nopt = mrb_fixnum(options);
        if (nopt & MRUBY_REGEXP_IGNORECASE) coptions |= PCRE_CASELESS;
        if (nopt & MRUBY_REGEXP_EXTENDED)   coptions |= PCRE_EXTENDED;
        if (nopt & MRUBY_REGEXP_MULTILINE)  coptions |= PCRE_MULTILINE;
    } else if (mrb_string_p(options)) {
        if (strchr(RSTRING_PTR(options), 'i')) coptions |= PCRE_CASELESS;
        if (strchr(RSTRING_PTR(options), 'x')) coptions |= PCRE_EXTENDED;
        if (strchr(RSTRING_PTR(options), 'm')) coptions |= PCRE_MULTILINE;
    } else if (mrb_type(options) == MRB_TT_TRUE) {
        coptions |= PCRE_CASELESS;
    }

    return coptions;
}

static int ngx_mrb_pcre_to_mruby_options(int coptions)
{
    int options = 0;

    if (coptions & PCRE_CASELESS) {
        options |= MRUBY_REGEXP_IGNORECASE;
    }

    if (coptions & PCRE_EXTENDED) {
        options |= MRUBY_REGEXP_EXTENDED;
    }

    if (coptions & PCRE_MULTILINE) {
        options |= MRUBY_REGEXP_MULTILINE;
    }

    return options;
}

static mrb_value ngx_mrb_regexp_pcre_initialize(mrb_state *mrb, mrb_value self)
{
    ngx_http_request_t   *r;
    int erroff, coptions;
    const char *errstr = NULL;
    struct ngx_mrb_regexp_pcre *reg = NULL;
    mrb_value source, opt = mrb_nil_value();

    r   = ngx_mrb_get_request();
    reg = (struct ngx_mrb_regexp_pcre *)DATA_PTR(self);
    if (reg) {
        ngx_mrb_regexp_free(mrb, reg);
    }
    DATA_TYPE(self) = &ngx_mrb_regexp_type;
    DATA_PTR(self) = NULL;

    mrb_get_args(mrb, "S|o", &source, &opt);

    reg = mrb_malloc(mrb, sizeof(struct ngx_mrb_regexp_pcre));
    reg->re = NULL;
    DATA_PTR(self) = reg;

    coptions = ngx_mrb_mruby_to_pcre_options(opt);
    source   = mrb_str_new(mrb, RSTRING_PTR(source), RSTRING_LEN(source));

    // As nginx orverrides pcre_(malloc|gree), 
    // calling pcre_compile directly fails
    // This is the workaround for it.
    ngx_mrb_pcre_pool = r->pool;
    old_pcre_malloc   = pcre_malloc;
    old_pcre_free     = pcre_free;
    pcre_malloc       = ngx_mrb_pcre_malloc;
    pcre_free         = ngx_mrb_pcre_free;

    reg->re  = pcre_compile((const char *)RSTRING_PTR(source), coptions, &errstr, &erroff, NULL);

    pcre_malloc = old_pcre_malloc;
    pcre_free   = old_pcre_free;

    if (reg->re == NULL) {
        mrb_raisef(mrb, E_ARGUMENT_ERROR, "invalid regular expression");
    }
    mrb_iv_set(mrb, self, mrb_intern(mrb, "@source"), source);
    mrb_iv_set(mrb, self, mrb_intern(mrb, "@options"), mrb_fixnum_value(ngx_mrb_pcre_to_mruby_options(coptions)));

    unsigned char *name_table;
    int i, namecount, name_entry_size;

    pcre_fullinfo(reg->re, NULL, PCRE_INFO_NAMECOUNT, &namecount);
    if (namecount > 0) {
        pcre_fullinfo(reg->re, NULL, PCRE_INFO_NAMETABLE, &name_table);
        pcre_fullinfo(reg->re, NULL, PCRE_INFO_NAMEENTRYSIZE, &name_entry_size);
        unsigned char *tabptr = name_table;
        for (i = 0; i < namecount; i++) {
            int n = (tabptr[0] << 8) | tabptr[1];
            mrb_funcall(mrb, self, "name_push", 2, mrb_str_new(mrb, (const char *)(tabptr + 2), strlen((const char *)tabptr + 2)), mrb_fixnum_value(n));
            tabptr += name_entry_size;
        }
    }

    return self;
}

static mrb_value ngx_mrb_regexp_pcre_match(mrb_state *mrb, mrb_value self)
{
    struct ngx_mrb_matchdata *mrb_md;
    int rc;
    int ccount, matchlen;
    int *match;
    struct RClass *c;
    mrb_value md, str;
    mrb_int i, pos;
    pcre_extra extra;
    struct ngx_mrb_regexp_pcre *reg;

    reg = (struct ngx_mrb_regexp_pcre *)mrb_get_datatype(mrb, self, &ngx_mrb_regexp_type);
    if (!reg) {
        return mrb_nil_value();
    }

    pos = 0;
    mrb_get_args(mrb, "S|i", &str, &pos);

    // XXX: RSTRING_LEN(str) >= pos ...

    rc = pcre_fullinfo(reg->re, NULL, PCRE_INFO_CAPTURECOUNT, &ccount);
    if (rc < 0) {
        /* fullinfo error */
        return mrb_nil_value();
    }
    matchlen = ccount + 1;
    match = mrb_malloc(mrb, sizeof(int) * matchlen * 3);

    extra.flags = PCRE_EXTRA_MATCH_LIMIT_RECURSION;
    extra.match_limit_recursion = 1000;
    rc = pcre_exec(reg->re, &extra, RSTRING_PTR(str), RSTRING_LEN(str), pos, 0, match, matchlen * 3);
    if (rc < 0) {
        mrb_free(mrb, match);
        return mrb_nil_value();
    }

    /* XXX: need current scope */
    mrb_obj_iv_set(mrb, (struct RObject *)mrb_class_real(RDATA(self)->c), mrb_intern(mrb, "@last_match"), mrb_nil_value());

    c = mrb_class_get(mrb, "MatchData");
    md = mrb_funcall(mrb, mrb_obj_value(c), "new", 0);

    mrb_md = (struct ngx_mrb_matchdata *)mrb_get_datatype(mrb, md, &ngx_mrb_matchdata_type);
    mrb_md->ovector = match;
    mrb_md->length = matchlen;

    mrb_iv_set(mrb, md, mrb_intern(mrb, "@regexp"), self);
    mrb_iv_set(mrb, md, mrb_intern(mrb, "@string"), mrb_str_dup(mrb, str));
    /* XXX: need current scope */
    mrb_obj_iv_set(mrb, (struct RObject *)mrb_class_real(RDATA(self)->c), mrb_intern(mrb, "@last_match"), md);

    mrb_gv_set(mrb, mrb_intern(mrb, "$~"), md);
    mrb_gv_set(mrb, mrb_intern(mrb, "$&"), mrb_funcall(mrb, md, "to_s", 0));
    mrb_gv_set(mrb, mrb_intern(mrb, "$`"), mrb_funcall(mrb, md, "pre_match", 0));
    mrb_gv_set(mrb, mrb_intern(mrb, "$'"), mrb_funcall(mrb, md, "post_match", 0));

    for (i = 1; i < 10; i++) {
        char sym[8];
        snprintf(sym, sizeof(sym), "$%d", i);
        mrb_gv_set(mrb, mrb_intern(mrb, sym), mrb_funcall(mrb, md, "[]", 1, mrb_fixnum_value(i)));
    }

    return md;
}

static mrb_value ngx_mrb_regexp_equal(mrb_state *mrb, mrb_value self)
{
    mrb_value other;
    struct ngx_mrb_regexp_pcre *self_reg, *other_reg;

    mrb_get_args(mrb, "o", &other);
    if (mrb_obj_equal(mrb, self, other)) {
        return mrb_true_value();
    }

    if (mrb_type(other) != MRB_TT_DATA || DATA_TYPE(other) != &ngx_mrb_regexp_type) {
        return mrb_false_value();
    }

    self_reg  = (struct ngx_mrb_regexp_pcre *)DATA_PTR(self);
    other_reg = (struct ngx_mrb_regexp_pcre *)DATA_PTR(other);
    if (!self_reg || !other_reg) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "Invalid Regexp");
    }

    if (mrb_str_equal(mrb, mrb_iv_get(mrb, self, mrb_intern(mrb, "@source")),
                      mrb_iv_get(mrb, other, mrb_intern(mrb, "@source")))) {
        return mrb_true_value();
    }

    return mrb_false_value();
}

static mrb_value ngx_mrb_matchdata_init(mrb_state *mrb, mrb_value self)
{
    struct ngx_mrb_matchdata *mrb_md;

    mrb_md = (struct ngx_mrb_matchdata *)DATA_PTR(self);
    if (mrb_md) {
        ngx_mrb_matchdata_free(mrb, mrb_md);
    }
    DATA_TYPE(self) = &ngx_mrb_matchdata_type;
    DATA_PTR(self) = NULL;

    mrb_md = (struct ngx_mrb_matchdata *)mrb_malloc(mrb, sizeof(*mrb_md));
    mrb_md->ovector = NULL;
    mrb_md->length = -1;
    DATA_PTR(self) = mrb_md;

    return self;
}

static mrb_value ngx_mrb_matchdata_init_copy(mrb_state *mrb, mrb_value copy)
{
    mrb_value src;
    struct ngx_mrb_matchdata *mrb_md_copy, *mrb_md_src;
    int vecsize;

    mrb_get_args(mrb, "o", &src);

    if (mrb_obj_equal(mrb, copy, src)) return copy;
    if (!mrb_obj_is_instance_of(mrb, src, mrb_obj_class(mrb, copy))) {
        mrb_raise(mrb, E_TYPE_ERROR, "wrong argument class");
    }

    mrb_md_copy = (struct ngx_mrb_matchdata *)mrb_malloc(mrb, sizeof(*mrb_md_copy));
    mrb_md_src  = DATA_PTR(src);

    if (mrb_md_src->ovector == NULL) {
        mrb_md_copy->ovector = NULL;
        mrb_md_copy->length = -1;
    } else {
        vecsize = sizeof(int) * mrb_md_src->length * 3;
        mrb_md_copy->ovector = mrb_malloc(mrb, vecsize);
        memcpy(mrb_md_copy->ovector, mrb_md_src->ovector, vecsize);
        mrb_md_copy->length = mrb_md_src->length;
    }

    if (DATA_PTR(copy) != NULL) {
        ngx_mrb_matchdata_free(mrb, DATA_PTR(copy));
    }
    DATA_PTR(copy) = mrb_md_copy;

    mrb_iv_set(mrb, copy, mrb_intern(mrb, "@regexp"), mrb_iv_get(mrb, src, mrb_intern(mrb, "@regexp")));
    mrb_iv_set(mrb, copy, mrb_intern(mrb, "@string"), mrb_iv_get(mrb, src, mrb_intern(mrb, "@string")));
    
    return copy;
}

static mrb_value ngx_mrb_matchdata_beginend(mrb_state *mrb, mrb_value self, int beginend)
{
    struct ngx_mrb_matchdata *mrb_md;
    mrb_int i, offs;

    mrb_md = (struct ngx_mrb_matchdata *)mrb_get_datatype(mrb, self, &ngx_mrb_matchdata_type);
    if (!mrb_md) return mrb_nil_value();

    mrb_get_args(mrb, "i", &i);
    if (i < 0 || i >= mrb_md->length) {
        mrb_raisef(mrb, E_INDEX_ERROR, "index %d out of matches", i);
    }

    offs = mrb_md->ovector[i*2 + beginend];
    if (offs != -1) {
        return mrb_fixnum_value(offs);
    } else {
        return mrb_nil_value();
    }
}

static mrb_value ngx_mrb_matchdata_begin(mrb_state *mrb, mrb_value self)
{
    return ngx_mrb_matchdata_beginend(mrb, self, 0);
}

static mrb_value ngx_mrb_matchdata_end(mrb_state *mrb, mrb_value self)
{
    return ngx_mrb_matchdata_beginend(mrb, self, 1);
}

static mrb_value ngx_mrb_matchdata_length(mrb_state *mrb, mrb_value self)
{
    struct ngx_mrb_matchdata *mrb_md;

    mrb_md = (struct ngx_mrb_matchdata *)mrb_get_datatype(mrb, self, &ngx_mrb_matchdata_type);
    if (!mrb_md) {
        return mrb_nil_value();
    }

    return mrb_fixnum_value(mrb_md->length);
}

void ngx_mrb_regex_class_init(mrb_state *mrb)
{
    struct RClass *class_re, *class_md;

    class_re = mrb_define_class(mrb, "Regexp", mrb->object_class);
    MRB_SET_INSTANCE_TT(class_re, MRB_TT_DATA);

    mrb_define_method(mrb, class_re, "initialize", ngx_mrb_regexp_pcre_initialize, ARGS_REQ(1) | ARGS_OPT(2));
    mrb_define_method(mrb, class_re, "match",      ngx_mrb_regexp_pcre_match,      ARGS_REQ(1));
    mrb_define_method(mrb, class_re, "==",         ngx_mrb_regexp_equal,           ARGS_REQ(1));

    mrb_define_const(mrb, class_re, "IGNORECASE", mrb_fixnum_value(MRUBY_REGEXP_IGNORECASE));
    mrb_define_const(mrb, class_re, "EXTENDED",   mrb_fixnum_value(MRUBY_REGEXP_EXTENDED));
    mrb_define_const(mrb, class_re, "MULTILINE",  mrb_fixnum_value(MRUBY_REGEXP_MULTILINE));

    class_md = mrb_define_class(mrb, "MatchData", mrb->object_class);
    MRB_SET_INSTANCE_TT(class_md, MRB_TT_DATA);

    mrb_define_method(mrb, class_md, "initialize",      ngx_mrb_matchdata_init,      ARGS_REQ(1));
    mrb_define_method(mrb, class_md, "initialize_copy", ngx_mrb_matchdata_init_copy, ARGS_REQ(1));
    mrb_define_method(mrb, class_md, "begin",           ngx_mrb_matchdata_begin,     ARGS_REQ(1));
    mrb_define_method(mrb, class_md, "end",             ngx_mrb_matchdata_end,       ARGS_REQ(1));
    mrb_define_method(mrb, class_md, "length",          ngx_mrb_matchdata_length,    ARGS_NONE());
}
