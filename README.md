## Summary

mruby_nginx_module is the powerful extension by mruby for nginx.
mruby_nginx_module is based [ngx_mruby](https://github.com/matsumoto-r/ngx_mruby).

## Dependencies

  - [Nginx](http://nginx.org/)
  - [mruby](https://github.com/mruby/mruby)

## Build

    cd ${mruby_nginx_module_src_dir}
    git submodule update --init
    cd mruby
    rake ENABLE_GEMS="true" CFLAGS="-O2 -fPIC"
    cd {$nginx_src_dir}
    ./configure --add-module=${mruby_nginx_module_src_dir}
    make
    make install

## How to

Comming soon...
