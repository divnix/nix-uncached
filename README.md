# nix-uncached

This is a standalone implementation of [NixOS/nix#7526](https://github.com/NixOS/nix/pull/7526), so users can make use of it immediately.

## Example

In the following example we calculate all the build _and_ runtime dependencies of svn which are uncached:

```console
$ drv=$(nix-store -qd $(which svn))
$ nix-uncached $(nix-store -qR --include-outputs $drv)
/nix/store/5mbglq5ldqld8sj57273aljwkfvj22mc-subversion-1.1.4
/nix/store/9lz9yc6zgmc0vlqmn2ipcpkjlmbi51vv-glibc-2.3.4
...
```
