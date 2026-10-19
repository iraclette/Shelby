#pragma once

#include <QVector>
#include <QWidget>

#include "SupabaseClient.h"

class QTableWidget;
class QTableWidgetItem;

// Toggles which categories show on the storefront.
class CategoriesPage : public QWidget {
    Q_OBJECT

public:
    explicit CategoriesPage(SupabaseClient *client, QWidget *parent = nullptr);

    // Asks for a name and an optional parent (making it a subcategory), then creates it.
    // Shared with ProductDialog's "+ New".
    static void promptForNewCategory(QWidget *parent, SupabaseClient *client, const QVector<Category> &categories);

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
