/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *  Copyright (c) ngx_mruby developers 2012-
 *
 *  Use and distribution licensed under the MIT license.
 *  See LICENSE for full text.
 *
 */

#include "ngx_http_mruby_hash.h"

#include <nginx.h>
#include <ngx_config.h>
#include <ngx_http.h>

#include <mruby.h>
#include <mruby/class.h>
#include <mruby/string.h>


#if (NGX_OPENSSL)
#include <openssl/evp.h>
#include <openssl/hmac.h>
#endif

#include <ngx_md5.h>

#if NGX_HAVE_SHA1
#include <ngx_sha1.h>
#endif

#ifndef SHA_DIGEST_LENGTH
#define SHA_DIGEST_LENGTH 20
#endif


static mrb_value ngx_mrb_md5(mrb_state *mrb, mrb_value self);
#if (NGX_HAVE_SHA1)
static mrb_value ngx_mrb_sha1(mrb_state *mrb, mrb_value self);
#endif
#if (NGX_OPENSSL)
static mrb_value ngx_mrb_hmac_sha1(mrb_state *mrb, mrb_value self);
#endif
static mrb_value ngx_mrb_hexdigest(mrb_state *mrb, mrb_value self);

static mrb_value ngx_mrb_md5(mrb_state *mrb, mrb_value self)
{
    mrb_value  mrb_src;
    ngx_str_t  src;
    ngx_md5_t  md5;
    u_char     md5_buf[MD5_DIGEST_LENGTH];

    mrb_get_args(mrb, "o", &mrb_src);

    if (mrb_type(mrb_src) != MRB_TT_STRING) {
        mrb_src = mrb_funcall(mrb, mrb_src, "to_s", 0, NULL);
    }

    src.data = (u_char *)RSTRING_PTR(mrb_src);
    src.len  = RSTRING_LEN(mrb_src);

    ngx_md5_init(&md5);
    ngx_md5_update(&md5, src.data, src.len);
    ngx_md5_final(md5_buf, &md5);

    return mrb_str_new(mrb, (char *)md5_buf, sizeof(md5_buf));
}

#if (NGX_HAVE_SHA1)
static mrb_value ngx_mrb_sha1(mrb_state *mrb, mrb_value self)
{
    mrb_value  mrb_src;
    ngx_str_t  src;
    ngx_sha1_t sha;
    u_char     sha_buf[SHA_DIGEST_LENGTH];

    mrb_get_args(mrb, "o", &mrb_src);

    if (mrb_type(mrb_src) != MRB_TT_STRING) {
        mrb_src = mrb_funcall(mrb, mrb_src, "to_s", 0, NULL);
    }

    src.data = (u_char *)RSTRING_PTR(mrb_src);
    src.len  = RSTRING_LEN(mrb_src);

    ngx_sha1_init(&sha);
    ngx_sha1_update(&sha, src.data, src.len);
    ngx_sha1_final(sha_buf, &sha);

    return mrb_str_new(mrb, (char *)sha_buf, sizeof(sha_buf));
}
#endif

#if (NGX_OPENSSL)
static mrb_value ngx_mrb_hmac_sha1(mrb_state *mrb, mrb_value self)
{
    mrb_value mrb_key, mrb_val;
    ngx_str_t key, text;
    u_char md[EVP_MAX_MD_SIZE];
    unsigned int md_len;
    const EVP_MD *evp_md;

    mrb_get_args(mrb, "oo", &mrb_key, &mrb_val);

    if (mrb_type(mrb_key) != MRB_TT_STRING) {
        mrb_key = mrb_funcall(mrb, mrb_key, "to_s", 0, NULL);
    }

    if (mrb_type(mrb_val) != MRB_TT_STRING) {
        mrb_val = mrb_funcall(mrb, mrb_val, "to_s", 0, NULL);
    }

    key.data  = (u_char *)RSTRING_PTR(mrb_key);
    key.len   = RSTRING_LEN(mrb_key);
    text.data = (u_char *)RSTRING_PTR(mrb_val);
    text.len  = RSTRING_LEN(mrb_val);

    evp_md = EVP_sha1();
    HMAC(evp_md, key.data, key.len, text.data, text.len, md, &md_len);

    return mrb_str_new(mrb, (char *)md, md_len);
}
#endif

static mrb_value ngx_mrb_hexdigest(mrb_state *mrb, mrb_value self)
{
    mrb_value mrb_digest, mrb_hexdigest;
    ngx_str_t digest;
    u_char    hexdigest[40 + 1];

    mrb_get_args(mrb, "o", &mrb_digest);

    if (mrb_type(mrb_digest) != MRB_TT_STRING) {
        mrb_digest = mrb_funcall(mrb, mrb_digest, "to_s", 0, NULL);
    }

    digest.data = (u_char *)RSTRING_PTR(mrb_digest);
    digest.len  = RSTRING_LEN(mrb_digest);

    ngx_memzero(hexdigest, 40 + 1);

    switch (digest.len) {
    case MD5_DIGEST_LENGTH:
        ngx_hex_dump(hexdigest, digest.data, MD5_DIGEST_LENGTH);
        mrb_hexdigest = mrb_str_new(mrb, (char *)hexdigest, 32);
        break;
    case SHA_DIGEST_LENGTH:
        ngx_hex_dump(hexdigest, digest.data, SHA_DIGEST_LENGTH);
        mrb_hexdigest = mrb_str_new(mrb, (char *)hexdigest, 40);
        break;
    default:
        return mrb_nil_value();
        break;
    }

    return mrb_hexdigest;
}

void ngx_mrb_hash_class_init(mrb_state *mrb, struct RClass *class)
{
    struct RClass *class_hash;

    class_hash = mrb_define_class_under(mrb, class, "Hash", mrb->object_class);

    mrb_define_class_method(mrb, class_hash, "md5",       ngx_mrb_md5,       ARGS_ANY());
#if NGX_HAVE_SHA1
    mrb_define_class_method(mrb, class_hash, "sha1",      ngx_mrb_sha1,      ARGS_ANY());
#endif
#if (NGX_OPENSSL)
    mrb_define_class_method(mrb, class_hash, "hmac_sha1", ngx_mrb_hmac_sha1, ARGS_ANY());
#endif
    mrb_define_class_method(mrb, class_hash, "hexdigest", ngx_mrb_hexdigest, ARGS_ANY());
}