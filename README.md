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
sudo apt install file  # Ubuntu/Debian etc
sudo pacman -S file  # Arch-based distros
```

Second, get your nginx version:
```bash
$ nginx -v
nginx version: nginx/1.18.0
```
In this case, the version is `1.18.0`, so the download URL will be `https://nginx.org/download/nginx-1.18.0.tar.gz`.

Then, execute below commands replacing with your nginx version.
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
sudo cp objs/ngx_http_mime_magic_module.so /etc/nginx/modules
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
