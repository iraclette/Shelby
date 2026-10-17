#include "SessionStore.h"

#include <QSettings>

#ifdef Q_OS_WIN
#include <windows.h>
#include <dpapi.h>
#endif

namespace {

QSettings settings() {
    return QSettings("Shelby", "ShopConsole");
}

QByteArray protect(const QByteArray &plain) {
#ifdef Q_OS_WIN
    DATA_BLOB in{static_cast<DWORD>(plain.size()), reinterpret_cast<BYTE *>(const_cast<char *>(plain.data()))};
    DATA_BLOB out{};
    if (!CryptProtectData(&in, L"Shop Console session", nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &out)) {
        return {};
    }
    const QByteArray result(reinterpret_cast<const char *>(out.pbData), static_cast<int>(out.cbData));
    LocalFree(out.pbData);
    return result;
#else
    return plain; // development builds on Linux/macOS only
#endif
}

QByteArray unprotect(const QByteArray &sealed) {
#ifdef Q_OS_WIN
    DATA_BLOB in{static_cast<DWORD>(sealed.size()), reinterpret_cast<BYTE *>(const_cast<char *>(sealed.data()))};
    DATA_BLOB out{};
    if (!CryptUnprotectData(&in, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &out)) {
        return {};
    }
    const QByteArray result(reinterpret_cast<const char *>(out.pbData), static_cast<int>(out.cbData));
    LocalFree(out.pbData);
    return result;
#else
    return sealed;
#endif
}

} // namespace

namespace sessionstore {

QString lastUsername() {
    return settings().value("login/lastUsername").toString();
}

void setLastUsername(const QString &username) {
    settings().setValue("login/lastUsername", username);
}

bool staySignedIn() {
    return settings().value("login/staySignedIn", false).toBool();
}

void setStaySignedIn(bool enabled) {
    settings().setValue("login/staySignedIn", enabled);
}

QString refreshToken() {
    const QByteArray sealed = QByteArray::fromBase64(settings().value("login/session").toByteArray());
    return sealed.isEmpty() ? QString() : QString::fromUtf8(unprotect(sealed));
}

void setRefreshToken(const QString &token) {
    const QByteArray sealed = protect(token.toUtf8());
    if (sealed.isEmpty()) {
        clearSession(); // never fall back to storing it unencrypted
        return;
    }
    settings().setValue("login/session", sealed.toBase64());
}

bool resumeAfterUpdate() {
    return settings().value("login/resumeAfterUpdate", false).toBool();
}

void setResumeAfterUpdate(bool resume) {
    settings().setValue("login/resumeAfterUpdate", resume);
}

void clearSession() {
    QSettings s = settings();
    s.remove("login/session");
    s.remove("login/resumeAfterUpdate");
}

} // namespace sessionstore
