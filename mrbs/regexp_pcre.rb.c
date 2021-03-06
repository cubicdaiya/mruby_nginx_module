static const char *ngx_http_mruby_regexp_pcre_rb_string = 
"class Regexp\n"
"  attr_reader :options\n"
"  attr_reader :source\n"
"  attr_reader :last_match\n"
"\n"
"  def self.quote(str)\n"
"    self.escape(str)\n"
"  end\n"
"\n"
"  def self.compile(*args)\n"
"    self.new(*args)\n"
"  end\n"
"\n"
"  def self.last_match\n"
"    @last_match\n"
"  end\n"
"\n"
"  def =~(str)\n"
"    return nil unless str\n"
"\n"
"    m = self.match(str)\n"
"    if m\n"
"      m.begin(0)\n"
"    else\n"
"      nil\n"
"    end\n"
"  end\n"
"\n"
"  def ===(str)\n"
"    unless str.is_a? String\n"
"      if str.is_a? Symbol\n"
"        str = str.to_s\n"
"      else\n"
"        return false\n"
"      end\n"
"    end\n"
"    self.match(str) != nil\n"
"  end\n"
"\n"
"  def casefold?\n"
"    (@options & IGNORECASE) > 0\n"
"  end\n"
"\n"
"  def to_s\n"
"    s = \"(?\"\n"
"    s += get_enable_option_string\n"
"\n"
"    if @options & MULTILINE == 0 or @options & IGNORECASE == 0 or @options & EXTENDED == 0\n"
"      s += \"-\"\n"
"      s += get_disable_option_string\n"
"    end\n"
"\n"
"    s += \":\"\n"
"    s += @source\n"
"    s += \")\"\n"
"    s\n"
"  end\n"
"\n"
"  def inspect\n"
"    s = \"/\"\n"
"    s += @source\n"
"    s += \"/\"\n"
"    s += get_enable_option_string\n"
"    s\n"
"  end\n"
"\n"
"  def get_enable_option_string\n"
"    s = \"\"\n"
"    s += \"m\" if @options & MULTILINE > 0\n"
"    s += \"i\" if @options & IGNORECASE > 0\n"
"    s += \"x\" if @options & EXTENDED > 0\n"
"    s\n"
"  end\n"
"\n"
"  def get_disable_option_string\n"
"    s = \"\"\n"
"    s += \"m\" if @options & MULTILINE == 0\n"
"    s += \"i\" if @options & IGNORECASE == 0\n"
"    s += \"x\" if @options & EXTENDED == 0\n"
"    s\n"
"  end\n"
"\n"
"  def self.escape(str)\n"
"    escape_table = {\n"
"      \"\\ \" => '\\\\ ', \n"
"      \"[\"  => '\\\\[', \n"
"      \"]\"  => '\\\\]', \n"
"      \"{\"  => '\\\\{', \n"
"      \"}\"  => '\\\\}', \n"
"      \"(\"  => '\\\\(', \n"
"      \")\"  => '\\\\)', \n"
"      \"|\"  => '\\\\|', \n"
"      \"-\"  => '\\\\-', \n"
"      \"*\"  => '\\\\*', \n"
"      \".\"  => '\\\\.', \n"
"      \"\\\\\" => '\\\\\\\\',\n"
"      \"?\"  => '\\\\?', \n"
"      \"+\"  => '\\\\+', \n"
"      \"^\"  => '\\\\^', \n"
"      \"$\"  => '\\\\$', \n"
"      \"#\"  => '\\\\#', \n"
"      \"\\n\" => '\\\\n', \n"
"      \"\\r\" => '\\\\r', \n"
"      \"\\f\" => '\\\\f', \n"
"      \"\\t\" => '\\\\t', \n"
"      \"\\v\" => '\\\\v', \n"
"    }\n"
"\n"
"    s = \"\"\n"
"    str.each_char do |c|\n"
"      if escape_table[c]\n"
"        s += escape_table[c]\n"
"      else\n"
"        s += c\n"
"      end\n"
"    end\n"
"    s\n"
"  end\n"
"\n"
"  def named_captures\n"
"    h = {}\n"
"    if @names\n"
"      @names.each do |k, v|\n"
"        h[k.to_s] = [v + 1]\n"
"      end\n"
"    end\n"
"    h\n"
"  end\n"
"\n"
"  def names\n"
"    if @names\n"
"      ar = Array.new(@names.size)\n"
"      @names.each do |k, v|\n"
"        ar[v] = k.to_s\n"
"      end\n"
"      ar\n"
"    else\n"
"      []\n"
"    end\n"
"  end\n"
"\n"
"  # private\n"
"  def name_push(name, index)\n"
"    @names ||= {}\n"
"    @names[name.to_sym] = index - 1\n"
"  end\n"
"end\n"
"\n"
"class MatchData\n"
"  attr_reader :regexp\n"
"  attr_reader :string\n"
"\n"
"  def [](n)\n"
"    # XXX: if n is_a? Range\n"
"    # XXX: when we have 2nd argument...\n"
"    if n < 0\n"
"      n += self.length\n"
"      return nil if n < 0\n"
"    elsif n >= self.length\n"
"      return nil\n"
"    end\n"
"    b = self.begin(n)\n"
"    e = self.end(n)\n"
"    if b and e\n"
"      @string[b, e-b]\n"
"    else\n"
"      nil\n"
"    end\n"
"  end\n"
"\n"
"  def captures\n"
"    self.to_a[1, self.length-1]\n"
"  end\n"
"\n"
"  def inspect\n"
"    if self.length == 1\n"
"      '#<MatchData \"#{self[0]}\">'\n"
"    else\n"
"      idx = 0 \n"
"      capts = self.captures.map! { |s| \"#{idx += 1}:#{s.inspect}\" }.join(' ')\n"
"      '#<MatchData \"#{self[0]}\" #{capts}>'\n"
"    end \n"
"  end\n"
"\n"
"  def names\n"
"    self.regexp.names\n"
"  end\n"
"\n"
"  def offset(n)\n"
"    [self.begin(n), self.end(n)]\n"
"  end\n"
"\n"
"  def post_match\n"
"    @string[self.end(0), @string.length]\n"
"  end\n"
"\n"
"  def pre_match\n"
"    @string[0, self.begin(0)]\n"
"  end\n"
"\n"
"  def push(start = nil, finish = nil)\n"
"    @data.push({start: start, finish: finish})\n"
"  end\n"
"\n"
"  def to_a\n"
"    a = []\n"
"    self.length.times { |i| a << self[i] }\n"
"    a\n"
"  end\n"
"\n"
"  def to_s\n"
"    self[0]\n"
"  end\n"
"\n"
"  def values_at(*args)\n"
"    args.map { |i| if i == -self.length then nil else self[i] end }\n"
"  end\n"
"\n"
"  alias size length\n"
"end\n"
;
;
