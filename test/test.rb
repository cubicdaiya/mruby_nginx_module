require "minitest/autorun"
require "net/empty_port"
require "net/http"

class TestMrubyNginxModule < MiniTest::Test
    def setup
      @dir = File.dirname(File.expand_path(__FILE__))
      nginx_options = " -p #{@dir}/ngx_base/ -c #{@dir}/ngx_base/etc/nginx.conf"
      nginx_bin     = ENV['NGINX_BIN']
      if nginx_bin == nil then
        nginx_bin = "/usr/sbin/nginx"
      end
      if !File.exists?(nginx_bin)
        puts "#{nginx_bin} is not a nginx binary"
        exit false
      end
      `sudo #{nginx_bin} #{nginx_options}`
      Net::EmptyPort.wait(8000, 10)
    end

    def get(path)
      url = URI.parse('http://localhost:8000')
      res = Net::HTTP.start(url.host, url.port) {|http|
        http.get(path)
      }
      yield res
    end

    def test_hello
      get('/hello') { |res|
        assert_equal(res["content-type"], "text/plain")
        assert_equal(res.body, "Hello, World!")
      }
    end

    def test_redirect
      get('/redirect') { |res|
        assert_equal(res.code, "301")
      }
    end

    def test_redirect_internal
      get('/redirect_internal') { |res|
        assert_equal(res.code, "200")
        assert_equal(res["content-type"], "text/plain")
        assert_equal(res.body, "Hello, World!")
      }
    end

    def test_ctx
      get('/ctx') { |res|
        assert_equal(res.body, "3")
      }
    end

    def test_acl_ok
      # This test is available until 2018/08/04 for expires
      get('/acl.html?accesskey=bokko&expires=1533240048&signature=cDcAm6xNVlsNwuX3CP3KGg/pIPU=') { |res|
        assert_equal(res.code, "200")
      }
    end

    def test_acl_ng
      get('/acl.html?accesskey=bokko&expires=1375558147&signature=OmZp9qEsl5GfVcxyRnIi7bGuP4I=') { |res|
        assert_equal(res.code, "403")
      }
    end

    def test_header_and_body_filter
      get('/header_and_body_filter.html') { |res|
        string = IO.read("#{@dir}/ngx_base/www/header_and_body_filter.html")
        assert_equal(res["content-type"], "text/plain")
        assert_equal(res.body, "<!DOCTYPE html>\n" + string)
      }
    end

    def test_header_filter_only
      get('/header_filter_only.html') { |res|
        assert_equal(res["content-type"], "text/plain")
      }
    end

    def test_body_filter_only
      get('/body_filter_only.html') { |res|
        string = IO.read("#{@dir}/ngx_base/www/body_filter_only.html")
        assert_equal(res["content-type"], "text/html")
        assert_equal(res.body, "<!DOCTYPE html>\n" + string)
      }
    end

    def test_set
      get('/set') { |res|
        assert_equal(res.body, "cubicdaiya\n")
      }
    end

    def teardown
      `sudo pkill nginx`
    end
end
