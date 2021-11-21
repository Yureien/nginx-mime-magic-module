# nginx-mime-magic-module

Set Content-Type mime-type automatically from the magic bytes of some file! (Similar to file(1))

By default, it compulsorily reads the file (upto a limit) to be served to get the mimetype using libmagic.

You are able to configure the magic database to be used for this module. By default, it uses the default magic file for the system.

Optionally, you can use fallback mode. In fallback mode, first nginx will try to guess the mimetype (based off extension only) and if it fails, it'll use libmagic to guess mimetype of the file to be served.

## Warning

1. This module **was not** extensively tested. May have bugs that can crash your server. Report in issues if you find one, and I'll fix straight away.
2. Searching through the buffer for magic bytes has a performance cost. You can either enable this module in only one location if you want. You might want to enable fallback mode for some additional performance (which will only use this module if nginx fails to predict mimetype).

## Installation

### Arch Linux (AUR package)

You can install this directly from the AUR ([nginx-mod-mime-magic<sup>AUR</sup>](https://aur.archlinux.org/packages/nginx-mod-mime-magic/)):
```
yay -S nginx-mod-mime-magic
```

### Compile from source

If you have nginx with version greater than 1.9.11, congrats you don't have to recompile entire nginx! Follow this to load this module dynamically:

First, you need `libmagic.so`, it is provided with `file` with most distros. It is probably preinstalled.

```bash
sudo apt install libmagic-dev  # Ubuntu/Debian etc
sudo pacman -S file  # Arch-based distros
```

Second, get your nginx version and build flags:
```bash
$ nginx -v
nginx version: nginx/1.18.0
built with OpenSSL 1.1.1k  25 Mar 2021
TLS SNI support enabled
configure arguments: --with-cc-opt='-g -O2 -ffile-prefix-map=/build/nginx-q9LD4J/nginx-1.18.0=. -fstack-protector-strong -Wformat -Werror=format-security -fPIC -Wdate-time -D_FORTIFY_SOURCE=2' --with-ld-opt='-Wl,-z,relro -Wl,-z,now -fPIC' --prefix=/usr/share/nginx --conf-path=/etc/nginx/nginx.conf --http-log-path=/var/log/nginx/access.log --error-log-path=/var/log/nginx/error.log --lock-path=/var/lock/nginx.lock --pid-path=/run/nginx.pid --modules-path=/usr/lib/nginx/modules --http-client-body-temp-path=/var/lib/nginx/body --http-fastcgi-temp-path=/var/lib/nginx/fastcgi --http-proxy-temp-path=/var/lib/nginx/proxy --http-scgi-temp-path=/var/lib/nginx/scgi --http-uwsgi-temp-path=/var/lib/nginx/uwsgi --with-compat --with-debug --with-pcre-jit --with-http_ssl_module --with-http_stub_status_module --with-http_realip_module --with-http_auth_request_module --with-http_v2_module --with-http_dav_module --with-http_slice_module --with-threads --with-http_addition_module --with-http_gunzip_module --with-http_gzip_static_module --with-http_sub_module

```

Take note of the configure arguments because otherwise your module will not load because of "incompatible binary" error.
You can remove all the modules to save time (no `--with-*_module`) and add `--add-dynamic-module=../nginx-mime-magic-module/`, and you will need to add "`-lmagic`" at the end of the `--with-ld-opt` argument.

In this case, the version is `1.18.0`, so the download URL will be `https://nginx.org/download/nginx-1.18.0.tar.gz`.

Then, execute below commands replacing with your nginx version and build flags.
```bash 
# Please download a stable release.
# Latest stable release at https://github.com/FadedCoder/nginx-mime-magic-module/releases/latest
# Or, you can clone latest release (but not recommended):
# git clone https://github.com/FadedCoder/nginx-mime-magic-module.git
wget https://nginx.org/download/nginx-NGINX_VERSION_HERE.tar.gz -O nginx
tar -xf nginx-NGINX_VERSION_HERE.tar.gz
cd nginx-NGINX_VERSION_HERE
./configure --with-compat --with-ld-opt="-lmagic" --add-dynamic-module=../nginx-mime-magic-module/
make modules
sudo mkdir -p /usr/lib/nginx/modules
sudo cp objs/ngx_http_mime_magic_module.so /usr/lib/nginx/modules
```

## Configuration

At the top of your `nginx.conf` file (above `http {...}`) add:
```
load_module /usr/lib/nginx/modules/ngx_http_mime_magic_module.so;
```

| Name | Accepted Values | Works In | Description | Default |
| --- | --- | --- | --- | --- |
| `mime_magic` | `on`/`off` | `location`/`server`/`main` | Turn on the mime magic module with this. | `off` |
| `mime_magic_db` | `/path/to/magic/file` | `location`/`server`/`main` | **Optional** Choose which magic database to use. Useful for using a custom magic database. | automatically obtained |
| `mime_magic_fallback_mode` | `on`/`off` | `location`/`server`/`main` | **Optional** Turn on/off the mime magic fallback mode. It will first use nginx to predict mimetypes and only use libmagic if nginx fails. | `off` |

For example:

```
location /mimemagic {
    alias /srv/http;
    # ... Other config stuff ...

    mime_magic on;
    # mime_magic_db /path/to/magic;
    # mime_magic_fallback_mode off;
}
```

You can also enable or disable in different directories/ports:

```
location /mimemagic-enabled {
    # ... Other config stuff ...
    mime_magic on;
    # mime_magic_db /path/to/magic;
    # mime_magic_fallback_mode on;
}

location /mimemagic-disabled {
    # ... Other config stuff ...
    mime_magic off;
    # mime_magic_db /path/to/custom/magic;
    # mime_magic_fallback_mode off;
}
```

Then reload nginx. Probably `sudo systemctl restart nginx`.

## TODO

1. Try to do a benchmark of with/without and improve performance as far as possible. Gain inspiration from https://github.com/apache/httpd/blob/trunk/modules/metadata/mod_mime_magic.c maybe.

## Contribution

Just create a PR, and submit. Or open an issue. Thanks :)
