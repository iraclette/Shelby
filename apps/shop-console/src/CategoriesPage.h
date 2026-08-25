#pragma once

#include <QVector>
#include <QWidget>

#include "SupabaseClient.h"

class QTableWidget;
class QTableWidgetItem;

// Category visibility toggle: lets an admin hide a whole storefront section
// (e.g. "Handmade Leather") before it's ready, without deleting the
// category or its products. Category *creation* is unchanged — still the
// inline "+ New" prompt in ProductDialog; this page only manages the
// visible_on_storefront flag, no rename/delete (no such UI exists anywhere
// else in this app either).
class CategoriesPage : public QWidget {
    Q_OBJECT

public:
    explicit CategoriesPage(SupabaseClient *client, QWidget *parent = nullptr);

signals:
    void statusMessage(const QString &message);

private:
    void rebuildTable();
    void handleItemChanged(QTableWidgetItem *item);

    SupabaseClient *m_client;
    QTableWidget *m_table;

    QVector<Category> m_categories;
    bool m_populating = false;

    static constexpr int kColName = 0;
    static constexpr int kColVisible = 1;
};
