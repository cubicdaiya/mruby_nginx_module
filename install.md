```sh
cd ${mruby_nginx_module_src_dir}
git submodule update --init
cd mruby
rake ENABLE_GEMS="true" CFLAGS="-O2 -fPIC"
cd {$nginx_src_dir}
./configure --add-module=${mruby_nginx_module_src_dir}
make
make install
```

If you want to use 'mruby_set' and 'mruby_set_code', 
you may embed [ngx_devel_kit](https://github.com/simpl/ngx_devel_kit) when executing configure.

```sh
./configure --add-module=${mruby_nginx_module_src_dir} --add-module=${ngx_devel_kit_src_dir}
```

