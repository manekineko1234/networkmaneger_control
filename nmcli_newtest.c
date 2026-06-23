#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> /* 追加: sleep関数を使用するため */

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

    printf("WiFiシグナル強度の監視を開始します (終了するには Ctrl+C)\n");

    /* 無限ループで継続的に取得・表示を行う */
    while (1) {
        if (get_rssi_nmcli(iface, &rssi) == 0) {
            /* \r を使って行頭に戻り、前回の出力を上書きする */
            /* 行末のスペースは、桁数が減った際の古い文字の残骸を消すため */
            printf("\r%s Signal Strength: %d%%       ", iface, rssi);
            fflush(stdout); /* バッファをフラッシュして即座に画面に反映させる */
        } else {
            printf("\r%s の情報取得に失敗しました。再試行中...       ", iface);
            fflush(stdout);
        }
        
        sleep(1); /* 1秒ごとに更新 (負荷軽減のため) */
    }

    return 0;
}
