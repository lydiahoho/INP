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




#define errquit(m)  { perror(m); exit(-1); }

static int port_http = 80;
static int port_https = 443;
static const char *docroot = "/html";



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

// Function to handle HTTP GET requests
void handle_get_request(int client_socket, const char *path) {
    char fp[256];
    FILE *file;
    chdir(docroot);

    // Construct the absolute file path based on the requested path
    sprintf(fp, "%s%s", docroot, path);
    char *decoded_path = url_decode(fp);

    // Extract and process query parameters
    char *file_path = strtok(decoded_path, "?"); // Get the path before the query parameters
    char *query_params = strtok(NULL, "?");     // Get the query parameters if present

    if (query_params != NULL) {
        // Process query parameters and their values
        char *param = strtok(query_params, "&");
        while (param != NULL) {
            char *value = strchr(param, '=');
            if (value != NULL) {
                *value++ = '\0'; // Split parameter and value
                printf("Parameter: %s, Value: %s\n", param, value);
                // Process the parameter and its value as needed
            }
            param = strtok(NULL, "&");
        }
    }
    
    // Check if the requested path ends with a '/'
    int len = strlen(file_path);
    if (len > 0 && file_path[len - 1] == '/') {
        const char *ht = "html/";
        const char *end = file_path + strlen(file_path) - strlen(ht);
        if(strcmp(end, ht) == 0){
            strcat(file_path, "index.html");
        }else{
            file = fopen(file_path, "rb");
            if (file == NULL) {
                write(client_socket, "HTTP/1.0 404 Not Found\r\n\r\n", strlen("HTTP/1.0 404 Not Found\r\n\r\n"));
                close(client_socket);
            }else {
                write(client_socket, "HTTP/1.0 403 Forbidden\r\n\r\n", strlen("HTTP/1.0 403 Forbidden\r\n\r\n")); 
                close(client_socket);
            }
        }
    }

    // Open the requested file
    file = fopen(file_path, "rb");

    if (file == NULL) {
        // File not found, send 404 status code 
        write(client_socket, "HTTP/1.0 404 Not Found\r\n\r\n", strlen("HTTP/1.0 404 Not Found\r\n\r\n"));
        close(client_socket);
        
    } else {
        // File found, determine MIME type based on file extension (simplified)
        const char *mime_type = "text/html"; // Default MIME type (.txt)
        char *ext = strrchr(file_path, '.');

        if (ext != NULL) {
            if (strcmp(ext, ".txt") == 0) {    // .html
                mime_type = "text/plain; charset=UTF-8";
            } else if (strcmp(ext, ".jpg") == 0 ){
                mime_type = "image/jpeg";
            } else if ( strcmp(ext, ".png") == 0) {
                mime_type = "image/png";
            } else if ( strcmp(ext, ".mp3") == 0) {
                mime_type = "audio/mpeg";
            } 
        }
        else{
            //write(client_socket, "HTTP/1.0 301 Moved Permanently\r\n\r\n", strlen("HTTP/1.0 301 Moved Permanently\r\n\r\n"));
            char location_header[256];
            sprintf(location_header, "Location: %s/", path);
            write(client_socket, "HTTP/1.0 301 Moved Permanently\r\n", strlen("HTTP/1.0 301 Moved Permanently\r\n"));
            write(client_socket, location_header, strlen(location_header));
            write(client_socket, "\r\n", strlen("\r\n"));
            close(client_socket);
        }

        // Send HTTP headers with appropriate MIME type
        dprintf(client_socket, "HTTP/1.0 200 OK\r\nContent-Type: %s\r\n\r\n", mime_type);

        // Read and send the file contents in chunks
        char buffer[1024];
        int bytes_read;
        while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
            write(client_socket, buffer, bytes_read);
        }
        fclose(file);
    }
}

int main(int argc, char *argv[]) {
    int server_socket, client_socket;
    struct sockaddr_in sin;

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
    sin.sin_port = htons(port_http);

    if (bind(server_socket, (struct sockaddr*)&sin, sizeof(sin)) < 0) errquit("bind");
    if (listen(server_socket, SOMAXCONN) < 0) errquit("listen");

    printf("Server running on port %d...\n", port_http);

    while (1) {
        struct sockaddr_in csin;
        socklen_t csinlen = sizeof(csin);

        if ((client_socket = accept(server_socket, (struct sockaddr*)&csin, &csinlen)) < 0) {
            perror("accept");
            continue;
        }

        char request[1024];
        memset(request, 0, sizeof(request));

        // Read the client's request
        if (read(client_socket, request, sizeof(request) - 1) < 0) {
            perror("read");
            close(client_socket);
            continue;
        }

        // Parse the request to get the HTTP method and requested path
        char method[10], path[256];
        sscanf(request, "%s %s", method, path);

        // Handle only GET requests
        if (strcmp(method, "GET") != 0) {
            // Unsupported HTTP method, send 501 status code
            write(client_socket, "HTTP/1.0 501 Not Implemented\r\n\r\n", strlen("HTTP/1.0 501 Not Implemented\r\n\r\n"));
        } else {
            // Handle GET request
            handle_get_request(client_socket, path);
        }

        // Close the client socket
        close(client_socket);
    }

    close(server_socket);
    return 0;
}