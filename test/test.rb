require "test/unit"
require "net/empty_port"
require "net/http"

class MrubyNginxModuleTest < Test::Unit::TestCase
    def setup
      @dir = File.dirname(File.expand_path(__FILE__))
      nginx_bin     = ENV['NGINX_BIN']
      nginx_options = " -p #{@dir}/ngx_base/ -c #{@dir}/ngx_base/etc/nginx.conf"
      `sudo #{nginx_bin} #{nginx_options}`
      Net::EmptyPort.wait(8000, 10)
    end

    def test_hello
      url = URI.parse('http://localhost:8000')
      res = Net::HTTP.start(url.host, url.port) {|http|
        http.get('/hello')
      }
      assert_equal(res["content-type"], "text/plain")
      assert_equal(res.body, "Hello, World!")
    end

    def test_redirect
      url = URI.parse('http://localhost:8000')
      res = Net::HTTP.start(url.host, url.port) {|http|
        http.get('/redirect')
      }
      assert_equal(res.code, "301")
    end

    def test_redirect_internal
      url = URI.parse('http://localhost:8000')
      res = Net::HTTP.start(url.host, url.port) {|http|
        http.get('/redirect_internal')
      }
      assert_equal(res.code, "200")
      assert_equal(res["content-type"], "text/plain")
      assert_equal(res.body, "Hello, World!")
    end

    def test_ctx
      url = URI.parse('http://localhost:8000')
      res = Net::HTTP.start(url.host, url.port) {|http|
        http.get('/ctx')
      }
      assert_equal(res.body, "3")
    end

    def test_acl_ok
      # This test is available until 2018/08/04 for expires
      url = URI.parse('http://localhost:8000')
      res = Net::HTTP.start(url.host, url.port) {|http|
        http.get('/acl.html?accesskey=bokko&expires=1533240048&signature=cDcAm6xNVlsNwuX3CP3KGg/pIPU=')
      }
      assert_equal(res.code, "200")
    end

    def test_acl_ng
      url = URI.parse('http://localhost:8000')
      res = Net::HTTP.start(url.host, url.port) {|http|
        http.get('/acl.html?accesskey=bokko&expires=1375558147&signature=OmZp9qEsl5GfVcxyRnIi7bGuP4I=')
      }
      assert_equal(res.code, "403")
    end

    def test_filter1
      url = URI.parse('http://localhost:8000')
      res = Net::HTTP.start(url.host, url.port) {|http|
        http.get('/filter1.html')
      }
      string = IO.read("#{@dir}/ngx_base/www/filter1.html")
      assert_equal(res["content-type"], "text/plain")
      assert_equal(res.body, "<!DOCTYPE html>\n" + string)
    end

# following tests are not pass through yet.

    def test_filter2
      url = URI.parse('http://localhost:8000')
      res = Net::HTTP.start(url.host, url.port) {|http|
        http.get('/filter2.html')
      }
      assert_equal(res["content-type"], "text/plain")
    end

    def test_filter3
      url = URI.parse('http://localhost:8000')
      res = Net::HTTP.start(url.host, url.port) {|http|
        http.get('/filter3.html')
      }
      string = IO.read("#{@dir}/ngx_base/www/filter3.html")
      assert_equal(res["content-type"], "text/html")
      assert_equal(res.body, "<!DOCTYPE html>\n" + string)
    end

    def teardown
      `sudo pkill nginx`
    end
end
