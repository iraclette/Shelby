#pragma once

#include <QString>

// Loads SUPABASE_URL / SUPABASE_ANON_KEY from a ".env" file next to the
// executable (see .env.example), falling back to process environment
// variables. Mirrors the .env.example convention used by the other apps
// in this repo.
struct Config {
    QString supabaseUrl;
    QString supabaseAnonKey;

    bool isValid() const { return !supabaseUrl.isEmpty() && !supabaseAnonKey.isEmpty(); }

    static Config load();
};
