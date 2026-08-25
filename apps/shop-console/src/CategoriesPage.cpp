#include "CategoriesPage.h"
#include "SupabaseClient.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

CategoriesPage::CategoriesPage(SupabaseClient *client, QWidget *parent)
    : QWidget(parent), m_client(client) {
    auto *toolbar = new QHBoxLayout;
    auto *refreshButton = new QPushButton("Refresh", this);
    toolbar->addWidget(refreshButton);
    toolbar->addStretch();

    connect(refreshButton, &QPushButton::clicked, this, [this]() { m_client->fetchCategories(); });

    m_table = new QTableWidget(this);
    m_table->setColumnCount(2);
    m_table->setHorizontalHeaderLabels({"Category", "Visible on storefront"});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(kColName, QHeaderView::ResizeToContents);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setAlternatingRowColors(true);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(toolbar);
    layout->addWidget(m_table);

    connect(m_table, &QTableWidget::itemChanged, this, &CategoriesPage::handleItemChanged);

    connect(m_client, &SupabaseClient::categoriesFetched, this, [this](const QVector<Category> &categories) {
        m_categories = categories;
        rebuildTable();
    });
    connect(m_client, &SupabaseClient::categoriesFetchFailed, this, [this](const QString &message) {
        emit statusMessage("Failed to load categories: " + message);
    });
    m_client->fetchCategories();

    connect(m_client, &SupabaseClient::categoryUpdateFailed, this, [this](const QString &message) {
        emit statusMessage("Update failed: " + message);
        m_client->fetchCategories();
    });
    connect(m_client, &SupabaseClient::categoryUpdated, this, [this]() { m_client->fetchCategories(); });
}

void CategoriesPage::rebuildTable() {
    m_populating = true;

    m_table->setRowCount(m_categories.size());
    for (int row = 0; row < m_categories.size(); ++row) {
        const Category &category = m_categories[row];

        auto *nameItem = new QTableWidgetItem(category.name);
        nameItem->setData(Qt::UserRole, category.id);
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(row, kColName, nameItem);

        auto *visibleItem = new QTableWidgetItem;
        visibleItem->setFlags((visibleItem->flags() & ~Qt::ItemIsEditable) | Qt::ItemIsUserCheckable);
        visibleItem->setCheckState(category.visibleOnStorefront ? Qt::Checked : Qt::Unchecked);
        m_table->setItem(row, kColVisible, visibleItem);
    }

    m_populating = false;
    emit statusMessage(QString("%1 categor%2.").arg(m_categories.size()).arg(m_categories.size() == 1 ? "y" : "ies"));
}

void CategoriesPage::handleItemChanged(QTableWidgetItem *item) {
    if (m_populating || item->column() != kColVisible) return;

    const int row = item->row();
    const QTableWidgetItem *nameItem = m_table->item(row, kColName);
    const QString categoryId = nameItem ? nameItem->data(Qt::UserRole).toString() : QString();
    if (categoryId.isEmpty()) return;

    m_client->updateCategoryField(categoryId, "visible_on_storefront", item->checkState() == Qt::Checked);
}
