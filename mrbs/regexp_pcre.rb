class Regexp
  attr_reader :options
  attr_reader :source
  attr_reader :last_match

  def self.quote(str)
    self.escape(str)
  end

  def self.compile(*args)
    self.new(*args)
  end

  def self.last_match
    @last_match
  end

  def =~(str)
    return nil unless str

    m = self.match(str)
    if m
      m.begin(0)
    else
      nil
    end
  end

  def ===(str)
    unless str.is_a? String
      if str.is_a? Symbol
        str = str.to_s
      else
        return false
      end
    end
    self.match(str) != nil
  end

  def casefold?
    (@options & IGNORECASE) > 0
  end

  def to_s
    s = "(?"
    s += get_enable_option_string

    if @options & MULTILINE == 0 or @options & IGNORECASE == 0 or @options & EXTENDED == 0
      s += "-"
      s += get_disable_option_string
    end

    s += ":"
    s += @source
    s += ")"
    s
  end

  def inspect
    s = "/"
    s += @source
    s += "/"
    s += get_enable_option_string
    s
  end

  def get_enable_option_string
    s = ""
    s += "m" if @options & MULTILINE > 0
    s += "i" if @options & IGNORECASE > 0
    s += "x" if @options & EXTENDED > 0
    s
  end

  def get_disable_option_string
    s = ""
    s += "m" if @options & MULTILINE == 0
    s += "i" if @options & IGNORECASE == 0
    s += "x" if @options & EXTENDED == 0
    s
  end

  def self.escape(str)
    escape_table = {
      "\ " => '\\ ', 
      "["  => '\\[', 
      "]"  => '\\]', 
      "{"  => '\\{', 
      "}"  => '\\}', 
      "("  => '\\(', 
      ")"  => '\\)', 
      "|"  => '\\|', 
      "-"  => '\\-', 
      "*"  => '\\*', 
      "."  => '\\.', 
      "\\" => '\\\\',
      "?"  => '\\?', 
      "+"  => '\\+', 
      "^"  => '\\^', 
      "$"  => '\\$', 
      "#"  => '\\#', 
      "\n" => '\\n', 
      "\r" => '\\r', 
      "\f" => '\\f', 
      "\t" => '\\t', 
      "\v" => '\\v', 
    }

    s = ""
    str.each_char do |c|
      if escape_table[c]
        s += escape_table[c]
      else
        s += c
      end
    end
    s
  end

  def named_captures
    h = {}
    if @names
      @names.each do |k, v|
        h[k.to_s] = [v + 1]
      end
    end
    h
  end

  def names
    if @names
      ar = Array.new(@names.size)
      @names.each do |k, v|
        ar[v] = k.to_s
      end
      ar
    else
      []
    end
  end

  # private
  def name_push(name, index)
    @names ||= {}
    @names[name.to_sym] = index - 1
  end
end

class MatchData
  attr_reader :regexp
  attr_reader :string

  def [](n)
    # XXX: if n is_a? Range
    # XXX: when we have 2nd argument...
    if n < 0
      n += self.length
      return nil if n < 0
    elsif n >= self.length
      return nil
    end
    b = self.begin(n)
    e = self.end(n)
    if b and e
      @string[b, e-b]
    else
      nil
    end
  end

  def captures
    self.to_a[1, self.length-1]
  end

  def inspect
    if self.length == 1
      '#<MatchData "#{self[0]}">'
    else
      idx = 0 
      capts = self.captures.map! { |s| "#{idx += 1}:#{s.inspect}" }.join(' ')
      '#<MatchData "#{self[0]}" #{capts}>'
    end 
  end

  def names
    self.regexp.names
  end

  def offset(n)
    [self.begin(n), self.end(n)]
  end

  def post_match
    @string[self.end(0), @string.length]
  end

  def pre_match
    @string[0, self.begin(0)]
  end

  def push(start = nil, finish = nil)
    @data.push({start: start, finish: finish})
  end

  def to_a
    a = []
    self.length.times { |i| a << self[i] }
    a
  end

  def to_s
    self[0]
  end

  def values_at(*args)
    args.map { |i| if i == -self.length then nil else self[i] end }
  end

  alias size length
end
