// examples/vim-cmd.c - cross-platform client for hostd with config + REPL + set
// Build (Unix):    cc -Wall -Wextra -O2 -g -Iinclude -o vim-cmd examples/vim-cmd.c
// Build (MinGW):   x86_64-w64-mingw32-gcc -O2 -o vim-cmd.exe examples/vim-cmd.c -lws2_32

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <windows.h>
  #pragma comment(lib, "ws2_32.lib")
  typedef SOCKET socket_t;
  #define CLOSESOCK closesocket
  #define SOCKERR() WSAGetLastError()
  #define PATH_SEP '\\'
#else
  #include <unistd.h>
  #include <sys/socket.h>
  #include <sys/un.h>
  #include <netdb.h>
  #include <arpa/inet.h>
  #include <pwd.h>
  #include <sys/stat.h>
  #include <sys/types.h>
  typedef int socket_t;
  #define CLOSESOCK close
  #define SOCKERR() errno
  #define PATH_SEP '/'
#endif

// ----- Modes -----
typedef enum { VC_MODE_UNSET=0, VC_MODE_UNIX=1, VC_MODE_TCP=2 } vc_mode_t;

// ----- Config -----
typedef struct {
    vc_mode_t mode;
    char   socket_path[256];
    char   host[128];
    int    port;
    char   cfg_path[512];
} cfg_t;

#ifndef _WIN32
static const char *DEFAULT_UNIX_SOCK = "/tmp/hostd.sock";
#endif

// ----- small utils -----
static char *ltrim(char *s) {
    while (s && *s && isspace((unsigned char)*s)) s++;
    return s;
}
static void rstrip(char *s) {
    if (!s) return;
    size_t n = strlen(s);
    while (n && isspace((unsigned char)s[n-1])) s[--n] = 0;
}
static char *trim(char *s) { rstrip(s); return ltrim(s); }

static void default_cfg_path(char *out, size_t outsz) {
#ifdef _WIN32
    const char *appdata = getenv("APPDATA");
    if (appdata && *appdata) {
        snprintf(out, outsz, "%s\\vim-cmd\\config", appdata);
    } else {
        const char *home = getenv("USERPROFILE"); if (!home) home = "";
        snprintf(out, outsz, "%s\\AppData\\Roaming\\vim-cmd\\config", home);
    }
#else
    const char *xdg = getenv("XDG_CONFIG_HOME");
    if (xdg && *xdg) {
        snprintf(out, outsz, "%s/vim-cmd/config", xdg);
    } else {
        const char *home = getenv("HOME");
        if (!home || !*home) {
            struct passwd *pw = getpwuid(getuid());
            home = pw ? pw->pw_dir : "";
        }
        snprintf(out, outsz, "%s/.config/vim-cmd/config", home ? home : "");
    }
#endif
}

#ifndef _WIN32
static int ensure_parent_dir(const char *path) {
    // create parent dirs of path (very small helper)
    char buf[512]; snprintf(buf, sizeof buf, "%s", path);
    char *p = strrchr(buf, PATH_SEP); if (!p) return 0;
    *p = 0;
    // mkdir -p style (just one level needed here)
    #ifdef _WIN32
      (void)buf; return 0;
    #else
      if (mkdir(buf, 0700) == 0 || errno == EEXIST) return 0;
      return -1;
    #endif
}
#endif

// ----- cfg helpers -----
static void cfg_init_defaults(cfg_t *c) {
    memset(c, 0, sizeof(*c));
#ifdef _WIN32
    c->mode = VC_MODE_TCP;
    c->port = 9000;
    snprintf(c->host, sizeof(c->host), "%s", "127.0.0.1");
#else
    c->mode = VC_MODE_UNIX;
    snprintf(c->socket_path, sizeof(c->socket_path), "%s", DEFAULT_UNIX_SOCK);
#endif
    default_cfg_path(c->cfg_path, sizeof(c->cfg_path));
}

static void cfg_show(const cfg_t *c) {
    fprintf(stderr, "[cfg] mode=%s socket=%s host=%s port=%d cfg=%s\n",
        c->mode==VC_MODE_TCP?"tcp":(c->mode==VC_MODE_UNIX?"unix":"unset"),
        c->socket_path[0]?c->socket_path:"(n/a)",
        c->host[0]?c->host:"(n/a)",
        c->port,
        c->cfg_path[0]?c->cfg_path:"(none)");
}

static void cfg_load_file(cfg_t *c, const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) return; // optional
    char line[512];
    while (fgets(line, sizeof line, fp)) {
        char *p = ltrim(line);
        if (*p=='#' || *p==0) continue; // comment/blank
        rstrip(p);
        char *eq = strchr(p, '=');
        if (!eq) continue;
        *eq = 0;
        char *k = trim(p);
        char *v = trim(eq+1);
        if (!*k) continue;

        if (!strcasecmp(k,"mode")) {
#ifdef _WIN32
            if (!strcasecmp(v,"tcp")) c->mode = VC_MODE_TCP;
#else
            if (!strcasecmp(v,"tcp")) c->mode = VC_MODE_TCP;
            else if (!strcasecmp(v,"unix")) c->mode = VC_MODE_UNIX;
#endif
        } else if (!strcasecmp(k,"socket")) {
#ifndef _WIN32
            if (snprintf(c->socket_path, sizeof(c->socket_path), "%s", v) >= (int)sizeof(c->socket_path)) {
                fprintf(stderr, "config: socket path truncated to %zu bytes\n", sizeof(c->socket_path)-1);
            }
#endif
        } else if (!strcasecmp(k,"host")) {
            if (snprintf(c->host, sizeof(c->host), "%s", v) >= (int)sizeof(c->host)) {
                fprintf(stderr, "config: host truncated to %zu bytes\n", sizeof(c->host)-1);
            }
        } else if (!strcasecmp(k,"port")) {
            c->port = atoi(v);
        }
    }
    fclose(fp);
}

static int cfg_write_file(const cfg_t *c, const char *path) {
#ifndef _WIN32
    if (ensure_parent_dir(path) != 0) {
        perror("mkdir parent");
        return -1;
    }
#else
    // create %APPDATA%\vim-cmd if needed
    char dir[512]; snprintf(dir, sizeof dir, "%s", path);
    char *p = strrchr(dir, PATH_SEP); if (p) { *p = 0; CreateDirectoryA(dir, NULL); }
#endif
    FILE *fp = fopen(path, "w");
    if (!fp) { perror("open config for write"); return -1; }
#ifdef _WIN32
    fprintf(fp, "mode=tcp\n");
#else
    fprintf(fp, "mode=%s\n", c->mode==VC_MODE_TCP?"tcp":"unix");
    if (c->mode==VC_MODE_UNIX) fprintf(fp, "socket=%s\n", c->socket_path);
#endif
    if (c->mode==VC_MODE_TCP) {
        fprintf(fp, "host=%s\n", c->host[0]?c->host:"127.0.0.1");
        fprintf(fp, "port=%d\n", c->port>0?c->port:9000);
    }
    fclose(fp);
    fprintf(stderr, "[cfg] wrote %s\n", path);
    return 0;
}

// ----- connections -----
#ifndef _WIN32
static int connect_unix_path(const char *sock) {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket(AF_UNIX)"); return -1; }
    struct sockaddr_un addr; memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;

    size_t slen = strlen(sock);
    if (slen >= sizeof(addr.sun_path)) {
        fprintf(stderr, "unix socket path too long (max %zu): %s\n",
                sizeof(addr.sun_path) - 1, sock);
        CLOSESOCK(fd);
        return -1;
    }
    memcpy(addr.sun_path, sock, slen + 1);

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect(unix)"); CLOSESOCK(fd); return -1;
    }
    return fd;
}
#endif

static int connect_tcp_host(const char *host, int port) {
    char portstr[16]; snprintf(portstr, sizeof portstr, "%d", port);
    struct addrinfo hints, *res=NULL;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int err = getaddrinfo(host, portstr, &hints, &res);
    if (err) { fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err)); return -1; }

    socket_t fd = (socket_t)-1;
    for (struct addrinfo *ai=res; ai; ai=ai->ai_next) {
        fd = (socket_t)socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if ((int)fd < 0) continue;
        if (connect(fd, ai->ai_addr, (int)ai->ai_addrlen) == 0) { freeaddrinfo(res); return (int)fd; }
        CLOSESOCK(fd); fd = (socket_t)-1;
    }
    freeaddrinfo(res);
#ifdef _WIN32
    fprintf(stderr, "connect(tcp) failed, WSAErr=%d\n", SOCKERR());
#else
    perror("connect(tcp)");
#endif
    return -1;
}

static int connect_from_cfg(const cfg_t *c) {
    if (c->mode == VC_MODE_TCP) {
        if (!c->host[0] || c->port<=0) { fprintf(stderr, "tcp config incomplete\n"); return -1; }
        return connect_tcp_host(c->host, c->port);
    }
#ifndef _WIN32
    if (c->mode == VC_MODE_UNIX) {
        if (!c->socket_path[0]) { fprintf(stderr, "unix socket path missing\n"); return -1; }
        return connect_unix_path(c->socket_path);
    }
#endif
    fprintf(stderr, "no valid mode\n");
    return -1;
}

// ----- I/O -----
static int send_command_fd(int fd, const char *line) {
    size_t len = strlen(line);
#ifdef _WIN32
    if (len==0 || line[len-1] != '\n') {
        if (send(fd, line, (int)len, 0) < 0) return -1;
        if (send(fd, "\n", 1, 0) < 0) return -1;
    } else {
        if (send(fd, line, (int)len, 0) < 0) return -1;
    }
#else
    if (len==0 || line[len-1] != '\n') {
        if (write(fd, line, len) < 0) return -1;
        if (write(fd, "\n", 1) < 0) return -1;
    } else {
        if (write(fd, line, len) < 0) return -1;
    }
#endif

    char buf[8192];
#ifdef _WIN32
    int n = recv(fd, buf, (int)sizeof(buf)-1, 0);
    if (n < 0) { fprintf(stderr, "recv failed, WSAErr=%d\n", SOCKERR()); return -1; }
#else
    ssize_t n = read(fd, buf, sizeof(buf)-1);
    if (n < 0) { perror("read"); return -1; }
#endif
    if (n == 0) { fprintf(stderr, "server closed connection\n"); return -2; }
    buf[n] = 0;
    fputs(buf, stdout);
    return 0;
}

// ----- CLI -----
static void usage(const char *prog) {
#ifdef _WIN32
    fprintf(stderr,
        "Usage:\n"
        "  %s [-c cfgfile] [-T host:port] set key=value [key=value ...]\n"
        "  %s [-c cfgfile] [-T host:port] COMMAND [ARGS...]\n"
        "  %s [-c cfgfile] [-T host:port]\n"
        "Config: %%APPDATA%%\\vim-cmd\\config\n",
        prog, prog, prog);
#else
    fprintf(stderr,
        "Usage:\n"
        "  %s [-c cfgfile] [-S socket] [-T host:port] set key=value [key=value ...]\n"
        "  %s [-c cfgfile] [-S socket] [-T host:port] COMMAND [ARGS...]\n"
        "  %s [-c cfgfile] [-S socket] [-T host:port]\n"
        "Config: $XDG_CONFIG_HOME/vim-cmd/config or ~/.config/vim-cmd/config\n",
        prog, prog, prog);
#endif
}

static int apply_kv(cfg_t *cfg, const char *k, const char *v) {
    if (!k || !v) return -1;
    if (!strcasecmp(k,"mode")) {
#ifdef _WIN32
        if (!strcasecmp(v,"tcp")) cfg->mode = VC_MODE_TCP;
#else
        if (!strcasecmp(v,"tcp")) cfg->mode = VC_MODE_TCP;
        else if (!strcasecmp(v,"unix")) cfg->mode = VC_MODE_UNIX;
#endif
        else return -1;
    } else if (!strcasecmp(k,"host")) {
        if (snprintf(cfg->host, sizeof(cfg->host), "%s", v) >= (int)sizeof(cfg->host)) {
            fprintf(stderr, "host truncated to %zu bytes\n", sizeof(cfg->host)-1);
        }
    } else if (!strcasecmp(k,"port")) {
        cfg->port = atoi(v);
    } else if (!strcasecmp(k,"socket")) {
#ifndef _WIN32
        if (snprintf(cfg->socket_path, sizeof(cfg->socket_path), "%s", v) >= (int)sizeof(cfg->socket_path)) {
            fprintf(stderr, "socket path too long (limit %zu)\n", sizeof(cfg->socket_path)-1);
        }
#else
        fprintf(stderr, "socket not supported on Windows; ignoring\n");
#endif
    } else {
        return -1;
    }
    return 0;
}

int main(int argc, char **argv) {
#ifdef _WIN32
    WSADATA w;
    if (WSAStartup(MAKEWORD(2,2), &w) != 0) { fprintf(stderr, "WSAStartup failed\n"); return 1; }
#endif

    cfg_t cfg; cfg_init_defaults(&cfg);

    const char *cli_cfg = NULL;
    const char *cli_tcp = NULL;
#ifndef _WIN32
    const char *cli_sock = NULL;
#endif

#ifdef _WIN32
    int argi = 1;
    while (argi < argc) {
        if (!strcmp(argv[argi], "-c") && argi+1<argc) { cli_cfg = argv[argi+1]; argi+=2; continue; }
        if (!strcmp(argv[argi], "-T") && argi+1<argc) { cli_tcp = argv[argi+1]; argi+=2; continue; }
        if (!strcmp(argv[argi], "-h")) { usage(argv[0]); WSACleanup(); return 0; }
        break;
    }
#else
    int opt;
    while ((opt = getopt(argc, argv, "c:S:T:h")) != -1) {
        switch (opt) {
            case 'c': cli_cfg = optarg; break;
            case 'S': cli_sock = optarg; break;
            case 'T': cli_tcp  = optarg; break;
            case 'h': default: usage(argv[0]); return opt=='h'?0:1;
        }
    }
    int argi = optind;
#endif

    if (cli_cfg) {
        snprintf(cfg.cfg_path, sizeof(cfg.cfg_path), "%s", cli_cfg);
        cfg_load_file(&cfg, cfg.cfg_path);
    } else {
        cfg_load_file(&cfg, cfg.cfg_path);
    }
#ifndef _WIN32
    if (cli_sock) {
        cfg.mode = VC_MODE_UNIX;
        if (snprintf(cfg.socket_path, sizeof(cfg.socket_path), "%s", cli_sock) >= (int)sizeof(cfg.socket_path)) {
            fprintf(stderr, "socket path too long (limit %zu)\n", sizeof(cfg.socket_path)-1);
        }
    }
#endif
    if (cli_tcp) {
        const char *colon = strchr(cli_tcp, ':');
        if (!colon) { fprintf(stderr, "-T expects host:port\n");
#ifdef _WIN32
            WSACleanup();
#endif
            return 1;
        }
        size_t hl = (size_t)(colon - cli_tcp);
        if (hl >= sizeof(cfg.host)) { fprintf(stderr, "host too long\n");
#ifdef _WIN32
            WSACleanup();
#endif
            return 1;
        }
        memcpy(cfg.host, cli_tcp, hl); cfg.host[hl]=0;
        cfg.port = atoi(colon+1);
        cfg.mode = VC_MODE_TCP;
    }

    // ---- One-shot "set" subcommand: write config and exit
    if (argi < argc && !strcasecmp(argv[argi], "set")) {
        if (!cfg.cfg_path[0]) { default_cfg_path(cfg.cfg_path, sizeof cfg.cfg_path); }
        for (int i=argi+1; i<argc; i++) {
            char *eq = strchr(argv[i], '=');
            if (!eq) { fprintf(stderr, "expected key=value, got '%s'\n", argv[i]); continue; }
            *eq = 0;
            const char *k = argv[i];
            const char *v = eq+1;
            if (apply_kv(&cfg, k, v) != 0) fprintf(stderr, "unknown key '%s'\n", k);
        }
        cfg_write_file(&cfg, cfg.cfg_path);
#ifdef _WIN32
        WSACleanup();
#endif
        return 0;
    }

    // ---- One-shot command if remaining args exist (not "set")
    if (argi < argc) {
        size_t total=0; for (int i=argi;i<argc;i++) total += strlen(argv[i])+1;
        char *line = (char*)malloc(total+2); if (!line) { perror("malloc");
#ifdef _WIN32
            WSACleanup();
#endif
            return 1;
        }
        line[0]=0; for (int i=argi;i<argc;i++){ strcat(line, argv[i]); if (i+1<argc) strcat(line," "); }
        int fd = connect_from_cfg(&cfg);
        if (fd < 0) { free(line);
#ifdef _WIN32
            WSACleanup();
#endif
            return 2;
        }
        int rc = send_command_fd(fd, line);
        CLOSESOCK(fd);
        free(line);
#ifdef _WIN32
        WSACleanup();
#endif
        return (rc==0)?0:3;
    }

    // ---- Interactive REPL
    fprintf(stderr, "vim-cmd shell. Type /help. Using config: %s\n", cfg.cfg_path);
reconnect:
    cfg_show(&cfg);
    int fd = connect_from_cfg(&cfg);
    if (fd < 0) fprintf(stderr, "unable to connect; you can /connect or Ctrl-C\n");

#if defined(_WIN32)
    char ibuf[4096];
    for (;;) {
        fprintf(stderr, "vim-cmd> ");
        if (!fgets(ibuf, sizeof ibuf, stdin)) { fprintf(stderr, "\n"); break; }
        char *cmd = trim(ibuf);
#else
    char *line = NULL; size_t cap=0;
    for (;;) {
        fprintf(stderr, "vim-cmd> ");
        ssize_t n = getline(&line, &cap, stdin);
        if (n < 0) { fprintf(stderr, "\n"); break; }
        char *cmd = trim(line);
#endif
        if (*cmd==0) continue;

        // allow both /quit and plain quit
        if (!strcasecmp(cmd,"quit") || !strcasecmp(cmd,"exit") ||
            !strcasecmp(cmd,"/quit") || !strcasecmp(cmd,"/exit")) break;

        if (!strcasecmp(cmd,"/help")) {
#ifdef _WIN32
            fprintf(stderr,
                "Built-ins:\n"
                "  /help\n"
                "  /show\n"
                "  /set key=value [key=value ...]   (writes config)\n"
                "  /connect tcp <host> <port>\n"
                "  /quit | /exit\n");
#else
            fprintf(stderr,
                "Built-ins:\n"
                "  /help\n"
                "  /show\n"
                "  /set key=value [key=value ...]   (writes config)\n"
                "  /connect tcp <host> <port>\n"
                "  /connect unix <socket>\n"
                "  /quit | /exit\n");
#endif
            continue;
        }
        if (!strcasecmp(cmd,"/show")) { cfg_show(&cfg); continue; }

        if (!strncasecmp(cmd,"/set",4)) {
            // parse /set key=value [key=value ...]
            char *p = cmd+4;
            int changed = 0;
            while (*p) {
                while (*p && isspace((unsigned char)*p)) p++;
                if (!*p) break;
                char *kv = p;
                while (*p && !isspace((unsigned char)*p)) p++;
                char save = *p; *p = 0;
                char *eq = strchr(kv, '=');
                if (eq) {
                    *eq = 0;
                    const char *k = kv;
                    const char *v = eq+1;
                    if (apply_kv(&cfg, k, v) == 0) changed = 1;
                    else fprintf(stderr, "unknown key '%s'\n", k);
                } else {
                    fprintf(stderr, "expected key=value near '%s'\n", kv);
                }
                *p = save;
            }
            if (changed) cfg_write_file(&cfg, cfg.cfg_path);
            continue;
        }

        if (!strncasecmp(cmd,"/connect",8)) {
            char kind[16]={0}, a[256]={0}, b[64]={0};
            int n = sscanf(cmd+8, "%15s %255s %63s", kind, a, b);
            if (n >= 2) {
                if (!strcasecmp(kind,"tcp")) {
                    int p = (n>=3)?atoi(b):cfg.port;
                    if (p<=0) { fprintf(stderr, "bad port\n"); continue; }
                    cfg.mode = VC_MODE_TCP;
                    if (snprintf(cfg.host, sizeof(cfg.host), "%s", a) >= (int)sizeof(cfg.host)) {
                        fprintf(stderr, "host truncated to %zu bytes\n", sizeof(cfg.host)-1);
                    }
                    cfg.port = p;
#ifndef _WIN32
                } else if (!strcasecmp(kind,"unix")) {
                    cfg.mode = VC_MODE_UNIX;
                    if (snprintf(cfg.socket_path, sizeof(cfg.socket_path), "%s", a) >= (int)sizeof(cfg.socket_path)) {
                        fprintf(stderr, "socket path too long (limit %zu)\n", sizeof(cfg.socket_path)-1);
                    }
#endif
                } else {
#ifdef _WIN32
                    fprintf(stderr, "usage: /connect tcp <host> <port>\n");
#else
                    fprintf(stderr, "usage: /connect tcp <host> <port> | /connect unix <socket>\n");
#endif
                    continue;
                }
                if (fd >= 0) { CLOSESOCK(fd); fd = -1; }
                goto reconnect;
            } else {
#ifdef _WIN32
                fprintf(stderr, "usage: /connect tcp <host> <port>\n");
#else
                fprintf(stderr, "usage: /connect tcp <host> <port> | /connect unix <socket>\n");
#endif
            }
            continue;
        }

        if (fd < 0) { fprintf(stderr, "not connected; try /connect or /set\n"); continue; }
        int rc = send_command_fd(fd, cmd);
        if (rc == -2) { CLOSESOCK(fd); fd=-1; fprintf(stderr, "[info] reconnecting...\n"); fd = connect_from_cfg(&cfg); }
    }

    if (fd >= 0) CLOSESOCK(fd);
#ifndef _WIN32
    free(line);
#endif
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
