# Maximus-CBCS — Unix BBS Core

[castle-bbs-demo.webm](https://github.com/user-attachments/assets/a2b6adbb-3108-4c70-a9ef-2c8d36d0d345)


Maximus-CBCS is a GPL-2.0 Bulletin Board System core, originally developed by Lanius Corporation and ported to Unix by Wes Garland. This tree contains the native Unix source, compiler tools, stock configuration tree, message-base utilities, and the Castle of the Gods V compatibility and performance work.

The included video demonstrates the Castle deployment that uses this source. It is documentation media only; operational configuration and game assets remain deployment-specific.

## Current Working Features

### RIPscrip and terminal graphics

- RIPscrip capability is detected during login in `max/max_log.c`; callers that respond to the RIP probe receive RIP content, with ANSI and plain-text fallbacks for other terminals.
- `max/display.c` selects `.rbs` files for MECCA-aware RIP screens and raw `.rip` files for static or video scenes.
- Raw RIP scenes use `DisplayRipRaw()`: 64 KiB `ComWrite()` blocks avoid per-character display processing and allow large RIP animations to begin immediately at network speed.
- The Telnet transport in `comdll/ipcomm.c` coalesces burst output, preserves interactive prompt responsiveness, and normalizes CRLF input so a Return cannot leave a phantom LF for the next login or password prompt.

### Doors and external programs

- Maximus launches external applications through its outside-program subsystem in `max/max_xtrn.c`.
- Before launching an application, the BBS preserves caller state; on return it restores the user record and reconciles the time remaining value. This is the native foundation for classic BBS doors.
- The consuming BBS deployment supplies door-specific launch commands, drop files such as `DOOR.SYS`, and any container, DOS, or network bridge. Keeping that integration outside this core tree allows the same Maximus build to host native programs or external door runtimes.

### BBS services and tooling

- Telnet-capable communications driver with option negotiation in `comdll/`.
- ANSI, AVATAR, RIP, and TTY output paths, plus user terminal detection.
- Squish message-base tools in `squish/` and shared messaging APIs in `msgapi/`.
- MECCA screen compiler and MEX scripting support for menus, prompts, and custom BBS logic.
- Silt configuration compiler, MAID language compiler, file areas, message areas, user records, and protocol-transfer support.

## Build

The historical build and installation reference is [`QUICKSTART`](QUICKSTART). A typical Unix build uses:

```bash
./configure;
make build;
```

`make install` builds and installs Maximus, Squish, the support libraries, and the stock configuration tree. The target installation prefix is configured by `./configure`; review `vars.mk` before installing.

For a deployment that maintains its own runtime tree, build the libraries and binaries here, then use that deployment's scripts to copy binaries, compile control files, install artwork, and start the BBS listener.

## Repository Layout

- `max/` — main BBS server, display, login, menu, file, message, and external-program code.
- `comdll/` — Unix communications drivers, including Telnet.
- `squish/` and `msgapi/` — message-base implementation and API.
- `mec/`, `mex/`, and `ripmec/` — MECCA, MEX, and RIP source assets.
- `util/` — build-time and administrative utilities such as Silt, MAID, and MECCA.
- `ctl/` and `install_tree/` — stock control files and installable runtime configuration.
- `docs/` — upstream manuals and this README's media asset.

## License and Upstream Documentation

Maximus-CBCS is distributed under the GNU General Public License, version 2 or later. See [`LICENSE`](LICENSE) for the full terms, [`00-README.1ST`](00-README.1ST) for the original distribution notes, and [`docs/max_mast.txt`](docs/max_mast.txt) for the Maximus reference manual.
