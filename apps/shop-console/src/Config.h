#pragma once

#include <QString>

// Reads .env next to the exe, then env vars, then values baked in at build time.
struct Config {
    QString supabaseUrl;
    QString supabaseAnonKey;
    QString photosSyncDir;
    QString telegramBotUsername; // without the @; empty = Telegram not set up

    bool isValid() const { return !supabaseUrl.isEmpty() && !supabaseAnonKey.isEmpty(); }

    static Config load();
};
