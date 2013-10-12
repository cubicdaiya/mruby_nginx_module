#!/usr/bin/env ruby
# coding: utf-8

def wrap_dquote_string(path)
  content = ""
  File.open(path).each do |l|
    l = l.gsub(/\\/, "\\\\\\\\")
    l = l.gsub(/"/, '\"')
    content  += "\"" + l.chomp + '\n' + "\"\n"
  end
  content
end

def update_inline_regex_mrb(path, vname, content)
  File.open(path + ".c", File::WRONLY) do |f|
    f.write("static const char *#{vname} = \n")
    f.write(content)
    f.write(";\n")
  end
  puts "#{path}.c is generated."
end

pwd      = File.dirname(File.expand_path(__FILE__))
mrbs_dir = File.dirname(pwd) + "/mrbs"
regexp_mrb_path = mrbs_dir + "/regexp_pcre.rb"
string_mrb_path = mrbs_dir + "/string_pcre.rb"

update_inline_regex_mrb(
  regexp_mrb_path, 
  "ngx_mrb_regexp_pcre_rb_string",
  wrap_dquote_string(regexp_mrb_path)
)

update_inline_regex_mrb(
  string_mrb_path, 
  "ngx_mrb_string_pcre_rb_string",
  wrap_dquote_string(string_mrb_path)
)
