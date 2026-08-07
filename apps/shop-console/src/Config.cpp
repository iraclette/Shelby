#include "Config.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QProcessEnvironment>
#include <QTextStream>

static QMap<QString, QString> parseDotEnv(const QString &path) {
    QMap<QString, QString> values;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return values;
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) continue;

        const int eq = line.indexOf('=');
        if (eq < 0) continue;

        values.insert(line.left(eq).trimmed(), line.mid(eq + 1).trimmed());
    }
    return values;
}

Config Config::load() {
    const QString dotEnvPath = QDir(QCoreApplication::applicationDirPath()).filePath(".env");
    const QMap<QString, QString> fileValues = parseDotEnv(dotEnvPath);
    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

    Config config;
    config.supabaseUrl = fileValues.value("SUPABASE_URL", env.value("SUPABASE_URL"));
    config.supabaseAnonKey = fileValues.value("SUPABASE_ANON_KEY", env.value("SUPABASE_ANON_KEY"));
    return config;
}
