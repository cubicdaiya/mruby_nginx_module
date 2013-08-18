## Summary

mruby_nginx_module is the powerful extension by mruby for nginx.
mruby_nginx_module is based [ngx_mruby](https://github.com/matsumoto-r/ngx_mruby).

## Dependencies

  - [Nginx](http://nginx.org/)
  - [mruby](https://github.com/mruby/mruby)
  - [ngx_devel_kit](https://github.com/simpl/ngx_devel_kit) (optional)

## Build

```sh
cd #{mruby_nginx_module_src_dir}
git submodule update --init
cd mruby
rake ENABLE_GEMS="true" CFLAGS="-O2 -fPIC"
cd #{nginx_src_dir}
./configure --add-module=#{mruby_nginx_module_src_dir}
make
make install
```

If you want to use **mruby_set** and **mruby_set_code**, 
you may embed [ngx_devel_kit](https://github.com/simpl/ngx_devel_kit) when executing configure.

```sh
./configure --add-module=#{mruby_nginx_module_src_dir} --add-module=#{ngx_devel_kit_src_dir}
```

## How to use

Please See [Wiki](https://github.com/cubicdaiya/mruby_nginx_module/wiki).

## How to test

Execute the following command. Note that [echo-nginx-module](https://github.com/agentzh/echo-nginx-module) is requierd.

```sh
rake NGINX_BIN=#{nginx_bin}
```
