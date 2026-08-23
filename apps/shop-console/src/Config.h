#pragma once

#include <QString>

// Loads SUPABASE_URL / SUPABASE_ANON_KEY from a ".env" file next to the
// executable (see .env.example), falling back to process environment
// variables. Mirrors the .env.example convention used by the other apps
// in this repo.
struct Config {
    QString supabaseUrl;
    QString supabaseAnonKey;
    // Folder to default photo pickers to (e.g. wherever a phone's cloud
    // photo sync downloads new photos). Optional — falls back to the OS's
    // own remembered/default directory when empty or nonexistent.
    QString photosSyncDir;

    bool isValid() const { return !supabaseUrl.isEmpty() && !supabaseAnonKey.isEmpty(); }

    static Config load();
};
