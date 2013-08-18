require "minitest/autorun"
require "net/empty_port"
require "net/http"

class TestMrubyNginxModule < MiniTest::Test
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

    def test_header_and_body_filter
      url = URI.parse('http://localhost:8000')
      res = Net::HTTP.start(url.host, url.port) {|http|
        http.get('/header_and_body_filter.html')
      }
      string = IO.read("#{@dir}/ngx_base/www/header_and_body_filter.html")
      assert_equal(res["content-type"], "text/plain")
      assert_equal(res.body, "<!DOCTYPE html>\n" + string)
    end

    def test_header_filter_only
      url = URI.parse('http://localhost:8000')
      res = Net::HTTP.start(url.host, url.port) {|http|
        http.get('/header_filter_only.html')
      }
      assert_equal(res["content-type"], "text/plain")
    end

    def test_body_filter_only
      url = URI.parse('http://localhost:8000')
      res = Net::HTTP.start(url.host, url.port) {|http|
        http.get('/body_filter_only.html')
      }
      string = IO.read("#{@dir}/ngx_base/www/body_filter_only.html")
      assert_equal(res["content-type"], "text/html")
      assert_equal(res.body, "<!DOCTYPE html>\n" + string)
    end

    def test_set
      url = URI.parse('http://localhost:8000')
      res = Net::HTTP.start(url.host, url.port) {|http|
        http.get('/set')
      }
      assert_equal(res.body, "cubicdaiya\n")
    end

    def teardown
      `sudo pkill nginx`
    end
end
