#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE 256

static int get_rssi_nmcli(const char *iface, int *rssi) {
    char cmd[BUF_SIZE];
    char line[BUF_SIZE];
    FILE *fp;

    snprintf(cmd, sizeof(cmd),
             "nmcli -t -f IN-USE,SIGNAL dev wifi list ifname %s 2>/dev/null",
             iface);

    fp = popen(cmd, "r");
    if (!fp) {
        return -1;
    }

    while (fgets(line, sizeof(line), fp)) {
        char *p = strchr(line, ':');
        if (!p) {
            continue;
        }

        if (line[0] == '*') {
            *rssi = atoi(p + 1);
            pclose(fp);
            return 0;
        }
    }

    pclose(fp);
    return -1;
}

int main(int argc, char *argv[]) {
    const char *iface = "wlan0";
    int rssi;

    if (argc >= 2) {
        iface = argv[1];
    }

    if (get_rssi_nmcli(iface, &rssi) == 0) {
        printf("%s RSSI: %d\n", iface, rssi);
        return 0;
    }

    fprintf(stderr, "Failed to get RSSI for %s\n", iface);
    return 1;
}