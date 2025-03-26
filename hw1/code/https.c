#include <openssl/ssl.h>
#include <openssl/err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <libgen.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/sendfile.h>

#define errquit(m)  { perror(m); exit(-1); }


static int port_http = 80;
static int port_https = 443;
static const char *docroot = "/html";

SSL_CTX *create_context() {
    SSL_CTX *ctx;

    ctx = SSL_CTX_new(SSLv23_server_method());
    if (!ctx) {
        ERR_print_errors_fp(stderr);
        return NULL;
    }

    // 加载服务器证书和私钥
    if (SSL_CTX_use_certificate_file(ctx, "../cert/server.crt", SSL_FILETYPE_PEM) <= 0) {
        perror("SSL_CTX_use_certificate_file");
        exit(EXIT_FAILURE);
    }
    if (SSL_CTX_use_PrivateKey_file(ctx, "../cert/server.key", SSL_FILETYPE_PEM) <= 0) {
        perror("SSL_CTX_use_PrivateKey_file");
        exit(EXIT_FAILURE);
    }

    // 验证服务器证书和私钥匹配
    // if (!SSL_CTX_check_private_key(ctx)) {
    //     perror("Private key does not match the certificate public key");
    //     exit(EXIT_FAILURE);
    // }

    //SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);


    return ctx;
}

void init_openssl() {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

void cleanup_openssl() {
    EVP_cleanup();
}


int hex(char ch) {
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    } else {
        ch = (ch >= 'A' && ch <= 'Z') ? ch + 32 : ch; 
        return ch - 'a' + 10;
    }
}

char* url_decode(const char *src) {
    char *dest = malloc(strlen(src) + 1);
    char *p = dest;
    while (*src) {
        if (*src == '%' && src[1] && src[2]) {
            *p++ = hex(src[1]) << 4 | hex(src[2]);
            src += 3;
        } else {
            *p++ = *src++;
        }
    }
    *p = '\0';
    return dest;
}

const char *get_mime_type(const char *file_path) {
    static const char *default_type = "000"; // Default MIME type 
    static const struct {
        const char *extension;
        const char *mime_type;
    } mime_types[] = {
		{".html", "text/html"},
        { ".txt", "text/plain; charset=UTF-8" },
        { ".jpg", "image/jpeg" },
        { ".jpeg", "image/jpeg" },
        { ".png", "image/png" },
        { ".mp3", "audio/mpeg" },
        // Add more MIME types as needed
    };

    const char *ext = strrchr(file_path, '.');
    if (ext != NULL) {
        for (size_t i = 0; i < sizeof(mime_types) / sizeof(mime_types[0]); ++i) {
            if (strcmp(ext, mime_types[i].extension) == 0) {
                return mime_types[i].mime_type;
            }
        }
    }

    return default_type;
}

void send_response(SSL *ssl, const char *status, const char *content_type, const char *body) {
    char response[4096]; // Adjust buffer size as needed

    snprintf(response, sizeof(response), "HTTP/1.0 %s\r\nContent-Type: %s\r\n\r\n%s", status, content_type, body);

    SSL_write(ssl, response, strlen(response));
}

// Function to handle HTTP GET requests
void handle_get_request(SSL *ssl, const char *path) {
    char fp[256];
    FILE *file;
    chdir(docroot);

    // Construct the absolute file path based on the requested path
    sprintf(fp, "%s%s", docroot, path);
    char *decoded_path = url_decode(fp);

    // Extract and process query parameters
    char *file_path = strtok(decoded_path, "?"); // Get the path before the query parameters
    
    // Check if the requested path ends with a '/'
    int len = strlen(file_path);
    //free(decoded_path);
    if (len > 0 && file_path[len - 1] == '/') {
        const char *ht = "html/";
        const int ht_len = strlen(ht);
        if (strncmp(file_path + len - ht_len, ht, ht_len) != 0) {
            // If the file_path doesn't end with "html/", perform necessary actions
            file = fopen(file_path, "rb");
            if (file == NULL) {
                const char *error_body = "<html><body><h1>404 Not Found</h1></body></html>";
                send_response(ssl, "404 Not Found", "text/html", error_body);
                return;
            } else {
                const char *error_body = "<html><body><h1>403 Forbidden</h1></body></html>";
                send_response(ssl, "403 Forbidden", "text/html", error_body);
                fclose(file);
                return;
            }
        } else {
            // Append "index.html" to file_path
            const char *index_file = "index.html";
            strcat(file_path, index_file);
        }
    }

    file = fopen(file_path, "rb");

    if (file == NULL) {
        // File not found, send 404 status code 
        const char *error_body = "<html><body><h1>404 Not Found</h1></body></html>";
        send_response(ssl, "404 Not Found", "text/html", error_body);
        return;
    } else {
        const char *mime_type = get_mime_type(file_path);
        if (strncmp(mime_type, "000", 3) == 0) {
            const char *location_format = "Location: %s/\r\n";
            char location_header[256];
            snprintf(location_header, sizeof(location_header), location_format, path);
            char response_header[1024];
            sprintf(response_header, "HTTP/1.0 301 Moved Permanently\r\n%s\r\n", location_header);
            SSL_write(ssl, response_header, strlen(response_header));
            fclose(file);
            return;
        }

        // Send HTTP headers
        char response_header[1024];
        sprintf(response_header, "HTTP/1.0 200 OK\r\nContent-Type: %s\r\n\r\n", mime_type);
        SSL_write(ssl, response_header, strlen(response_header));

        // Use sendfile to send the file content
        off_t offset = 0;
        struct stat file_stat;
        fstat(fileno(file), &file_stat);
        int file_fd = fileno(file);
        off_t total_bytes_sent = 0;
        while (total_bytes_sent < file_stat.st_size) {
            off_t bytes_sent = sendfile(SSL_get_fd(ssl), file_fd, &offset, file_stat.st_size - total_bytes_sent);
            if (bytes_sent <= 0) {
                // Handle error or break the loop
                break;
            }
            total_bytes_sent += bytes_sent;
        }
        fclose(file);
        return;
    }
    return;
}


int main(int argc, char *argv[]) {
    int server_socket, client_socket;
    struct sockaddr_in sin;
    // Initialize OpenSSL
    init_openssl();

    // Create SSL context
    SSL_CTX *ctx = create_context();

    // Create and configure server_socket here (bind, listen, etc.)
    if (argc > 1) { port_http = strtol(argv[1], NULL, 0); }
    if (argc > 2) { if ((docroot = strdup(argv[2])) == NULL) errquit("strdup"); }
    if (argc > 3) { port_https = strtol(argv[3], NULL, 0); }

    if ((server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) errquit("socket");

    do {
        int v = 1;
        setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(v));
    } while (0);

    bzero(&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(port_https);

    if (bind(server_socket, (struct sockaddr*)&sin, sizeof(sin)) < 0) errquit("bind");
    if (listen(server_socket, SOMAXCONN) < 0) errquit("listen");

    while (1) {
        printf("ssl  1\n");
        // Accept incoming connections and handle SSL/TLS
        if ((client_socket = accept(server_socket, NULL, NULL)) < 0) {
            perror("accept");printf("ssl  2\n");
            // Handle error appropriately
            continue;
        }
        
        // 创建 SSL 对象
        SSL *ssl = SSL_new(ctx);
        SSL_set_fd(ssl, client_socket);

        // SSL 握手
        if (SSL_accept(ssl) <= 0) {
            perror("SSL_accept");
            SSL_shutdown(ssl);
            SSL_free(ssl);
            continue;
        }

        // 检查 SSL 连接是否建立
        X509 *cert = SSL_get_peer_certificate(ssl);
        if (cert == NULL) {
            perror("SSL_get_peer_certificate");
            SSL_shutdown(ssl);
            SSL_free(ssl);
            close(client_socket);
            continue;
        }

        // 判断是否为 SSL 连接
        if (SSL_get_verify_result(ssl) != X509_V_OK) {
            perror("SSL_get_verify_result");
            SSL_shutdown(ssl);
            SSL_free(ssl);
            close(client_socket);
            X509_free(cert);
            continue;
        }

        // 根据 SSL 套接字进行处理
        char request[1024];
        memset(request, 0, sizeof(request));
        if (SSL_read(ssl, request, sizeof(request) - 1) < 0) {
            perror("SSL_read");
            SSL_shutdown(ssl);
            SSL_free(ssl);
            close(client_socket);
            X509_free(cert);
            continue;
        }

        char method[10], path[256];
        sscanf(request, "%s %s", method, path);

        if (strncmp(method, "GET", 3) != 0) {
            const char *not_implemented = "HTTP/1.0 501 Not Implemented\r\n\r\n";
            SSL_write(ssl, not_implemented, strlen(not_implemented));
        } else {
            printf("ssl\n");
            handle_get_request(ssl, path); // Pass SSL object instead of client_socket
        }

        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(client_socket);
        X509_free(cert);
    }

    // 关闭和清理操作
    close(server_socket);
    SSL_CTX_free(ctx);
    cleanup_openssl();
    return 0;
}
