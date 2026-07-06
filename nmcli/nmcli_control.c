#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUF_SIZE 512

static void trim_newline(char *s) {
    size_t len = strlen(s);
    if (len > 0 && s[len - 1] == '\n') {
        s[len - 1] = '\0';
    }
}

static int get_rssi_dbm_nmcli(const char *iface, int *rssi_dbm) {
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
        if (line[0] == '*' && line[1] == ':') {
            int rssi = 0;
            if (sscanf(line + 2, "%d", &rssi) == 1) {
                *rssi_dbm = rssi;
                pclose(fp);
                return 0;
            }
        }
    }

    pclose(fp);
    return -1;
}

static int set_bond_primary(const char *bond, const char *iface) {
    char cmd[BUF_SIZE * 2];

    snprintf(cmd, sizeof(cmd),
             "nmcli con mod %s bond.options \"mode=active-backup,miimon=100,primary=%s,primary_reselect=better\"",
             bond, iface);

    return system(cmd);
}

int main(int argc, char *argv[]) {
    const char *iface1 = "wlan0";
    const char *iface2 = "wlan1";
    const char *bond = "bond0";
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

        if (get_rssi_dbm_nmcli(iface1, &rssi1) != 0) {
            fprintf(stderr, "Failed to get RSSI for %s\n", iface1);
            sleep(interval_sec);
            continue;
        }

        if (get_rssi_dbm_nmcli(iface2, &rssi2) != 0) {
            fprintf(stderr, "Failed to get RSSI for %s\n", iface2);
            sleep(interval_sec);
            continue;
        }

        printf("%s RSSI: %d\n", iface1, rssi1);
        printf("%s RSSI: %d\n", iface2, rssi2);

        if (rssi1 > rssi2 + threshold_dbm) {
            printf("Decision: %s is better\n", iface1);
            if (set_bond_primary(bond, iface1) == 0) {
                printf("Updated bond primary to %s\n", iface1);
            } else {
                printf("Failed to update bond primary to %s\n", iface1);
            }
        } else if (rssi2 > rssi1 + threshold_dbm) {
            printf("Decision: %s is better\n", iface2);
            if (set_bond_primary(bond, iface2) == 0) {
                printf("Updated bond primary to %s\n", iface2);
            } else {
                printf("Failed to update bond primary to %s\n", iface2);
            }
        } else {
            printf("Decision: RSSI difference is within threshold, keep current\n");
        }

        printf("\n");
        sleep(interval_sec);
    }

    return 0;
}