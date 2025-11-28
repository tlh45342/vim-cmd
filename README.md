# VIM-CMD

**VIM-CMD** is a lightweight command-line client designed to communicate with **hostd**, a CPU-emulator management service.  
The name comes from **Virtual Infrastructure Manager – Command**, reflecting its intended role as a simple operational tool.

This project has **no affiliation with VMware**, ESXi, or the VMware `vim-cmd` utility.  
The name is an intentional nod, but the implementation is entirely independent.

> **WORK IN PROGRESS**  
> Both `hostd` and `vim-cmd` are under active development.  
> Expect frequent changes until the command set, protocol, and configuration format stabilize.

---

## Installation

```sh
cd /opt
git clone https://github.com/tlh45342/vim-cmd.git
cd vim-cmd
make
make install
```

This builds the CLI and installs it into the user’s environment.

---

## macOS Installation Notes

On macOS, `make install` places the `vim-cmd` binary into:

```
~/bin
```

If `~/bin` isn’t already listed in your `$PATH`, the installer will add it **once**.  
Because `$PATH` is evaluated at shell startup, you may need to:

- Close and reopen Terminal, or  
- Run:  
  ```sh
  source ~/.zshrc
  ```
  (or `~/.bashrc` if you use Bash)

---

## Overview

`vim-cmd` is a minimalist front-end for interacting with **hostd**, the backend ARM/VM management daemon.

Current goals include:

- A clean, simple command-line interface  
- Zero automatic connections — everything is explicit  
- Persistent configuration of TCP or UNIX transport modes  
- A local REPL with internal commands (`/set`, `/show`, `/connect`, `version`, etc.)  
- A safe, human-readable config file

Security features (authentication, encryption, access control) will be introduced once the command plumbing is stable.

---

## Configuration

`vim-cmd` loads configuration automatically on startup and rewrites the config file whenever you use `/set`.

Supported configuration keys:

| Key   | Description                          | Notes                                  |
|-------|--------------------------------------|----------------------------------------|
| `mode` | `tcp` or `unix`                     | `unix` available only on POSIX systems |
| `host` | Target hostname or IP               | Used only when `mode=tcp`              |
| `port` | TCP port number                     | Used only when `mode=tcp`              |
| `socket` | UNIX socket path                  | Used only when `mode=unix` (read-only via REPL) |

### Example TCP configuration file

```
mode=tcp
host=127.0.0.1
port=9000
```

### Example UNIX configuration file (POSIX only)

```
mode=unix
socket=/tmp/hostd.sock
```

`vim-cmd` keeps the configuration **clean and canonical**.

---

## Config File Location

`vim-cmd` stores its configuration in OS-appropriate locations.

### Linux / macOS
```
~/.config/vim-cmd/config
```

### Windows
```
%APPDATA%\\vim-cmd\\config
```

Typical Windows example:
```
C:\\Users\\<username>\\AppData\\Roaming\\vim-cmd\\config
```

---

## Remote Connection Modes

### TCP Mode

Use when communicating with a remote or local `hostd` over TCP.

Examples:

```
/set mode=tcp host=127.0.0.1 port=9000
/connect tcp 127.0.0.1 9000
```

### UNIX Socket Mode (POSIX Only)

Use when communicating with a local `hostd` via UNIX domain sockets.

Examples:

```
/set mode=unix
/connect unix /tmp/hostd.sock
```

---

## Security Notice

There is currently:

- **No TLS**
- **No authentication**
- **No encryption**
- **No access control**

**Do not expose `hostd` or vim-cmd TCP ports to untrusted networks.**

---

## REPL Commands

Launching `vim-cmd` without arguments enters the interactive prompt:

```
vim-cmd>
```

Available built-in commands:

| Command                | Description                                |
|------------------------|--------------------------------------------|
| `version` / `/version` | Show vim-cmd version                       |
| `/help`                | Display help text                          |
| `/show`                | Show current configuration                 |
| `/set key=value ...`  | Update config and rewrite config file      |
| `/connect ...`        | Connect to `hostd` (TCP or UNIX)           |
| `/quit`, `/exit`      | Exit the REPL                              |

### Example Session

```
vim-cmd> /set mode=tcp host=192.168.170.1 port=9000
[cfg] wrote /home/user/.config/vim-cmd/config

vim-cmd> /show
[cfg] mode=tcp socket=/tmp/hostd.sock host=192.168.170.1 port=9000 cfg=/home/user/.config/vim-cmd/config

vim-cmd> /connect tcp 192.168.170.1 9000
```

`vim-cmd` **never auto-connects**; all connections must be initiated by the user.

---

## Command-Line Usage

```
vim-cmd [-c cfgfile] [-S socket] [-T host:port] [COMMAND ...]
vim-cmd set key=value [key=value ...]
vim-cmd version
```

### Options

| Option | Meaning                                   |
|--------|-------------------------------------------|
| `-c`   | Use an explicit configuration file        |
| `-S`   | Use a UNIX socket (POSIX only)            |
| `-T`   | Use TCP with host:port                    |
| `-v`   | Verbose mode (show config after connect)  |
| `-V`   | Print version and exit                    |
| `-h`   | Show help                                 |

---

## Project Status

The project is currently in the **foundational phase**.  
The focus is on:

- A stable CLI  
- Reliable persistent configuration  
- A clear separation between `vim-cmd` (client) and `hostd` (server)  
- Proper TCP and UNIX connectivity  
- Preparing a structured protocol for future commands  

Pending features:

- Authentication + encrypted transports  
- Robust request/response API  
- Expanded command set  
- Automated test coverage  
- Improved diagnostics and error handling  

---

## License

MIT License (unless otherwise noted in source files).
