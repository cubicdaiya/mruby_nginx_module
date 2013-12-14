mruby module for nginx [![Build Status](https://travis-ci.org/cubicdaiya/mruby_nginx_module.png?branch=master)](https://travis-ci.org/cubicdaiya/mruby_nginx_module)
======================
## Summary
**mruby_nginx_module** is the powerful extension by mruby for nginx.

## Concept
The concept of **mruby_nginx_module** is following.
 * Give developers to develop nginx module by mruby
 * Usability that higher affinity with nginx

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

Minumum Requre
  - [nginx](http://nginx.org/download/) - you shoukd be able that have all library dependecy to build nginx from source
  - [mruby](https://github.com/mruby/mruby) - ruby requre

Optional
  - [mgem](http://blog.mruby.sh/201301040627.html) - mruby low level gems
  - [ngx_devel_kit](https://github.com/simpl/ngx_devel_kit)
  - [nginx-echo-module](https://github.com/rangechow/nginx-echo-module)

## Build

#### Minumum Requre
**mruby+mruby_nginx_module**
```sh
$ git clone https://github.com/cubicdaiya/mruby_nginx_module.git
$ cd mruby_nginx_module
$ git submodule update --init
$ cd mruby
$ # Optional commands if you want
$ rake ENABLE_GEMS="true" CFLAGS="-O2 -fPIC"
```
**nginx+mruby_nginx_module**
```sh
$ # download and unpack stable nginx version
$ cd nginx-1.4.1 # for eaxmple
$ ./configure -add-module=../mruby_nginx_module
$ make
$ sudo make install
```
#### Optional commands
**mruby+mgem**
```sh
$ gem install mgem
$ mgem update && mgem list 
$ mgem add mruby-userdata # for example
$ mgem active mruby-userdata
$ mgem config # answer the question and then you should add this 
$ # replcae build_config.rb with last text output or make changes by hand, then
$ rake ENABLE_GEMS="true" CFLAGS="-O2 -fPIC" 
```
**+nginx-echo-module+ngx_devel_kit**
```sh
$ # download and unpack stable nginx version
$ cd nginx-1.4.1 # for eaxmple
$ ./configure --with-pcre-jit --add-module=../nginx-echo-module \
--add-module=../mruby_nginx_module --add-module=../ngx_devel_kit
$ make
$ sudo make install
```
## Run test
```sh
$ cd mruby_nginx_module
rake NGINX_BIN=#{nginx_bin}
```
Pass all tests - [echo-nginx-module](https://github.com/agentzh/echo-nginx-module) is requierd.
Make sure where ```nginx``` placed
Default NGINX_BIN is */usr/sbin/nginx*.
Change it if diferent in ```mruby_nginx_module/test/test.rb```

## Difference between ngx_mruby
**mruby_nginx_module** is forked from [ngx_mruby](https://github.com/matsumoto-r/ngx_mruby) at July 2013.

Though I used to develop [ngx_mruby](https://github.com/matsumoto-r/ngx_mruby) with ngx_mruby's developers,
what I want to develop were gradually departured from what they want to develop.

What I want to develop are as previously noted concepts.

 * Give developers by mruby to develop nginx module
 * Usability that higher affinity with nginx

[ngx_mruby](https://github.com/matsumoto-r/ngx_mruby) is a part of unified web server extension developing support institution(like mod_mruby).


But what I want to develop is the product that is more specialized for nginx. 


In detail, **mruby_nginx_modules** has the following features currently. 


 * Regexp
  * built-in regular expression class
 * Nginx::Context
  * hash table for sharing data in each request processing phase
 * Nginx::Time
  * nginx time API binding 
 * Nginx::Digest
  * nginx Digest API binding 
 * Nginx::Base64
  * nginx Base64(en|de)coding API binding 
 * mruby_require
  * The directive for requiring external mruby script

Additionally I'm going to implement the following features.

 * Nginx::Shared
  * nginx shared memory API binding
 * Nginx::Subrequest
  * nginx sub-request API binding
 * Nginx::Socket
  * nginx non-blocking socket API binding
