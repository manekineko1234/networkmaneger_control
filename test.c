#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// RSSIを取得する関数
int get_rssi(const char *iface) {
    FILE *fp = fopen("/proc/net/wireless", "r");
    if (fp == NULL) {
        return 0; // ファイルが開けない場合はエラー
    }

    char line[256];
    int rssi = 0;

    // ファイルを1行ずつ読み込む
    while (fgets(line, sizeof(line), fp)) {
        // 自分のインターフェース名（例: wlp2s0）が含まれている行を探す
        if (strstr(line, iface) != NULL) {
            // 見つかったら、スペースやコロンで区切って文字を分割する
            char *token = strtok(line, " :"); 
            
            if (token != NULL && strcmp(token, iface) == 0) {
                token = strtok(NULL, " "); // 1つ目: 状態 (status)
                token = strtok(NULL, " "); // 2つ目: リンク品質 (link)
                token = strtok(NULL, " "); // 3つ目: 電波強度 RSSI (level)
                
                if (token != NULL) {
                    // 「-66.」のような文字を、純粋なマイナスの数字(-66)に変換
                    rssi = atoi(token); 
                    break;
                }
            }
        }
    }
    fclose(fp);
    return rssi;
}

int main() {
    // ここを画面に表示されていた名前に変更
    // 例: wlan0 や wlp2s0 など
    const char *iface = "wlp2s0"; 
    
    printf("監視を開始します... (停止するには Ctrl+C を押してください)\n");

    // 無限ループで1秒ごとに監視
    while(1) {
        int rssi = get_rssi(iface);
        
        if (rssi != 0) {
            printf("現在の %s の電波強度: %d dBm\n", iface, rssi);
            
            // ★ 将来ここを「-80より小さくなったら切り替える」という処理に変えます
            if (rssi < -80) {
                printf("  -> [警告] 電波が弱いです！切り替えが必要です！\n");
            }
        } else {
            printf("情報を取得できませんでした。\n");
        }
        
        sleep(1); // 1秒待つ
    }
    
    return 0;
}
