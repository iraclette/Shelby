#pragma once

#include <QDateTime>
#include <QTimeZone>

#include "BusinessProfile.h"

// Supabase stores timestamps in UTC; the shops run on the business's time zone
// (Settings → Business). Day boundaries, "today" and every time shown on screen go
// through here, so a sale at 01:30 belongs to the day it happened, not the previous
// UTC day. Deliberately not the PC's own clock settings, which may be wrong.
namespace shoptime {

inline QTimeZone zone() {
    static QByteArray cachedId;
    static QTimeZone cached;
    const QByteArray id = BusinessProfile::instance()->settings().timezone.toUtf8();
    if (id != cachedId) {
        cachedId = id;
        const QTimeZone business(id);
        cached = business.isValid() ? business : QTimeZone::systemTimeZone();
    }
    return cached;
}

inline QDate today() {
    return QDateTime::currentDateTimeUtc().toTimeZone(zone()).date();
}

// Midnight starting `day` in the shops, as a UTC timestamp for PostgREST filters.
// Ends in "Z" rather than "+04:00", so there's no '+' to escape in the query string.
inline QString dayStartUtc(const QDate &day) {
    return QDateTime(day, QTime(0, 0), zone()).toUTC().toString(Qt::ISODate);
}

inline QDateTime fromDb(const QString &timestamp) {
    return QDateTime::fromString(timestamp, Qt::ISODateWithMs).toTimeZone(zone());
}

inline QDate dayOf(const QString &timestamp) {
    return fromDb(timestamp).date();
}

// e.g. format(order.createdAt, "yyyy-MM-dd HH:mm"); falls back to the raw text if it doesn't parse.
inline QString format(const QString &timestamp, const QString &pattern) {
    const QDateTime local = fromDb(timestamp);
    return local.isValid() ? local.toString(pattern) : timestamp;
}

} // namespace shoptime
