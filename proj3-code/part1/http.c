#include "http.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define BUFSIZE 512

const char *get_mime_type(const char *file_extension)
{
    if (strcmp(".txt", file_extension) == 0)
    {
        return "text/plain";
    }
    else if (strcmp(".html", file_extension) == 0)
    {
        return "text/html";
    }
    else if (strcmp(".jpg", file_extension) == 0)
    {
        return "image/jpeg";
    }
    else if (strcmp(".png", file_extension) == 0)
    {
        return "image/png";
    }
    else if (strcmp(".pdf", file_extension) == 0)
    {
        return "application/pdf";
    }
    else if (strcmp(".mp3", file_extension) == 0)
    {
        return "audio/mpeg";
    }

    return NULL;
}

int read_http_request(int fd, char *resource_name)
{
    char buffer[BUFSIZE];
    int total_read = 0;
    // reads from the socket until the full HTTP request is received
    while (total_read < BUFSIZE - 1)
    {
        ssize_t bytes = read(fd, buffer + total_read,
                             BUFSIZE - 1 - total_read);
        if (bytes <= 0)
        {
            return -1;
        }
        // keeps track of bytes read and terminates the string
        total_read += bytes;
        buffer[total_read] = '\0';
        // Check for the blank line marking the end of the request
        if (strstr(buffer, "\r\n\r\n") != NULL)
        {
            break;
        }
    }
    // make sure the complete request was received
    if (strstr(buffer, "\r\n\r\n") == NULL)
    {
        return -1;
    }
    // extract the requested file path from the GET request
    if (sscanf(buffer, "GET %s HTTP/1.0", resource_name) != 1)
    {
        return -1;
    }

    return 0;
}

int write_http_response(int fd, const char *resource_path)
{
    struct stat file_stat;
    char buffer[BUFSIZE];
    // Checks to see if the requested file exists
    if (stat(resource_path, &file_stat) == -1 ||
        !S_ISREG(file_stat.st_mode))
    {
        const char *response =
            "HTTP/1.0 404 Not Found\r\n"
            "Content-Length: 0\r\n"
            "\r\n";

        size_t remaining = strlen(response);
        const char *ptr = response;
        while (remaining > 0)
        {
            ssize_t n = write(fd, ptr, remaining);
            if (n <= 0)
            {
                return -1;
            }
            ptr += n;
            remaining -= n;
        }
        return 0;
    }
    int file_fd = open(resource_path, O_RDONLY);
    if (file_fd == -1)
    {
        return -1;
    }
    const char *extension = strrchr(resource_path, '.');
    const char *mime_type = extension ? get_mime_type(extension) : NULL;

    if (mime_type == NULL)
    {
        mime_type = "application/octet-stream";
    }
    // HTTP response headers
    char header[BUFSIZE];
    int header_len = snprintf(header, sizeof(header),
                              "HTTP/1.0 200 OK\r\n"
                              "Content-Type: %s\r\n"
                              "Content-Length: %lld\r\n"
                              "\r\n",
                              mime_type, (long long)file_stat.st_size);

    if (header_len < 0 || header_len >= (int)sizeof(header))
    {
        close(file_fd);
        return -1;
    }
    // headers to the client
    size_t sent = 0;
    while (sent < (size_t)header_len)
    {
        ssize_t n = write(fd, header + sent, header_len - sent);
        if (n <= 0)
        {
            close(file_fd);
            return -1;
        }
        sent += n;
    }
    // sends the file contents in chunks
    ssize_t bytes_read;
    while ((bytes_read = read(file_fd, buffer, BUFSIZE)) > 0)
    {
        ssize_t total_written = 0;
        while (total_written < bytes_read)
        {
            ssize_t n = write(fd, buffer + total_written,
                              bytes_read - total_written);
            if (n <= 0)
            {
                close(file_fd);
                return -1;
            }
            total_written += n;
        }
    }
    // Close the file after sending the response
    close(file_fd);

    return bytes_read == -1 ? -1 : 0;
}
