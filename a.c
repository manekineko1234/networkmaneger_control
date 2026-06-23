#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define BUF_SIZE 256

static void trim_newline(char *s) {
    size_t len = strlen(s);
    if (len > 0 && s[len - 1] == '\n') {
        s[len - 1] = '\0';
    }
}

static int get_rssi_dbm(const char *iface, int *rssi_dbm) {
    char cmd[BUF_SIZE];
    char line[BUF_SIZE];
    FILE *fp;

    snprintf(cmd, sizeof(cmd), "iw dev %s link 2>/dev/null", iface);
    fp = popen(cmd, "r");
    if (!fp) {
        return -1;
    }

    while (fgets(line, sizeof(line), fp)) {
        int rssi;
        if (sscanf(line, "signal: %d dBm", &rssi) == 1) {
            *rssi_dbm = rssi;
            pclose(fp);
            return 0;
        }
    }

    pclose(fp);
    return -1;
}

static int get_bond_active_slave(const char *bond, char *active_slave, size_t size) {
    char path[BUF_SIZE];
    FILE *fp;

    snprintf(path, sizeof(path), "/sys/class/net/%s/bonding/active_slave", bond);
    fp = fopen(path, "r");
    if (!fp) {
        return -1;
    }

    if (!fgets(active_slave, (int)size, fp)) {
        fclose(fp);
        return -1;
    }

    fclose(fp);
    trim_newline(active_slave);
    return 0;
}

static int set_bond_primary(const char *bond, const char *iface) {
    char cmd[BUF_SIZE * 2];
    int ret;

    snprintf(cmd, sizeof(cmd), "ip link set dev %s type bond primary %s", bond, iface);
    ret = system(cmd);
    if (ret == -1) {
        return -1;
    }

    if (WIFEXITED(ret) && WEXITSTATUS(ret) == 0) {
        return 0;
    }

    return -1;
}

int main(int argc, char *argv[]) {
    const char *bond = "bond0";
    const char *iface1 = "wlan0";
    const char *iface2 = "wlan1";
    int interval_sec = 5;
    int threshold_dbm = 5;

    if (argc >= 3) {
        iface1 = argv[1];
        iface2 = argv[2];
    }
    if (argc >= 4) {
        bond = argv[3];
    }
    if (argc >= 5) {
        interval_sec = atoi(argv[4]);
        if (interval_sec <= 0) interval_sec = 5;
    }
    if (argc >= 6) {
        threshold_dbm = atoi(argv[5]);
        if (threshold_dbm < 0) threshold_dbm = 5;
    }

    printf("Monitor start\n");
    printf("iface1=%s iface2=%s bond=%s interval=%d sec threshold=%d dBm\n\n",
           iface1, iface2, bond, interval_sec, threshold_dbm);

    while (1) {
        int rssi1, rssi2;
        char active_slave[BUF_SIZE] = {0};

        if (get_rssi_dbm(iface1, &rssi1) != 0) {
            fprintf(stderr, "Failed to get RSSI for %s\n", iface1);
            sleep(interval_sec);
            continue;
        }

        if (get_rssi_dbm(iface2, &rssi2) != 0) {
            fprintf(stderr, "Failed to get RSSI for %s\n", iface2);
            sleep(interval_sec);
            continue;
        }

        printf("%s RSSI: %d dBm\n", iface1, rssi1);
        printf("%s RSSI: %d dBm\n", iface2, rssi2);

        if (get_bond_active_slave(bond, active_slave, sizeof(active_slave)) == 0) {
            printf("Current active slave: %s\n", active_slave);
        } else {
            printf("Current active slave: (unknown)\n");
        }

        if (rssi1 > rssi2 + threshold_dbm) {
            printf("Decision: %s is better\n", iface1);
            if (strcmp(active_slave, iface1) != 0) {
                if (set_bond_primary(bond, iface1) == 0) {
                    printf("Switched primary to %s\n", iface1);
                } else {
                    printf("Failed to switch primary to %s\n", iface1);
                }
            }
        } else if (rssi2 > rssi1 + threshold_dbm) {
            printf("Decision: %s is better\n", iface2);
            if (strcmp(active_slave, iface2) != 0) {
                if (set_bond_primary(bond, iface2) == 0) {
                    printf("Switched primary to %s\n", iface2);
                } else {
                    printf("Failed to switch primary to %s\n", iface2);
                }
            }
        } else {
            printf("Decision: RSSI difference is within threshold, keep current\n");
        }

        printf("\n");
        sleep(interval_sec);
    }

    return 0;
}