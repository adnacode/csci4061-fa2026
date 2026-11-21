import socket
import pathlib

MIME_TYPES = {
    ".txt": "text/plain",
    ".html": "text/html",
    ".jpg": "text/jpeg",
    ".png": "image/png",
    ".pdf": "application/pdf",
    ".mp3": "audio/mpeg",
}


# Read in an HTTP request from a socket
# 'conn': The socket to read from
# Returns the name of the requested resource
def read_request(conn: socket.socket) -> str:
    reader = conn.makefile("rb")

    request_line = reader.readline().decode("utf-8")

    parts = request_line.split()
    if len(parts) >= 2:
        resource = parts[1].lstrip("/")
    else:
        resource = ""

    while True:
        line = reader.readline()
        if line == b"\r\n" or line == b"\n" or line == b"":
            break

    reader.close()
    return resource

# Write an HTTP response to a socket
# 'conn': The socket to write the request to
# 'file_path': A complete path (e.g., "server_files/quote.txt") for the file to
#   send in the response. Send a 404 response if this file does not exist.
def write_response(conn: socket.socket, file_path: str) -> None:
    path = pathlib.Path(file_path)

    if not path.is_file():
        response = (
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Length: 0\r\n"
            "Connection: close\r\n"
            "\r\n"
        )
        conn.sendall(response.encode("utf-8"))
        return

    size = path.stat().st_size
    mime_type = MIME_TYPES.get(path.suffix, "application/octet-stream")
    header = (
        "HTTP/1.1 200 OK\r\n"
        f"Content-Type: {mime_type}\r\n"
        f"Contect-Length: {size}\r\n"
        "Connection: close\r\n"
        "\r\n"
    )

    conn.sendall(header.encode("utf-8"))

    with open(file_path, "rb") as f:
        conn.sendfile(f)
