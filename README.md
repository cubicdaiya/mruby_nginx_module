## Summary

**mruby_nginx_module** is the powerful extension by mruby for Nginx.

## Concept

The concept of **mruby_nginx_module** is following.

 * Give developers to develop nginx module by mruby
 * Usability that higher affinity with Nginx
 * More features

## Examples

```nginx
location /mruby {
    mruby_content_handler_code "
        sum = 0
        (1..10).each { |i| sum += i }
        Nginx.rputs(sum.to_s + "\n") #=> 55
    ";
}
```

## How to use

Please see [Documents](http://cubicdaiya.github.io/mruby_nginx_module/).

## Dependencies

  - [Nginx](http://nginx.org/)
  - [mruby](https://github.com/mruby/mruby)
  - [ngx_devel_kit](https://github.com/simpl/ngx_devel_kit) (optional)

## Build

```sh
hg clone http://hg.nginx.org/nginx/
https://github.com/cubicdaiya/mruby_nginx_module.git
cd mruby_nginx_module
git submodule update --init
cd mruby
rake ENABLE_GEMS="true" CFLAGS="-O2 -fPIC"
cd ../../nginx
./configure --with-pcre --add-module=../mruby_nginx_module
make
make install
```

If you want to use **mruby_set** and **mruby_set_code**, you may embed [ngx_devel_kit](https://github.com/simpl/ngx_devel_kit) when executing configure with --add-module.


## Difference from ngx_mruby

**mruby_nginx_module** is forked from [ngx_mruby](https://github.com/matsumoto-r/ngx_mruby) at July 2013.


Though I used to develop [ngx_mruby](https://github.com/matsumoto-r/ngx_mruby) with ngx_mruby's developers,
what I want to develop were gradually departured from what they want to develop.

What I want to develop are as previously noted concepts.

 * Give developers by mruby to develop nginx module
 * Usability that higher affinity with Nginx
 * More features

[ngx_mruby](https://github.com/matsumoto-r/ngx_mruby) is a part of unified web server extension developing support institution(like mod_mruby).


But what I want to develop is the product that is more specialized for Nginx. 


To that end **mruby_nginx_module** needs to equip more features than **ngx_mruby** and I must implement drastic mechanism.

In detail, **mruby_nginx_modules** has the following features currently. These are not included in **ngx_mruby**.

 * Regexp
  * built-in regular expression engine
 * Nginx::Context
  * hash table for sharing data in each request processing phase
 * Nginx::Time
  * Nginx time API binding 
 * Nginx::Digest
  * Nginx Digest API binding 
 * Nginx::Base64
  * Nginx Base64(en|de)coding API binding 
 * mruby_require
  * The directive for requiring external mruby script

Additionally I'm going to implement the following features.

 * Nginx::Shared
  * Nginx shared memory API biding
 * Nginx::Subrequest
  * Nginx sub-request API biding
 * Nginx::Socket
  * Nginx non-blocking socket API biding

## How to test

Execute the following command. Note that [echo-nginx-module](https://github.com/agentzh/echo-nginx-module) is requierd.

```sh
rake NGINX_BIN=#{nginx_bin}
```

The default NGINX_BIN is */usr/sbin/nginx*.
