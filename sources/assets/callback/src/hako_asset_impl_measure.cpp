#include "hako_asset_impl_measure.hpp"
#include <cstdio>
#include <string>
#include <vector>
#include <sys/time.h>

#define CSV_BUFFER_SIZE 100 // デフォルトバッファサイズ（行数）

struct HakoAssetImplMeasureCsvBuffer {
    std::vector<std::string> rows;
    size_t buffer_size;
    FILE *fp;

    HakoAssetImplMeasureCsvBuffer(size_t size) : buffer_size(size), fp(nullptr) {
        rows.reserve(size);
    }

    // バッファにデータを追加
    void add_row(const std::string& row) {
        rows.push_back(row);
    }

    // バッファが満タンか確認
    bool is_full() const {
        return rows.size() >= buffer_size;
    }

    // バッファをファイルにフラッシュ
    void flush() {
        if (fp == nullptr) {
            fprintf(stderr, "File pointer is null. Cannot flush buffer.\n");
            return;
        }

        // データをファイルに書き込む
        for (const auto& row : rows) {
            if (fprintf(fp, "%s\n", row.c_str()) < 0) {
                // 書き込みエラー時の処理
                fprintf(stderr, "Failed to write to file.\n");
                return;
            }
        }

        // 書き込みが成功した場合のみクリア
        rows.clear();
    }

    // ファイルをクローズ
    void close() {
        if (fp != nullptr) {
            flush(); // 残っているデータをすべて書き込む
            fclose(fp);
            fp = nullptr;
        }
    }
};

// 現在時間の取得
long get_current_time_usec() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

// CSVファイル作成
void* hako_asset_impl_measure_create_csv(const char* asset_name) {
    // HakoAssetImplMeasureCsvBuffer のインスタンスを生成
    HakoAssetImplMeasureCsvBuffer* buffer = new HakoAssetImplMeasureCsvBuffer(CSV_BUFFER_SIZE);
    if (buffer == nullptr) {
        return nullptr;
    }

    // ファイル名を作成
    std::string filename = std::string(asset_name) + "-measure.csv";
    
    // ファイルを開く（書き込みモード、存在しない場合は新規作成）
    buffer->fp = fopen(filename.c_str(), "w");
    
    if (buffer->fp == nullptr) {
        // エラーハンドリング
        perror("Failed to create CSV file");
        delete buffer;
        return nullptr;
    }

    // CSVのカラムをヘッダとして書き込む
    fprintf(buffer->fp, "real-time, core-time, asset-time\n");
    
    return buffer;
}

// CSVファイルに書き込み
void hako_asset_impl_measure_write_csv(void* vp, long long int core_time, long long int asset_time) 
{
    HakoAssetImplMeasureCsvBuffer* buffer = (HakoAssetImplMeasureCsvBuffer*)vp;
    if (buffer == nullptr || buffer->fp == nullptr) {
        fprintf(stderr, "Invalid buffer or file pointer.\n");
        return;
    }

    // 現在時刻を取得
    long real_time_usec = get_current_time_usec();
    
    // std::stringを利用して動的にバッファを作成
    std::string row = std::to_string(real_time_usec) + ", " +
                      std::to_string(core_time) + ", " +
                      std::to_string(asset_time);
    
    // バッファに追加
    buffer->add_row(row);

    // バッファが満タンならファイルに書き込み
    if (buffer->is_full()) {
        buffer->flush();
    }
}

void hako_asset_impl_measure_flush_csv(void* vp) {
    HakoAssetImplMeasureCsvBuffer* buffer = (HakoAssetImplMeasureCsvBuffer*)vp;
    if (buffer == nullptr) {
        return;
    }
    buffer->flush();
}

// CSVファイルをクローズ
void hako_asset_impl_measure_close_csv(void* vp) {
    HakoAssetImplMeasureCsvBuffer* buffer = (HakoAssetImplMeasureCsvBuffer*)vp;
    if (buffer == nullptr) {
        return;
    }

    // ファイルをクローズする前にフラッシュ
    buffer->close();

    // メモリを解放
    delete buffer;
}
