#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/bpf.h>

#define BPF_DEVICE_NUM 4

int
main (int argc, char *argv[]) {
    int fd = -1, index, enable = 1;
    char path[16], *ifname, *buf;
    struct ifreq ifr;
    unsigned int size, offset;
    ssize_t n;
    struct bpf_hdr *hdr;

    if (argc != 2) {
        fprintf(stderr, "usage: %s ifname\n", argv[0]);
        return -1;
    }
    /* 空いているBPFデバイスをオープン */
    for (index = 0; index < BPF_DEVICE_NUM; index++) {
        snprintf(path, sizeof(path), "/dev/bpf%d", index);
        fd = open(path, O_RDWR, 0);
        if (fd != -1) {
            break;
        }
    }
    if (fd == -1) {
        perror("open");
        return -1;
    }
    /* BPFデバイスにインタフェースを紐づけ */
    ifname = argv[1];
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
    if (ioctl(fd, BIOCSETIF, &ifr) == -1) {
        perror("ioctl [BIOCSETIF]");
        close(fd);
        return -1;
    }
    /* BPFデバイスの内部バッファのサイズを取得 */
    if (ioctl(fd, BIOCGBLEN, &size) == -1) {
        perror("ioctl [BIOCGBLEN]");
        close(fd);
        return -1;
    }
    /* 同サイズの受信バッファを動的確保 */
    buf = malloc(size);
    if (!buf) {
        fprintf(stderr, "malloc: failed\n");
        close(fd);
        return -1;
    }
    /* ドキュメント参照 */
    if (ioctl(fd, BIOCPROMISC, NULL) == -1) {
        perror("ioctl [BIOCPROMISC]");
        free(buf);
        close(fd);
        return -1;
    }
    /* ドキュメント参照 */
    if (ioctl(fd, BIOCIMMEDIATE, &enable) == -1) {
        perror("ioctl [BIOCIMMEDIATE]");
        free(buf);
        close(fd);
        return -1;
    }
    /* ドキュメント参照 */
    if (ioctl(fd, BIOCSHDRCMPLT, &enable) == -1) {
        perror("ioctl [BIOCSHDRCMPLT]");
        free(buf);
        close(fd);
        return -1;
    }
    while (1) {
        /* BPFデバイスからデータ受信 */
        n = read(fd, buf, size);
        if (n == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("read");
            free(buf);
            close(fd);
            return -1;
        }
        /* 受信バッファ内のフレームを順に処理 */
        offset = 0;
        hdr = (struct bpf_hdr *)buf;
        while ((uintptr_t)hdr < (uintptr_t)buf + n) {
            printf("read [%d]: %zd bytes via %s\n", offset++, hdr->bh_caplen, ifname);
            /* BPF_WORDALIGN マクロを使って次のフレームの先頭へアクセス */
            hdr = (struct bpf_hdr *)((uintptr_t)hdr + BPF_WORDALIGN(hdr->bh_hdrlen + hdr->bh_caplen));
        }
    }
    free(buf);
    close(fd);
    return 0;
}