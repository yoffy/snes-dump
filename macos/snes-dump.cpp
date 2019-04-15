#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

//! Set baud-rate
//!
//! @return 0 if success, or -1 if error
int SetBaud(int fd, int baud)
{
    termios options;
    if ( tcgetattr(fd, &options) < 0 ) {
        return -1;
    }
    if ( cfsetispeed(&options, baud) < 0 ) {
        return -1;
    }
    if ( cfsetospeed(&options, baud) < 0 ) {
        return -1;
    }
    if ( tcsetattr(fd, TCSANOW, &options) < 0 ) {
        return -1;
    }
    return 0;
}

//! Handshake
//!
//! @return 0 if success, or -1 if error
int Handshake(int fd)
{
    // wait `Hello`
    uint8_t data[128];
    ssize_t numRead = 0;

    while ( true ) {
        numRead = read(fd, data, sizeof(data));
        if ( numRead < 0 ) {
            return -1;
        }
        if ( numRead == 0 ) {
            return -1;
        }
        if ( memmem(data, sizeof(data), "Hello", 5) ) {
            break;
        }
    }

    // send any 1-byte
    if ( write(fd, data, 1) <= 0 ) {
        return -1;
    }

    return 0;
}

//! Receive ROM size
//!
//! @return ROM size, or -1 if error
ssize_t ReadRomSize(int fd)
{
    uint16_t romSizeKB = 0;

    if ( read(fd, &romSizeKB, sizeof(romSizeKB)) < 0 ) {
        return -1;
    }
    romSizeKB = (romSizeKB >> 8) | ((romSizeKB & 0xFF) << 8);

    return ssize_t(romSizeKB) << 10;
}

//! Receive ROM
//!
//! @return 0 if success, or -1 if error
int DumpRom(int fd, size_t romSize)
{
    uint8_t data[4096];
    ssize_t numRead = 0;
    size_t size = 0;
    size_t pos = 0;

    while ( true ) {
        numRead = read(fd, &data[pos], sizeof(data) - pos);
        if ( numRead <= 0 ) {
            return -1;
        }

        pos += numRead;
        size += numRead;

        if ( size == romSize || pos == sizeof(data) ) {
            write(STDOUT_FILENO, data, pos);
            pos = 0;
            fprintf(stderr, "\e[1F%lu KB\n", size >> 10);
        }
        if ( size == romSize ) {
            break;
        }
    }

    return 0;
}

int main(int argc, char* argv[])
{
    if ( argc < 2 ) {
        fprintf(stderr, "%s device-file\n", argv[0]);
        return 1;
    }

    const char* devicePath = argv[1];

    int fd = open(devicePath, O_RDWR | O_NOCTTY);
    if ( fd < 0 ) {
        perror("open");
        return 1;
    }

    if ( SetBaud(fd, B230400) < 0 ) {
        perror("SetBaud");
        return 1;
    }

    if ( Handshake(fd) < 0 ) {
        perror("Handshake");
        return 1;
    }

    ssize_t romSize = ReadRomSize(fd);
    if ( romSize < 0 ) {
        perror("ReadRomSize");
        return 1;
    }

    fprintf(stderr, "Size: %ld KB\n", romSize / 1024);
    fprintf(stderr, "\n\n");

    if ( DumpRom(fd, size_t(romSize)) < 0 ) {
        perror("DumpRom");
        return 1;
    }

    close(fd);

    return 0;
}
