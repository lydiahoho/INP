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

void send_response(int client_socket, const char *status, const char *content_type, const char *body) {
    dprintf(client_socket, "HTTP/1.0 %s\r\n", status);
    dprintf(client_socket, "Content-Type: %s\r\n", content_type);
    dprintf(client_socket, "\r\n"); // End of headers
    dprintf(client_socket, "%s", body);
    return;
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
                send_response(client_socket, "404 Not Found", "text/html", error_body);
				close(client_socket);
                return;
			} else {
				const char *error_body = "<html><body><h1>403 Forbidden</h1></body></html>";
                send_response(client_socket, "403 Forbidden", "text/html", error_body); 
				close(client_socket);
                //fclose(file);
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
        send_response(client_socket, "404 Not Found", "text/html", error_body);
        close(client_socket); 
        return;   
    } else {
        const char *mime_type = get_mime_type(file_path);
        if(strncmp(mime_type, "000", 3) == 0){
			const char *location_format = "Location: %s/\r\n";
			char location_header[256];
			snprintf(location_header, sizeof(location_header), location_format, path);
			dprintf(client_socket, "HTTP/1.0 301 Moved Permanently\r\n%s\r\n", location_header);
            close(client_socket);
            //fclose(file);
            return;
        }

		// Send HTTP headers
		dprintf(client_socket, "HTTP/1.0 200 OK\r\nContent-Type: %s\r\n\r\n", mime_type);

		// Use sendfile to send the file content
        off_t offset = 0;
		struct stat file_stat;
		fstat(fileno(file), &file_stat);
		int file_fd = fileno(file);
		off_t total_bytes_sent = 0;
		while (total_bytes_sent < file_stat.st_size) {
			off_t bytes_sent = sendfile(client_socket, file_fd, &offset, file_stat.st_size - total_bytes_sent);
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
    int num_active_processes = 0;
    while (1) {
            struct sockaddr_in csin;
            socklen_t csinlen = sizeof(csin);

            if ((client_socket = accept(server_socket, (struct sockaddr*)&csin, &csinlen)) < 0) {
                perror("accept");
                continue;
            }

            if (num_active_processes >= 4800) {
                wait(NULL); // Wait for any child process to terminate
            }
            pid_t pid = fork();
            num_active_processes++;
            if (pid == 0) {
                close(server_socket); 
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
                if (strncmp(method, "GET", 3) != 0) {
                    // Unsupported HTTP method, send 501 status code
                    write(client_socket, "HTTP/1.0 501 Not Implemented\r\n\r\n", strlen("HTTP/1.0 501 Not Implemented\r\n\r\n"));
                } else {
                    // Handle GET request
                    handle_get_request(client_socket, path);
                }
                close(client_socket);
                exit(0);
                num_active_processes--;
            } else if (pid < 0) {
                perror("fuck");
                close(client_socket); 
            } else {
                close(client_socket); 
            }
    }

    //close(server_socket);
    return 0;
}