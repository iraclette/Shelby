#pragma once

#include <QString>

// What the login screen remembers between launches, per Windows user.
// The refresh token is a password-equivalent, so on Windows it's encrypted with
// DPAPI: only this Windows account on this PC can read it back.
namespace sessionstore {

QString lastUsername();
void setLastUsername(const QString &username);

bool staySignedIn();
void setStaySignedIn(bool enabled);

QString refreshToken();
void setRefreshToken(const QString &token);

// Set just before an update installs, so the relaunched app signs straight back in once.
bool resumeAfterUpdate();
void setResumeAfterUpdate(bool resume);

void clearSession();

} // namespace sessionstore
